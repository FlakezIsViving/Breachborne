#include "BBVoidSnapProjectile.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBGlideBot.h"
#include "Breachborne/Combat/BBPrimitiveVisuals.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Combat/BBTargetDummy.h"
#include "Breachborne/Combat/BBTestAlly.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	bool ResolveVoidSnapTarget(AActor* Actor, int32 SourceTeamID, UAbilitySystemComponent*& OutASC)
	{
		OutASC = nullptr;
		int32 TargetTeamID = SourceTeamID;
		if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(Actor))
		{
			if (const ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>())
			{
				TargetTeamID = PS->GetTeamID();
			}
			OutASC = Hunter->GetAbilitySystemComponent();
		}
		else if (ABBTargetDummy* Dummy = Cast<ABBTargetDummy>(Actor))
		{
			TargetTeamID = Dummy->GetTeamID();
			OutASC = Dummy->GetAbilitySystemComponent();
		}
		else if (ABBGlideBot* GlideBot = Cast<ABBGlideBot>(Actor))
		{
			TargetTeamID = GlideBot->GetTeamID();
			OutASC = GlideBot->GetAbilitySystemComponent();
		}
		else if (ABBTestAlly* TestAlly = Cast<ABBTestAlly>(Actor))
		{
			TargetTeamID = TestAlly->GetTeamID();
			OutASC = TestAlly->GetAbilitySystemComponent();
		}
		return OutASC && TargetTeamID != SourceTeamID && !OutASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead);
	}
}

ABBVoidSnapProjectile::ABBVoidSnapProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	ConeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConeMesh"));
	ConeMesh->SetupAttachment(RootScene);
	ConeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Engine cone points along local +Z; -90 pitch maps its tip to actor-forward (+X).
	ConeMesh->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(TEXT("/Engine/BasicShapes/Cone"));
	if (ConeAsset.Succeeded())
	{
		ConeMesh->SetStaticMesh(ConeAsset.Object);
	}
	ConeMesh->SetRelativeScale3D(FVector(1.5f, 1.5f, 2.4f));
	BBPrimitiveVisuals::ApplyColor(ConeMesh, FLinearColor(0.8f, 0.05f, 1.0f, 1.0f));
}

void ABBVoidSnapProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBVoidSnapProjectile, bEmpowered);
}

void ABBVoidSnapProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(HasAuthority());
}

void ABBVoidSnapProjectile::InitSnap(AHunterCharacter* InSourceHunter, UAbilitySystemComponent* InSourceASC,
	TSubclassOf<UGameplayEffect> InDamageGE, TSubclassOf<UGameplayEffect> InStunGE,
	int32 InTeamID, const FVector& InDirection, bool bInEmpowered)
{
	SourceHunter = InSourceHunter;
	SourceASC = InSourceASC;
	DamageEffectClass = InDamageGE;
	StunEffectClass = InStunGE;
	SourceTeamID = InTeamID;
	Direction = InDirection.GetSafeNormal2D();
	bEmpowered = bInEmpowered;
	if (bEmpowered)
	{
		Speed *= 1.18f;
		ConeLength *= 1.25f;
		ConeRadius *= 1.25f;
		Damage *= 1.2f;
	}
	ApplySnapVisuals();
	SetActorRotation(Direction.Rotation());
	ForceNetUpdate();
}

void ABBVoidSnapProjectile::OnRep_Empowered()
{
	ApplySnapVisuals();
}

void ABBVoidSnapProjectile::ApplySnapVisuals()
{
	ConeMesh->SetRelativeScale3D(bEmpowered ? FVector(1.9f, 1.9f, 3.0f) : FVector(1.5f, 1.5f, 2.4f));
	BBPrimitiveVisuals::ApplyColor(ConeMesh, bEmpowered
		? FLinearColor(1.0f, 0.2f, 0.9f, 1.0f)
		: FLinearColor(0.8f, 0.05f, 1.0f, 1.0f));
}

void ABBVoidSnapProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		MoveAndHit(DeltaSeconds);
	}
}

