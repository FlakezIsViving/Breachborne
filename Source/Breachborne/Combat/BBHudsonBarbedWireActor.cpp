#include "BBHudsonBarbedWireActor.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/GliderComponent.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBBarbedWireSlowEffect.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBGlideBot.h"
#include "Breachborne/Combat/BBGroundedEffect.h"
#include "Breachborne/Combat/BBPrimitiveVisuals.h"
#include "Breachborne/Combat/BBStunTagEffect.h"
#include "Breachborne/Combat/BBTargetDummy.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

ABBHudsonBarbedWireActor::ABBHudsonBarbedWireActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);

	WireBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("WireBounds"));
	WireBounds->SetBoxExtent(FVector(360.0f, 90.0f, 100.0f));
	WireBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WireBounds->SetCollisionObjectType(ECC_WorldDynamic);
	WireBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	WireBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(WireBounds);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	UStaticMesh* Cube = CubeMesh.Succeeded() ? CubeMesh.Object : nullptr;

	auto MakeBar = [this, Cube](const TCHAR* Name, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
	{
		UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Comp->SetupAttachment(WireBounds);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetRelativeLocation(Location);
		Comp->SetRelativeRotation(Rotation);
		Comp->SetRelativeScale3D(Scale);
		Comp->SetHiddenInGame(false);
		Comp->SetVisibility(true);
		if (Cube)
		{
			Comp->SetStaticMesh(Cube);
		}
		return Comp;
	};

	GroundPlate = MakeBar(TEXT("GroundPlate"), FVector(0.0f, 0.0f, -92.0f), FRotator::ZeroRotator, FVector(7.2f, 1.8f, 0.03f));
	BarA = MakeBar(TEXT("BarA"), FVector(0.0f, -36.0f, -20.0f), FRotator(0.0f, 0.0f, 28.0f), FVector(7.2f, 0.08f, 0.08f));
	BarB = MakeBar(TEXT("BarB"), FVector(0.0f, 36.0f, -20.0f), FRotator(0.0f, 0.0f, -28.0f), FVector(7.2f, 0.08f, 0.08f));
	BarC = MakeBar(TEXT("BarC"), FVector(0.0f, 0.0f, 24.0f), FRotator(0.0f, 0.0f, 18.0f), FVector(7.2f, 0.06f, 0.06f));
	BarD = MakeBar(TEXT("BarD"), FVector(0.0f, 0.0f, 48.0f), FRotator(0.0f, 0.0f, -18.0f), FVector(7.2f, 0.06f, 0.06f));
	BBPrimitiveVisuals::ApplyColor(GroundPlate, FLinearColor(0.31f, 0.64f, 0.78f, 1.0f));
	BBPrimitiveVisuals::ApplyColor(BarA, FLinearColor(1.0f, 0.54f, 0.16f, 1.0f));
	BBPrimitiveVisuals::ApplyColor(BarB, FLinearColor(1.0f, 0.54f, 0.16f, 1.0f));
	BBPrimitiveVisuals::ApplyColor(BarC, FLinearColor(1.0f, 0.82f, 0.4f, 1.0f));
	BBPrimitiveVisuals::ApplyColor(BarD, FLinearColor(1.0f, 0.82f, 0.4f, 1.0f));

	DamageEffectClass = UBBDamageEffect::StaticClass();
	SlowEffectClass = UBBBarbedWireSlowEffect::StaticClass();
	GroundedEffectClass = UBBGroundedEffect::StaticClass();
	StunEffectClass = UBBStunTagEffect::StaticClass();
}

void ABBHudsonBarbedWireActor::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(HasAuthority());

	if (HasAuthority())
	{
		SetLifeSpan(LifetimeSeconds);
	}
}

void ABBHudsonBarbedWireActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	ElapsedTickTime += DeltaSeconds;
	if (ElapsedTickTime >= TickInterval)
	{
		ElapsedTickTime = 0.0f;
		ApplyWireTick();
	}
}

void ABBHudsonBarbedWireActor::InitWire(UAbilitySystemComponent* InSourceASC, int32 InTeamID, float InDuration, float InDamagePerSecond)
{
	SourceASC = InSourceASC;
	SourceTeamID = InTeamID;
	LifetimeSeconds = FMath::Max(0.5f, InDuration);
	DamagePerSecond = FMath::Max(0.0f, InDamagePerSecond);

	if (HasAuthority())
	{
		SetLifeSpan(LifetimeSeconds);
	}
}

