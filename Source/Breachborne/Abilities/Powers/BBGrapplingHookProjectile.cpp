#include "BBGrapplingHookProjectile.h"

#include "Breachborne/Breachborne.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABBGrapplingHookProjectile::ABBGrapplingHookProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(22.0f);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComp->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComp->SetNotifyRigidBodyCollision(true);
	SetRootComponent(CollisionComp);

	HookMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HookMesh"));
	HookMesh->SetupAttachment(CollisionComp);
	HookMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HookMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.8f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone"));
	if (ConeMesh.Succeeded())
	{
		HookMesh->SetStaticMesh(ConeMesh.Object);
	}

	ChainMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChainMesh"));
	ChainMesh->SetupAttachment(RootComponent);
	ChainMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		ChainMesh->SetStaticMesh(CylinderMesh.Object);
	}
	ChainMesh->SetRelativeScale3D(FVector(0.035f, 0.035f, 0.01f));

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 3800.0f;
	ProjectileMovement->MaxSpeed = 3800.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;

	InitialLifeSpan = 3.0f;
}

void ABBGrapplingHookProjectile::BeginPlay()
{
	Super::BeginPlay();

	OwnerHunter = Cast<AHunterCharacter>(GetOwner());
	if (HasAuthority())
	{
		StartLocation = GetActorLocation();
		CollisionComp->OnComponentHit.AddDynamic(this, &ABBGrapplingHookProjectile::OnHookHit);
		CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &ABBGrapplingHookProjectile::OnHookOverlap);
	}
	else
	{
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ABBGrapplingHookProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBGrapplingHookProjectile, AttachedActor);
	DOREPLIFETIME(ABBGrapplingHookProjectile, AttachedLocation);
	DOREPLIFETIME(ABBGrapplingHookProjectile, bAttached);
}

void ABBGrapplingHookProjectile::InitGrapple(AHunterCharacter* InOwnerHunter, const FVector& Direction, float InRange, float InInitialPullSpeed, float InMaxPullSpeed, float InStopDistance, float InMaxPullDuration, float InPullTickInterval)
{
	OwnerHunter = InOwnerHunter;
	Range = InRange;
	InitialPullSpeed = InInitialPullSpeed;
	MaxPullSpeed = InMaxPullSpeed;
	StopDistance = InStopDistance;
	MaxPullDuration = InMaxPullDuration;
	PullTickInterval = InPullTickInterval;

	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = Direction.GetSafeNormal() * ProjectileMovement->InitialSpeed;
	}
}

void ABBGrapplingHookProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateChainVisual();
	if (!HasAuthority())
	{
		return;
	}

	if (!bAttached && FVector::DistSquared(StartLocation, GetActorLocation()) >= FMath::Square(Range))
	{
		Destroy();
		return;
	}

	if (bAttached && OwnerHunter.IsValid())
	{
		const FVector EndPoint = IsValid(AttachedActor) ? AttachedActor->GetActorLocation() : FVector(AttachedLocation);
		if (FVector::DistSquared(OwnerHunter->GetActorLocation(), EndPoint) <= FMath::Square(StopDistance + 50.0f))
		{
			Destroy();
		}
	}
}

void ABBGrapplingHookProjectile::OnHookHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || bAttached || OtherActor == OwnerHunter.Get())
	{
		return;
	}

	AttachHook(Hit);
}

void ABBGrapplingHookProjectile::OnHookOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bAttached || !OtherActor || OtherActor == OwnerHunter.Get())
	{
		return;
	}

	AttachHook(SweepResult);
}

void ABBGrapplingHookProjectile::AttachHook(const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}

	bAttached = true;
	AttachedActor = Hit.GetActor();
	AttachedLocation = Hit.ImpactPoint.IsNearlyZero() ? GetActorLocation() : FVector(Hit.ImpactPoint);
	SetActorLocation(AttachedLocation);

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetLifeSpan(MaxPullDuration + 0.35f);
	ForceNetUpdate();

	if (OwnerHunter.IsValid())
	{
		OwnerHunter->BeginGrapplePull(AttachedActor, AttachedLocation, InitialPullSpeed, MaxPullSpeed, StopDistance, MaxPullDuration, PullTickInterval);
		UE_LOG(LogBreachborne, Log, TEXT("PowerAbility: GrapplingHook latched onto %s"),
			IsValid(AttachedActor) ? *AttachedActor->GetName() : *FVector(AttachedLocation).ToCompactString());
	}
}

void ABBGrapplingHookProjectile::OnRep_AttachmentState()
{
	if (!bAttached)
	{
		return;
	}

	SetActorLocation(FVector(AttachedLocation));
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UpdateChainVisual();
}

void ABBGrapplingHookProjectile::UpdateChainVisual()
{
	if (!OwnerHunter.IsValid())
	{
		OwnerHunter = Cast<AHunterCharacter>(GetOwner());
	}
	if (!OwnerHunter.IsValid() || !ChainMesh)
	{
		return;
	}

	const FVector Start = OwnerHunter->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
	const FVector End = bAttached && IsValid(AttachedActor) ? AttachedActor->GetActorLocation() : GetActorLocation();
	const FVector Mid = (Start + End) * 0.5f;
	const FVector Delta = End - Start;
	const float Length = Delta.Size();
	if (Length <= KINDA_SMALL_NUMBER)
	{
		ChainMesh->SetVisibility(false);
		return;
	}

	ChainMesh->SetVisibility(true);
	ChainMesh->SetWorldLocation(Mid);
	ChainMesh->SetWorldRotation(FRotationMatrix::MakeFromZ(Delta).Rotator());
	ChainMesh->SetWorldScale3D(FVector(0.035f, 0.035f, Length / 100.0f));
}
