#include "BBElunaStickyProjectile.h"
#include "BBElunaRootZone.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Combat/BBPrimitiveVisuals.h"
#include "Breachborne/Breachborne.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABBElunaStickyProjectile::ABBElunaStickyProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	ProjectileMesh->SetRelativeScale3D(FVector(0.5f));

	BBPrimitiveVisuals::ApplyColor(ProjectileMesh, FLinearColor(0.18f, 0.26f, 0.48f, 1.0f));

	ProjectileSpeed = 800.0f;
	ProjectileLifetime = 3.0f;
	RootZoneClass = ABBElunaRootZone::StaticClass();

	// Re-bind delegates to derived class methods (base constructor bound to base class methods)
	if (CollisionSphere)
	{
		CollisionSphere->SetSphereRadius(32.0f);
		CollisionSphere->OnComponentBeginOverlap.Clear();
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABBElunaStickyProjectile::OnProjectileOverlap);
		CollisionSphere->OnComponentHit.Clear();
		CollisionSphere->OnComponentHit.AddDynamic(this, &ABBElunaStickyProjectile::OnProjectileHit);
	}
}

void ABBElunaStickyProjectile::BeginPlay()
{
	Super::BeginPlay();
	PreviousSweepLocation = GetActorLocation();
	SetActorTickEnabled(HasAuthority());

	// The server simulates this projectile. Simulating ProjectileMovement again on
	// proxies makes replicated position corrections look like repeated reversals.
	if (!HasAuthority() && ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
		ProjectileMovement->SetUpdatedComponent(nullptr);
	}
}

void ABBElunaStickyProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBElunaStickyProjectile, StuckTarget);
	DOREPLIFETIME(ABBElunaStickyProjectile, bHasStuck);
	DOREPLIFETIME(ABBElunaStickyProjectile, StuckOffset);
}

void ABBElunaStickyProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	(void)DeltaSeconds;

	if (!HasAuthority() || bHasStuck || bHasExploded)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	if (!PreviousSweepLocation.Equals(CurrentLocation, 1.0f))
	{
		TArray<FHitResult> Hits;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ElunaStickyProjectileSweep), false, this);
		Params.AddIgnoredActor(GetOwner());
		Params.AddIgnoredActor(GetInstigator());
		FCollisionObjectQueryParams Objects;
		Objects.AddObjectTypesToQuery(ECC_Pawn);

		const float SweepRadius = CollisionSphere ? CollisionSphere->GetScaledSphereRadius() : 32.0f;
		if (GetWorld()->SweepMultiByObjectType(Hits, PreviousSweepLocation, CurrentLocation,
			FQuat::Identity, Objects, FCollisionShape::MakeSphere(SweepRadius), Params))
		{
			Hits.Sort([](const FHitResult& A, const FHitResult& B)
			{
				return A.Time < B.Time;
			});

			for (const FHitResult& Hit : Hits)
			{
				AHunterCharacter* HitHunter = Cast<AHunterCharacter>(Hit.GetActor());
				const ABreachbornePlayerState* TargetPS = HitHunter
					? HitHunter->GetPlayerState<ABreachbornePlayerState>()
					: nullptr;
				if (TargetPS && TargetPS->GetIsAlive() && TargetPS->GetTeamID() != SourceTeamID)
				{
					const FVector ContactLocation = Hit.Location.IsNearlyZero()
						? HitHunter->GetActorLocation()
						: FVector(Hit.Location);
					SetActorLocation(ContactLocation);
					StickToActor(HitHunter);
					return;
				}
			}
		}
	}

	PreviousSweepLocation = CurrentLocation;
}

void ABBElunaStickyProjectile::InitProjectile(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageGE, float InDamage, int32 InTeamID)
{
	Super::InitProjectile(InSourceASC, InDamageGE, InDamage, InTeamID);
}

