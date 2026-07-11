#include "GA_Power_BungeeShot.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/Powers/BBBungeeCordActor.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBStunTagEffect.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Power_BungeeShot::UGA_Power_BungeeShot()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void UGA_Power_BungeeShot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !Hunter->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FBBBungeeAnchor HitAnchor;
	if (!TraceBungeeAnchor(HitAnchor))
	{
		UE_LOG(LogBreachborne, Log, TEXT("PowerAbility: BungeeShot missed - pending anchor cleared"));
		PendingAnchor.Reset();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (!PendingAnchor.IsValid())
	{
		PendingAnchor = HitAnchor;
		UE_LOG(LogBreachborne, Log, TEXT("PowerAbility: BungeeShot anchor set actor=%s loc=%s enemy=%d"),
			*GetNameSafe(HitAnchor.Actor.Get()), *HitAnchor.Location.ToCompactString(), HitAnchor.bIsEnemyHunter ? 1 : 0);
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		return;
	}

	ResolveBungeeLink(PendingAnchor, HitAnchor);
	PendingAnchor.Reset();
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

bool UGA_Power_BungeeShot::TraceBungeeAnchor(FBBBungeeAnchor& OutAnchor) const
{
	const AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		return false;
	}

	const FVector Direction = GetAimDirection();
	const FVector Start = Hunter->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);
	const FVector End = Start + Direction * MaxRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BungeeShotTrace), false, Hunter);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);

	FHitResult Hit;
	if (!Hunter->GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, Objects, Params))
	{
		return false;
	}

	AHunterCharacter* EnemyHunter = nullptr;
	OutAnchor.Actor = Hit.GetActor();
	OutAnchor.bIsEnemyHunter = IsEnemyHunter(Hit.GetActor(), EnemyHunter);
	OutAnchor.Location = OutAnchor.bIsEnemyHunter && EnemyHunter
		? EnemyHunter->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f)
		: (Hit.ImpactPoint.IsNearlyZero() ? Hit.Location : FVector(Hit.ImpactPoint));
	return OutAnchor.IsValid();
}

bool UGA_Power_BungeeShot::IsEnemyHunter(AActor* Actor, AHunterCharacter*& OutHunter) const
{
	OutHunter = Cast<AHunterCharacter>(Actor);
	if (!OutHunter)
	{
		return false;
	}

	const ABreachbornePlayerState* SourcePS = GetBBPlayerState();
	const ABreachbornePlayerState* TargetPS = OutHunter->GetPlayerState<ABreachbornePlayerState>();
	return SourcePS && TargetPS && SourcePS->GetTeamID() != TargetPS->GetTeamID() && TargetPS->GetIsAlive();
}

void UGA_Power_BungeeShot::ResolveBungeeLink(const FBBBungeeAnchor& First, const FBBBungeeAnchor& Second)
{
	const FVector FirstPoint = First.bIsEnemyHunter && First.Actor.IsValid()
		? First.Actor->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f)
		: First.Location;
	const FVector SecondPoint = Second.bIsEnemyHunter && Second.Actor.IsValid()
		? Second.Actor->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f)
		: Second.Location;
	SpawnVisualCord(FirstPoint, SecondPoint, 0.65f, false);

	AHunterCharacter* FirstHunter = First.bIsEnemyHunter ? Cast<AHunterCharacter>(First.Actor.Get()) : nullptr;
	AHunterCharacter* SecondHunter = Second.bIsEnemyHunter ? Cast<AHunterCharacter>(Second.Actor.Get()) : nullptr;

	if (FirstHunter && SecondHunter && FirstHunter != SecondHunter)
	{
		const FVector Midpoint = (FirstHunter->GetActorLocation() + SecondHunter->GetActorLocation()) * 0.5f;
		PullHunterToward(FirstHunter, Midpoint, EnemyEnemyPullSpeed);
		PullHunterToward(SecondHunter, Midpoint, EnemyEnemyPullSpeed);

		const float Distance = FVector::Dist(FirstHunter->GetActorLocation(), SecondHunter->GetActorLocation());
		if (Distance <= CollisionStunRange)
		{
			ApplyStun(FirstHunter);
			ApplyStun(SecondHunter);
		}

		UE_LOG(LogBreachborne, Log, TEXT("PowerAbility: BungeeShot linked enemies %s <-> %s dist=%.0f"),
			*GetNameSafe(FirstHunter), *GetNameSafe(SecondHunter), Distance);
		return;
	}

	if (FirstHunter || SecondHunter)
	{
		AHunterCharacter* TargetHunter = FirstHunter ? FirstHunter : SecondHunter;
		const FVector ObjectPoint = FirstHunter ? SecondPoint : FirstPoint;
		PullHunterToward(TargetHunter, ObjectPoint, EnemyObjectPullSpeed);
		UE_LOG(LogBreachborne, Log, TEXT("PowerAbility: BungeeShot pulled %s toward object %s"),
			*GetNameSafe(TargetHunter), *ObjectPoint.ToCompactString());
		return;
	}

	SpawnVisualCord(FirstPoint, SecondPoint, ObjectCordDuration, true);
	UE_LOG(LogBreachborne, Log, TEXT("PowerAbility: BungeeShot created object cord %s -> %s"),
		*FirstPoint.ToCompactString(), *SecondPoint.ToCompactString());
}

void UGA_Power_BungeeShot::PullHunterToward(AHunterCharacter* Hunter, const FVector& Target, float PullSpeed) const
{
	if (!Hunter)
	{
		return;
	}

	const FVector Direction = (Target - Hunter->GetActorLocation()).GetSafeNormal();
	Hunter->LaunchCharacter(Direction * PullSpeed + FVector(0.0f, 0.0f, 180.0f), true, true);
}

void UGA_Power_BungeeShot::ApplyStun(AHunterCharacter* Hunter) const
{
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = Hunter ? Hunter->GetAbilitySystemComponent() : nullptr;
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(GetHunterCharacter(), GetHunterCharacter());
	FGameplayEffectSpecHandle StunSpec = SourceASC->MakeOutgoingSpec(UBBStunTagEffect::StaticClass(), 1.0f, Context);
	if (StunSpec.IsValid())
	{
		StunSpec.Data->SetDuration(StunDuration, false);
		SourceASC->ApplyGameplayEffectSpecToTarget(*StunSpec.Data, TargetASC);
	}
}

void UGA_Power_BungeeShot::SpawnVisualCord(const FVector& Start, const FVector& End, float Duration, bool bElastic) const
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !Hunter->GetWorld())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = Hunter;
	Params.Instigator = Hunter;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (ABBBungeeCordActor* Cord = Hunter->GetWorld()->SpawnActor<ABBBungeeCordActor>(ABBBungeeCordActor::StaticClass(), Start, FRotator::ZeroRotator, Params))
	{
		Cord->InitCord(Hunter, Start, End, Duration, bElastic ? ObjectCordBounceSpeed : 0.0f);
		if (!bElastic)
		{
			Cord->SetActorEnableCollision(false);
		}
	}
}
