#include "BBVoidSwapProjectile.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBTestAlly.h"
#include "Breachborne/Combat/BBPrimitiveVisuals.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Combat/BBVoidSwappableComponent.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Items/BBChestActor.h"
#include "Breachborne/Wisp/BBWispPawn.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABBVoidSwapProjectile::ABBVoidSwapProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);

	SwapSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SwapSphere"));
	SwapSphere->InitSphereRadius(Radius);
	SwapSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(SwapSphere);

	ZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneMesh"));
	ZoneMesh->SetupAttachment(SwapSphere);
	ZoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZoneMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(SwapSphere);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	ProjectileMesh->SetRelativeScale3D(FVector(0.2f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		ZoneMesh->SetStaticMesh(CylinderMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(SphereMesh.Object);
	}
	BBPrimitiveVisuals::ApplyColor(ZoneMesh, FLinearColor(0.0f, 1.0f, 0.65f, 1.0f));
	BBPrimitiveVisuals::ApplyColor(ProjectileMesh, FLinearColor(0.25f, 1.0f, 0.95f, 1.0f));
	RefreshVisuals();
}

void ABBVoidSwapProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBVoidSwapProjectile, LaunchLocation);
	DOREPLIFETIME(ABBVoidSwapProjectile, TargetLocation);
	DOREPLIFETIME(ABBVoidSwapProjectile, Radius);
	DOREPLIFETIME(ABBVoidSwapProjectile, bEmpowered);
}

void ABBVoidSwapProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(HasAuthority());
}

void ABBVoidSwapProjectile::InitSwapProjectile(AHunterCharacter* InSourceHunter, const FVector& InLaunchLocation, const FVector& InTargetLocation,
	float InTravelDuration, float InRadius, bool bInEmpowered)
{
	SourceHunter = InSourceHunter;
	LaunchLocation = InLaunchLocation;
	TargetLocation = InTargetLocation;
	TravelDuration = FMath::Max(0.1f, InTravelDuration);
	Radius = InRadius;
	bEmpowered = bInEmpowered;
	TravelArcHeight = bEmpowered ? 290.0f : 360.0f;
	SetActorLocation(LaunchLocation);
	if (bEmpowered)
	{
		BBPrimitiveVisuals::ApplyColor(ZoneMesh, FLinearColor(0.65f, 1.0f, 0.1f, 1.0f));
		BBPrimitiveVisuals::ApplyColor(ProjectileMesh, FLinearColor(0.85f, 1.0f, 0.15f, 1.0f));
	}
	RefreshVisuals();
	ForceNetUpdate();
}

void ABBVoidSwapProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		UpdateArc(DeltaSeconds);
	}
}

void ABBVoidSwapProjectile::OnRep_SwapVisuals()
{
	RefreshVisuals();
}

void ABBVoidSwapProjectile::RefreshVisuals()
{
	if (SwapSphere)
	{
		SwapSphere->SetSphereRadius(Radius, true);
	}
	if (ZoneMesh)
	{
		const float RadiusScale = Radius / 50.0f;
		ZoneMesh->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, 0.02f));
		BBPrimitiveVisuals::ApplyColor(ZoneMesh, bEmpowered
			? FLinearColor(0.65f, 1.0f, 0.1f, 1.0f)
			: FLinearColor(0.0f, 1.0f, 0.65f, 1.0f));
	}
	if (ProjectileMesh)
	{
		ProjectileMesh->SetRelativeScale3D(bEmpowered ? FVector(0.26f) : FVector(0.2f));
		BBPrimitiveVisuals::ApplyColor(ProjectileMesh, bEmpowered
			? FLinearColor(0.85f, 1.0f, 0.15f, 1.0f)
			: FLinearColor(0.25f, 1.0f, 0.95f, 1.0f));
	}
}

void ABBVoidSwapProjectile::UpdateArc(float DeltaSeconds)
{
	Elapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(Elapsed / FMath::Max(0.01f, TravelDuration), 0.0f, 1.0f);
	const FVector Current = GetArcPoint(Alpha);
	SetActorLocation(Current);
	if (HasAuthority() && Alpha >= 1.0f && !bSwapped)
	{
		ExecuteSwap();
		Destroy();
	}
}