void ABBVoidSnapProjectile::MoveAndHit(float DeltaSeconds)
{
	Elapsed += DeltaSeconds;
	SetActorLocation(GetActorLocation() + Direction * Speed * DeltaSeconds);
	TryHitTargets();
	if (Elapsed >= Lifetime || bHitTarget)
	{
		Destroy();
	}
}

void ABBVoidSnapProjectile::TryHitTargets()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VoidSnap), false, this);
	World->OverlapMultiByObjectType(Overlaps, GetActorLocation() - Direction * ConeLength * 0.5f, FQuat::Identity,
		ObjParams, FCollisionShape::MakeSphere(ConeLength), Params);

	AActor* BestActor = nullptr;
	UAbilitySystemComponent* BestASC = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Actor == GetOwner() || Actor == GetInstigator() || !IsActorInsideCone(Actor))
		{
			continue;
		}
		UAbilitySystemComponent* TargetASC = nullptr;
		if (!ResolveVoidSnapTarget(Actor, SourceTeamID, TargetASC))
		{
			continue;
		}
		const float Dist = FVector::DistSquared2D(Actor->GetActorLocation(), GetActorLocation());
		if (Dist < BestDistance)
		{
			BestDistance = Dist;
			BestActor = Actor;
			BestASC = TargetASC;
		}
	}

	if (BestActor && BestASC)
	{
		ApplyDamageAndStun(BestASC, BestActor);
		bHitTarget = true;
	}
}

bool ABBVoidSnapProjectile::IsActorInsideCone(AActor* Actor) const
{
	return Actor && IsLocationInsideCone(Actor->GetActorLocation());
}

bool ABBVoidSnapProjectile::IsLocationInsideCone(const FVector& Location) const
{
	const FVector ToActor = Location - GetActorLocation();
	const float ForwardDistance = FVector::DotProduct(ToActor.GetSafeNormal2D(), Direction) * ToActor.Size2D();
	if (ForwardDistance < 0.0f || ForwardDistance > ConeLength)
	{
		return false;
	}
	const FVector Closest = GetActorLocation() + Direction * ForwardDistance;
	const float AllowedRadius = FMath::Lerp(18.0f, ConeRadius, ForwardDistance / FMath::Max(1.0f, ConeLength));
	return FVector::Dist2D(Location, Closest) <= AllowedRadius;
}

void ABBVoidSnapProjectile::ApplyDamageAndStun(UAbilitySystemComponent* TargetASC, AActor* TargetActor)
{
	if (!SourceASC.IsValid() || !TargetASC)
	{
		return;
	}
	if (DamageEffectClass)
	{
		FGameplayEffectSpecHandle DamageSpec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
		if (DamageSpec.IsValid())
		{
			DamageSpec.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, Damage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
		}
	}
	if (StunEffectClass)
	{
		FGameplayEffectSpecHandle StunSpec = SourceASC->MakeOutgoingSpec(StunEffectClass, 1.0f, SourceASC->MakeEffectContext());
		if (StunSpec.IsValid())
		{
			StunSpec.Data->SetDuration(StunDuration, false);
			SourceASC->ApplyGameplayEffectSpecToTarget(*StunSpec.Data.Get(), TargetASC);
		}
	}
	FGameplayEventData EventData;
	EventData.Instigator = GetOwner();
	EventData.Target = TargetActor;
	SourceASC->HandleGameplayEvent(BBGameplayTags::Event_Void_DamageDealt, &EventData);

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = GetInstigator();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveBurstActor* Burst = GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
		ABBPrimitiveBurstActor::StaticClass(), TargetActor->GetActorLocation(), FRotator::ZeroRotator, Params))
	{
		Burst->InitBurst(TargetActor->GetActorLocation(), bEmpowered ? ConeRadius * 0.75f : ConeRadius * 0.55f,
			0.22f, bEmpowered ? FLinearColor(1.0f, 0.31f, 0.85f, 1.0f) : FLinearColor(0.55f, 0.36f, 0.96f, 1.0f));
	}
}