void ABBHudsonBarbedWireActor::ApplyWireTick()
{
	UAbilitySystemComponent* Source = SourceASC.Get();
	if (!Source)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	WireBounds->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		UAbilitySystemComponent* TargetASC = nullptr;
		int32 TargetTeamID = SourceTeamID;
		if (!ResolveTarget(Actor, TargetASC, TargetTeamID) || !TargetASC || TargetTeamID == SourceTeamID)
		{
			continue;
		}

		FGameplayEffectContextHandle Context = Source->MakeEffectContext();
		Context.AddInstigator(GetOwner(), GetOwner());

		if (SlowEffectClass)
		{
			if (FActiveGameplayEffectHandle* ExistingSlowHandle = ActiveSlowHandlesByActor.Find(Actor))
			{
				if (ExistingSlowHandle->IsValid())
				{
					TargetASC->RemoveActiveGameplayEffect(*ExistingSlowHandle);
				}
			}

			FGameplayEffectSpecHandle SlowSpec = Source->MakeOutgoingSpec(SlowEffectClass, 1.0f, Context);
			if (SlowSpec.IsValid())
			{
				SlowSpec.Data->SetDuration(SlowDuration, false);
				const FActiveGameplayEffectHandle NewSlowHandle = Source->ApplyGameplayEffectSpecToTarget(*SlowSpec.Data, TargetASC);
				if (NewSlowHandle.IsValid())
				{
					ActiveSlowHandlesByActor.Add(Actor, NewSlowHandle);
				}
			}
		}

		AHunterCharacter* TargetHunter = Cast<AHunterCharacter>(Actor);
		const bool bAirborneOrGliding =
			TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Gliding)
			|| (TargetHunter
				&& TargetHunter->GetCharacterMovement()
				&& TargetHunter->GetCharacterMovement()->IsFalling());

		if (bAirborneOrGliding && GroundedEffectClass)
		{
			if (TargetHunter && TargetHunter->GetGliderComponent())
			{
				TargetHunter->GetGliderComponent()->ForceCloseGlider();
			}

			FGameplayEffectSpecHandle GroundSpec = Source->MakeOutgoingSpec(GroundedEffectClass, 1.0f, Context);
			if (GroundSpec.IsValid())
			{
				GroundSpec.Data->SetDuration(AirborneStunDuration, false);
				Source->ApplyGameplayEffectSpecToTarget(*GroundSpec.Data, TargetASC);
			}
		}

		if (bAirborneOrGliding && StunEffectClass)
		{
			const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
			const float* LastStunTime = LastAirborneStunTimeByActor.Find(Actor);
			if (!LastStunTime || Now - *LastStunTime >= AirborneStunReapplyDelay)
			{
				FGameplayEffectSpecHandle StunSpec = Source->MakeOutgoingSpec(StunEffectClass, 1.0f, Context);
				if (StunSpec.IsValid())
				{
					StunSpec.Data->SetDuration(AirborneStunDuration, false);
					Source->ApplyGameplayEffectSpecToTarget(*StunSpec.Data, TargetASC);
				}
				LastAirborneStunTimeByActor.Add(Actor, Now);
			}
		}

		if (DamageEffectClass && DamagePerSecond > 0.0f)
		{
			FGameplayEffectSpecHandle DamageSpec = Source->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
			if (DamageSpec.IsValid())
			{
				DamageSpec.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, DamagePerSecond * TickInterval);
				Source->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data, TargetASC);
			}
		}

	}
}

bool ABBHudsonBarbedWireActor::ResolveTarget(AActor* Actor, UAbilitySystemComponent*& OutASC, int32& OutTeamID) const
{
	if (!Actor)
	{
		return false;
	}

	if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(Actor))
	{
		const ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>();
		OutASC = Hunter->GetAbilitySystemComponent();
		OutTeamID = PS ? PS->GetTeamID() : SourceTeamID;
		return OutASC != nullptr;
	}

	if (ABBTargetDummy* Dummy = Cast<ABBTargetDummy>(Actor))
	{
		OutASC = Dummy->GetAbilitySystemComponent();
		OutTeamID = Dummy->GetTeamID();
		return OutASC != nullptr;
	}

	if (ABBGlideBot* GlideBot = Cast<ABBGlideBot>(Actor))
	{
		OutASC = GlideBot->GetAbilitySystemComponent();
		OutTeamID = GlideBot->GetTeamID();
		return OutASC != nullptr;
	}

	return false;
}