FVector ABBVoidSwapProjectile::GetArcPoint(float Alpha) const
{
	const FVector Start = LaunchLocation;
	const FVector Target = TargetLocation;
	const FVector ControlA = Start + FVector(0.0f, 0.0f, TravelArcHeight);
	const FVector ControlB = Target + FVector(0.0f, 0.0f, TravelArcHeight);
	const float InvAlpha = 1.0f - Alpha;
	return (InvAlpha * InvAlpha * InvAlpha * Start)
		+ (3.0f * InvAlpha * InvAlpha * Alpha * ControlA)
		+ (3.0f * InvAlpha * Alpha * Alpha * ControlB)
		+ (Alpha * Alpha * Alpha * Target);
}

void ABBVoidSwapProjectile::ExecuteSwap()
{
	bSwapped = true;
	AHunterCharacter* Hunter = SourceHunter.Get();
	if (!Hunter)
	{
		return;
	}

	const FVector SourceCenter = Hunter->GetActorLocation();
	const FVector DestCenter = GetActorLocation();
	FActorSpawnParameters VisualParams;
	VisualParams.Owner = GetOwner();
	VisualParams.Instigator = GetInstigator();
	VisualParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (const FVector& Center : {SourceCenter, DestCenter})
	{
		if (ABBPrimitiveBurstActor* SwapBurst = GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
			ABBPrimitiveBurstActor::StaticClass(), Center, FRotator::ZeroRotator, VisualParams))
		{
			SwapBurst->InitBurst(Center, Radius, 0.3f,
				bEmpowered ? FLinearColor(1.0f, 0.31f, 0.85f, 1.0f) : FLinearColor(0.35f, 0.94f, 0.56f, 1.0f), true);
		}
	}
	TArray<AActor*> SourceActors;
	TArray<AActor*> DestActors;
	GatherEligibleActors(SourceCenter, SourceActors);
	GatherEligibleActors(DestCenter, DestActors);

	TSet<AActor*> SourceSet;
	for (AActor* Actor : SourceActors)
	{
		if (Actor)
		{
			SourceSet.Add(Actor);
		}
	}
	for (AActor* Actor : SourceActors)
	{
		if (!Actor)
		{
			continue;
		}
		const FVector Offset = Actor->GetActorLocation() - SourceCenter;
		TeleportActor(Actor, DestCenter + Offset);
	}
	for (AActor* Actor : DestActors)
	{
		if (!Actor || SourceSet.Contains(Actor))
		{
			continue;
		}
		const FVector Offset = Actor->GetActorLocation() - DestCenter;
		TeleportActor(Actor, SourceCenter + Offset);
	}
}

void ABBVoidSwapProjectile::GatherEligibleActors(const FVector& Center, TArray<AActor*>& OutActors) const
{
	if (!GetWorld())
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VoidSwap), false, this);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjParams, FCollisionShape::MakeSphere(Radius), Params);

	TSet<AActor*> Seen;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Actor == this || Seen.Contains(Actor))
		{
			continue;
		}
		Seen.Add(Actor);
		if (IsActorEligible(Actor))
		{
			OutActors.Add(Actor);
		}
		else if (const USceneComponent* Root = Actor->GetRootComponent(); Root && Root->Mobility == EComponentMobility::Static)
		{
			UE_LOG(LogTemp, Verbose, TEXT("Void Swap skipped static unmarked actor %s"), *GetNameSafe(Actor));
		}
	}
}

bool ABBVoidSwapProjectile::IsActorEligible(AActor* Actor) const
{
	if (!Actor || Actor == this)
	{
		return false;
	}
	if (const AHunterCharacter* Hunter = Cast<AHunterCharacter>(Actor))
	{
		const UAbilitySystemComponent* ASC = Hunter->GetAbilitySystemComponent();
		return ASC && !ASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead);
	}
	if (Cast<ABBWispPawn>(Actor) || Cast<ABBTestAlly>(Actor) || Cast<ABBChestActor>(Actor))
	{
		return true;
	}
	return Actor->FindComponentByClass<UBBVoidSwappableComponent>() != nullptr;
}

void ABBVoidSwapProjectile::TeleportActor(AActor* Actor, const FVector& NewLocation) const
{
	if (!Actor)
	{
		return;
	}
	if (ACharacter* Character = Cast<ACharacter>(Actor))
	{
		if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}
	}
	Actor->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
	Actor->ForceNetUpdate();
}
