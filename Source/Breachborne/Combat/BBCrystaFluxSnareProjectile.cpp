#include "BBCrystaFluxSnareProjectile.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBGlideBot.h"
#include "Breachborne/Combat/BBPrimitiveVisuals.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Combat/BBTargetDummy.h"
#include "Breachborne/Combat/BBTestAlly.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	bool ResolveFluxTarget(AActor* Actor, int32 SourceTeamID, UAbilitySystemComponent*& OutASC)
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

ABBCrystaFluxSnareProjectile::ABBCrystaFluxSnareProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(34.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionSphere);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionSphere);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetRelativeScale3D(FVector(0.22f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(SphereMesh.Object);
	}
	BBPrimitiveVisuals::ApplyColor(ProjectileMesh, FLinearColor(0.0f, 0.85f, 1.0f, 1.0f));

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABBCrystaFluxSnareProjectile::OnOverlap);
}

void ABBCrystaFluxSnareProjectile::BeginPlay()
{
	Super::BeginPlay();
	SpawnLocation = GetActorLocation();
	SetActorTickEnabled(HasAuthority());
}

void ABBCrystaFluxSnareProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBCrystaFluxSnareProjectile, bReturning);
}

void ABBCrystaFluxSnareProjectile::InitFluxSnare(AHunterCharacter* InSourceHunter, UAbilitySystemComponent* InSourceASC,
	TSubclassOf<UGameplayEffect> InDamageGE, TSubclassOf<UGameplayEffect> InMarkGE,
	TSubclassOf<UGameplayEffect> InGroundedGE, int32 InTeamID, const FVector& InInitialDirection)
{
	SourceHunter = InSourceHunter;
	SourceASC = InSourceASC;
	DamageEffectClass = InDamageGE;
	MarkEffectClass = InMarkGE;
	GroundedEffectClass = InGroundedGE;
	SourceTeamID = InTeamID;
	CurrentDirection = InInitialDirection.GetSafeNormal2D();
	SpawnLocation = GetActorLocation();
}

void ABBCrystaFluxSnareProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		MoveProjectile(DeltaSeconds);
	}
}

void ABBCrystaFluxSnareProjectile::StartReturn()
{
	bReturning = true;
	ApplyLegVisuals();
	ForceNetUpdate();
}

void ABBCrystaFluxSnareProjectile::OnRep_Returning()
{
	ApplyLegVisuals();
}

void ABBCrystaFluxSnareProjectile::ApplyLegVisuals()
{
	if (!ProjectileMesh)
	{
		return;
	}

	ProjectileMesh->SetRelativeScale3D(bReturning ? FVector(0.3f) : FVector(0.22f));
	BBPrimitiveVisuals::ApplyColor(ProjectileMesh,
		bReturning
			? FLinearColor(1.0f, 0.3f, 0.05f, 1.0f)
			: FLinearColor(0.0f, 0.85f, 1.0f, 1.0f));
}

void ABBCrystaFluxSnareProjectile::MoveProjectile(float DeltaSeconds)
{
	ElapsedTime += DeltaSeconds;
	AHunterCharacter* Hunter = SourceHunter.Get();
	if (!Hunter)
	{
		Destroy();
		return;
	}

	if (!bReturning)
	{
		const FVector AimDir = (Hunter->GetAimLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!AimDir.IsNearlyZero())
		{
			CurrentDirection = FMath::VInterpNormalRotationTo(CurrentDirection, AimDir, DeltaSeconds, SteeringStrength).GetSafeNormal2D();
		}
		if (FVector::Dist2D(SpawnLocation, GetActorLocation()) >= MaxRange || ElapsedTime >= MaxLifetime)
		{
			StartReturn();
		}
	}
	else
	{
		CurrentDirection = (Hunter->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (FVector::Dist2D(Hunter->GetActorLocation(), GetActorLocation()) <= ReturnEndDistance)
		{
			Destroy();
			return;
		}
	}

	const FVector Delta = CurrentDirection * (bReturning ? ReturnSpeed : OutboundSpeed) * DeltaSeconds;
	FHitResult Hit;
	SetActorLocation(GetActorLocation() + Delta, true, &Hit, ETeleportType::None);
	SetActorRotation(CurrentDirection.Rotation());
	if (Hit.bBlockingHit && !bReturning)
	{
		StartReturn();
	}
}

void ABBCrystaFluxSnareProjectile::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority())
	{
		ProcessHit(OtherActor);
	}
}

void ABBCrystaFluxSnareProjectile::ProcessHit(AActor* Actor)
{
	if (!Actor || Actor == GetOwner() || Actor == GetInstigator())
	{
		return;
	}

	TWeakObjectPtr<AActor> WeakActor(Actor);
	TSet<TWeakObjectPtr<AActor>>& HitSet = bReturning ? ReturnHits : OutboundHits;
	if (HitSet.Contains(WeakActor))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = nullptr;
	if (!ResolveFluxTarget(Actor, SourceTeamID, TargetASC))
	{
		return;
	}

	HitSet.Add(WeakActor);
	ApplyDamage(TargetASC, bReturning ? ReturnDamage : OutboundDamage);
	if (bReturning)
	{
		ApplyEffect(TargetASC, GroundedEffectClass, GroundedDuration);
		PullTarget(Actor);
	}
	else
	{
		ApplyEffect(TargetASC, MarkEffectClass, 4.0f);
	}
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = GetInstigator();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveBurstActor* Burst = GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
		ABBPrimitiveBurstActor::StaticClass(), Actor->GetActorLocation(), FRotator::ZeroRotator, Params))
	{
		Burst->InitBurst(Actor->GetActorLocation(), bReturning ? 85.0f : 55.0f, 0.2f,
			bReturning ? FLinearColor(1.0f, 0.3f, 0.05f, 1.0f) : FLinearColor(0.13f, 0.83f, 0.93f, 1.0f));
	}
}

void ABBCrystaFluxSnareProjectile::ApplyDamage(UAbilitySystemComponent* TargetASC, float Damage) const
{
	if (!SourceASC.IsValid() || !TargetASC || !DamageEffectClass || Damage <= 0.0f)
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, Damage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void ABBCrystaFluxSnareProjectile::ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, float Duration) const
{
	if (!SourceASC.IsValid() || !TargetASC || !EffectClass)
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetDuration(Duration, false);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void ABBCrystaFluxSnareProjectile::PullTarget(AActor* Actor) const
{
	const AHunterCharacter* Hunter = SourceHunter.Get();
	ACharacter* TargetCharacter = Cast<ACharacter>(Actor);
	if (!Hunter || !TargetCharacter)
	{
		return;
	}

	const FVector PullDir = (Hunter->GetActorLocation() - TargetCharacter->GetActorLocation()).GetSafeNormal2D();
	if (UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
	{
		MoveComp->Velocity += PullDir * PullImpulse;
		MoveComp->ForceReplicationUpdate();
	}
}