void ABBElunaStickyProjectile::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || bHasStuck || bHasExploded)
	{
		return;
	}

	if (OtherActor && OtherActor != GetOwner() && OtherActor != GetInstigator())
	{
		// Check if it's an enemy hunter
		bool bIsEnemy = false;
		if (AHunterCharacter* HitHunter = Cast<AHunterCharacter>(OtherActor))
		{
			if (const ABreachbornePlayerState* TargetPS = HitHunter->GetPlayerState<ABreachbornePlayerState>())
			{
				bIsEnemy = (TargetPS->GetTeamID() != SourceTeamID);
			}
		}

		if (bIsEnemy)
		{
			StickToActor(OtherActor);
			return;
		}
	}

	// Hit wall or non-enemy — stick to ground at hit location
	StickToActor(nullptr);
	SetActorLocation(Hit.ImpactPoint);
}

void ABBElunaStickyProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bHasStuck || bHasExploded)
	{
		return;
	}

	if (OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	// Check if enemy
	bool bIsEnemy = false;
	if (AHunterCharacter* HitHunter = Cast<AHunterCharacter>(OtherActor))
	{
		if (const ABreachbornePlayerState* TargetPS = HitHunter->GetPlayerState<ABreachbornePlayerState>())
		{
			bIsEnemy = (TargetPS->GetTeamID() != SourceTeamID);
		}
	}

	if (bIsEnemy)
	{
		StickToActor(OtherActor);
	}
}

void ABBElunaStickyProjectile::StickToActor(AActor* TargetActor)
{
	bHasStuck = true;
	StuckTarget = TargetActor;

	if (TargetActor)
	{
		StuckOffset = GetActorLocation() - TargetActor->GetActorLocation();
		AttachToActor(TargetActor, FAttachmentTransformRules::KeepWorldTransform);
		SetReplicateMovement(false);
		ForceNetUpdate();
	}
	else
	{
		StuckOffset = GetActorLocation();
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
		ProjectileMovement->SetUpdatedComponent(nullptr);
	}

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	GetWorldTimerManager().SetTimer(
		ExplosionTimerHandle,
		this,
		&ABBElunaStickyProjectile::Explode,
		ExplosionDelay,
		false
	);
}

void ABBElunaStickyProjectile::OnRep_StuckState()
{
	if (!bHasStuck)
	{
		return;
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
		ProjectileMovement->SetUpdatedComponent(nullptr);
	}
	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (StuckTarget)
	{
		SetActorLocation(StuckTarget->GetActorLocation() + StuckOffset);
		AttachToActor(StuckTarget, FAttachmentTransformRules::KeepWorldTransform);
	}
	else
	{
		SetActorLocation(StuckOffset);
	}
}

void ABBElunaStickyProjectile::Explode()
{
	if (bHasExploded)
	{
		return;
	}
	bHasExploded = true;

	if (IsValid(StuckTarget))
	{
		SetActorLocation(StuckTarget->GetActorLocation() + StuckOffset);
	}

	if (RootZoneClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = GetInstigator();

		ABBElunaRootZone* RootZone = GetWorld()->SpawnActor<ABBElunaRootZone>(
			RootZoneClass, GetActorLocation(), FRotator::ZeroRotator, Params);

		if (RootZone)
		{
			RootZone->InitZone(SourceASC.Get(), DamageEffectClass, BaseDamage, SourceTeamID, RootRadius, RootDuration);
		}
		else
		{
		}
	}

	if (SourceASC.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Instigator = GetInstigator();
		CueParams.EffectCauser = this;
		CueParams.Location = GetActorLocation();
		CueParams.Normal = FVector::UpVector;
		SourceASC->ExecuteGameplayCue(BBGameplayTags::GameplayCue_Hunter_Eluna_RMB_Impact, CueParams);
	}

	FActorSpawnParameters BurstParams;
	BurstParams.Owner = GetOwner();
	BurstParams.Instigator = GetInstigator();
	BurstParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveBurstActor* Burst = GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
		ABBPrimitiveBurstActor::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, BurstParams))
	{
		Burst->InitBurst(GetActorLocation(), RootRadius, 0.22f, FLinearColor(0.43f, 0.78f, 1.0f, 1.0f), true);
	}

	Destroy();
}
