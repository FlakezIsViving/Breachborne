#include "BBProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBTargetDummy.h"
#include "Breachborne/Combat/BBGlideBot.h"
#include "Breachborne/Breachborne.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABBProjectile::ABBProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(true);

	// --- Collision sphere (root) ---
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(CollisionRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	// --- Placeholder visual mesh ---
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionSphere);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetRelativeScale3D(FVector(0.15f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(SphereMesh.Object);
	}

	// --- Projectile movement ---
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	// Bind overlap and hit events
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABBProjectile::OnProjectileOverlap);
	CollisionSphere->OnComponentHit.AddDynamic(this, &ABBProjectile::OnProjectileHit);
}

void ABBProjectile::InitProjectile(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageGE, float InDamage, int32 InTeamID)
{
	SourceASC = InSourceASC;
	DamageEffectClass = InDamageGE;
	BaseDamage = InDamage;
	SourceTeamID = InTeamID;

	UE_LOG(LogBreachborne, Verbose, TEXT("BBProjectile [%s]: Init — Damage=%.1f, TeamID=%d, Speed=%.0f, Lifetime=%.1fs, Piercing=%s"),
		*GetName(), BaseDamage, SourceTeamID, ProjectileSpeed, ProjectileLifetime, bPiercing ? TEXT("YES") : TEXT("NO"));

	// Start lifetime timer (server only — server will Destroy which replicates)
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			LifetimeTimerHandle,
			this,
			&ABBProjectile::OnLifetimeExpired,
			ProjectileLifetime,
			false
		);
	}
}

void ABBProjectile::FireInDirection(const FVector& Direction)
{
	ProjectileMovement->Velocity = Direction.GetSafeNormal() * ProjectileSpeed;
}

void ABBProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Damage only on server
	if (!HasAuthority())
	{
		return;
	}

	// Skip the hunter that fired this projectile
	if (OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	// Prevent double-hits on the same actor
	TWeakObjectPtr<AActor> WeakActor(OtherActor);
	if (HitActors.Contains(WeakActor))
	{
		return;
	}

	// Resolve target ASC and team — supports both HunterCharacter and TargetDummy
	UAbilitySystemComponent* TargetASC = nullptr;
	int32 TargetTeamID = SourceTeamID; // default = same team → skip

	if (AHunterCharacter* HitHunter = Cast<AHunterCharacter>(OtherActor))
	{
		if (const ABreachbornePlayerState* TargetPS = HitHunter->GetPlayerState<ABreachbornePlayerState>())
		{
			TargetTeamID = TargetPS->GetTeamID();
		}
		TargetASC = HitHunter->GetAbilitySystemComponent();
	}
	else if (ABBTargetDummy* Dummy = Cast<ABBTargetDummy>(OtherActor))
	{
		TargetTeamID = Dummy->GetTeamID();
		TargetASC = Dummy->GetAbilitySystemComponent();
	}
	else if (ABBGlideBot* GlideBot = Cast<ABBGlideBot>(OtherActor))
	{
		TargetTeamID = GlideBot->GetTeamID();
		TargetASC = GlideBot->GetAbilitySystemComponent();
	}

	// Team check — skip same team or unresolved target
	if (TargetTeamID == SourceTeamID || !TargetASC)
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("BBProjectile: Overlap with %s — skipped (same team %d or no ASC)"),
			*OtherActor->GetName(), TargetTeamID);
		return;
	}

	HitActors.Add(WeakActor);

	UE_LOG(LogBreachborne, Verbose, TEXT("BBProjectile: Hit %s (Team %d) — applying %.1f base damage via GAS pipeline"),
		*OtherActor->GetName(), TargetTeamID, BaseDamage);

	// Apply damage via GAS pipeline
	if (SourceASC.IsValid() && DamageEffectClass)
	{
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1, SourceASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, BaseDamage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

			UE_LOG(LogBreachborne, Verbose, TEXT("BBProjectile: GE applied successfully to %s"), *OtherActor->GetName());
		}
		else
		{
			UE_LOG(LogBreachborne, Warning, TEXT("BBProjectile: MakeOutgoingSpec FAILED — DamageEffectClass=%s"),
				*DamageEffectClass->GetName());
		}
	}
	else
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BBProjectile: Cannot apply damage — SourceASC valid=%s, DamageEffectClass=%s"),
			SourceASC.IsValid() ? TEXT("YES") : TEXT("NO"),
			DamageEffectClass ? TEXT("valid") : TEXT("NULL"));
	}

	if (!bPiercing)
	{
		Destroy();
	}
}

void ABBProjectile::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Hit world static (wall) — destroy on server, replicates destruction
	if (HasAuthority())
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("BBProjectile: Hit wall, destroying"));
		Destroy();
	}
}

void ABBProjectile::OnLifetimeExpired()
{
	UE_LOG(LogBreachborne, Verbose, TEXT("BBProjectile: Lifetime expired, destroying"));
	Destroy();
}
