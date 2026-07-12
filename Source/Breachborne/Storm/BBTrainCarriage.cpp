#include "BBTrainCarriage.h"
#include "BBTrainTrack.h"
#include "BBTrainChestComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Core/BreachborneGameMode.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "UObject/ConstructorHelpers.h"
#include "Breachborne/Breachborne.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/OverlapResult.h"

ABBTrainCarriage::ABBTrainCarriage()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bReplicates = true;

	CarriageMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarriageMesh"));
	SetRootComponent(CarriageMesh);
	CarriageMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // Visual only, no collision response at all
	CarriageMesh->SetCollisionObjectType(ECC_WorldDynamic);
	CarriageMesh->SetCollisionResponseToAllChannels(ECR_Ignore); // Completely invisible to all queries
	CarriageMesh->SetCanEverAffectNavigation(false);
	CarriageMesh->SetIsReplicated(true);

	// Assign default primitive meshes so carriages are visible out of the box
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	if (CubeMesh.Succeeded())
	{
		DefaultCubeMesh = CubeMesh.Object;
	}
	if (CylinderMesh.Succeeded())
	{
		DefaultCylinderMesh = CylinderMesh.Object;
	}
	if (SphereMesh.Succeeded())
	{
		DefaultSphereMesh = SphereMesh.Object;
	}

	// Default scale: flat walkable platform (length × width × thickness)
	CarriageMesh->SetRelativeScale3D(FVector(4.0f, 8.0f, 0.3f));

	SideCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("SideCollision"));
	SideCollision->SetupAttachment(CarriageMesh);
	SideCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // Query-only: CMC sweeps detect floor, zero physics fighting
	SideCollision->SetCollisionObjectType(ECC_WorldDynamic);
	SideCollision->SetCollisionResponseToAllChannels(ECR_Block);
	SideCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	SideCollision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	SideCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block); // Main floor collision for standing on train
	SideCollision->SetAbsolute(false, false, true); // CRITICAL: don't inherit mesh's non-uniform scale
	SideCollision->SetBoxExtent(FVector(200.0f, 400.0f, 50.0f)); // Tall enough for FindFloor to hit (platform top to above capsule center)
	SideCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f)); // Centered so bottom is at platform top (15cm) and top is at 115cm

	FrontKillZone = CreateDefaultSubobject<UBoxComponent>(TEXT("FrontKillZone"));
	FrontKillZone->SetupAttachment(CarriageMesh);
	FrontKillZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FrontKillZone->SetCollisionObjectType(ECC_WorldDynamic);
	FrontKillZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	FrontKillZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	FrontKillZone->SetGenerateOverlapEvents(true);
	FrontKillZone->SetBoxExtent(FVector(50.0f, 200.0f, 50.0f));
	FrontKillZone->SetRelativeLocation(FVector(400.0f, 0.0f, 0.0f));

	// Locomotive thin walls — no solid interior, just vertical slabs
	LocomotiveLeftWall = CreateDefaultSubobject<UBoxComponent>(TEXT("LocomotiveLeftWall"));
	LocomotiveLeftWall->SetupAttachment(CarriageMesh);
	LocomotiveLeftWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LocomotiveLeftWall->SetCollisionObjectType(ECC_WorldDynamic);
	LocomotiveLeftWall->SetCollisionResponseToAllChannels(ECR_Block);
	LocomotiveLeftWall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	LocomotiveLeftWall->SetAbsolute(false, false, true);
	LocomotiveLeftWall->SetBoxExtent(FVector(200.0f, 5.0f, 50.0f));

	LocomotiveRightWall = CreateDefaultSubobject<UBoxComponent>(TEXT("LocomotiveRightWall"));
	LocomotiveRightWall->SetupAttachment(CarriageMesh);
	LocomotiveRightWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LocomotiveRightWall->SetCollisionObjectType(ECC_WorldDynamic);
	LocomotiveRightWall->SetCollisionResponseToAllChannels(ECR_Block);
	LocomotiveRightWall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	LocomotiveRightWall->SetAbsolute(false, false, true);
	LocomotiveRightWall->SetBoxExtent(FVector(200.0f, 5.0f, 50.0f));

	LocomotiveBackWall = CreateDefaultSubobject<UBoxComponent>(TEXT("LocomotiveBackWall"));
	LocomotiveBackWall->SetupAttachment(CarriageMesh);
	LocomotiveBackWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LocomotiveBackWall->SetCollisionObjectType(ECC_WorldDynamic);
	LocomotiveBackWall->SetCollisionResponseToAllChannels(ECR_Block);
	LocomotiveBackWall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	LocomotiveBackWall->SetAbsolute(false, false, true);
	LocomotiveBackWall->SetBoxExtent(FVector(5.0f, 200.0f, 50.0f));

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetupAttachment(CarriageMesh);
	InteractSphere->SetSphereRadius(InteractRadius);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	ChestComponent = CreateDefaultSubobject<UBBTrainChestComponent>(TEXT("ChestComponent"));
}

void ABBTrainCarriage::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(HasAuthority());

	if (HasAuthority())
	{
		FrontKillZone->OnComponentBeginOverlap.AddDynamic(this, &ABBTrainCarriage::OnFrontKillZoneOverlap);
		InteractSphere->OnComponentBeginOverlap.AddDynamic(this, &ABBTrainCarriage::OnCircleImmuneOverlap);
		InteractSphere->OnComponentEndOverlap.AddDynamic(this, &ABBTrainCarriage::OnCircleImmuneEndOverlap);
	}

	// Safety net: catch unexpected blocking hits on carriage mesh
	CarriageMesh->OnComponentHit.AddDynamic(this, &ABBTrainCarriage::OnCarriageMeshHit);

	SetupCollision();
}

void ABBTrainCarriage::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority())
	{
		return;
	}

	// Server: maintain circle immunity for overlapping hunters
	if (CarriageType == ETrainCarriageType::CircleImmune)
	{
		TArray<AActor*> OverlappingActors;
		InteractSphere->GetOverlappingActors(OverlappingActors, AHunterCharacter::StaticClass());

		// Re-apply to current overlaps
		for (AActor* Actor : OverlappingActors)
		{
			if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(Actor))
			{
				ApplyCircleImmunity(Hunter);
			}
		}
	}

	// Server: sweep-based kill detection for locomotive (more reliable than overlap events for teleported volumes)
	if (CarriageType == ETrainCarriageType::Locomotive)
	{
		CheckFrontKillZoneSweep();
		DebugLogLocomotiveState();
	}
}

void ABBTrainCarriage::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABBTrainCarriage, CarriageType);
	DOREPLIFETIME(ABBTrainCarriage, CarriageLength);
	DOREPLIFETIME(ABBTrainCarriage, OffsetFromTrainProgress);
}

void ABBTrainCarriage::OnRep_CarriageType()
{
	// Client must recompute length from replicated type so collision sizes match server
	CarriageLength = GetCarriageLength(CarriageType);
	SetupCollision();
}

void ABBTrainCarriage::InitializeCarriage(ETrainCarriageType InType, float InOffsetFromTrainProgress)
{
	CarriageType = InType;
	CarriageLength = GetCarriageLength(InType);
	OffsetFromTrainProgress = InOffsetFromTrainProgress;

	// CRITICAL: SetStaticMesh() may apply the mesh's default collision profile,
	// overriding our ECR_Ignore. Re-apply after mesh assignment.
	CarriageMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CarriageMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	SetupCollision();
}

float ABBTrainCarriage::GetCarriageLength(ETrainCarriageType Type)
{
	switch (Type)
	{
	case ETrainCarriageType::Locomotive:
		return 800.0f;
	default:
		return 400.0f;
	}
}

void ABBTrainCarriage::EnableReviveBeacon()
{
	bHasReviveBeacon = true;
	SetupCollision();
}

void ABBTrainCarriage::UpdatePosition(float TrainProgress, ABBTrainTrack* Track)
{
	if (!Track || !Track->GetSpline())
	{
		return;
	}

	// Store transform before moving — used by CaptureRiderOffsets to compute local offsets
	PreviousTransform = GetActorTransform();
	bHasPreviousTransform = true;

	// Calculate this carriage's progress along the track
	const float CarriageProgress = Track->AdvanceProgress(TrainProgress,
		-OffsetFromTrainProgress * Track->GetTrackLength(), true);

	const FTransform TrackTransform = Track->GetTransformAtProgress(CarriageProgress, true);
	const FVector TargetLocation = TrackTransform.GetLocation();
	const FQuat TargetRotation = TrackTransform.GetRotation();

	const FVector OldLocation = GetActorLocation();
	const FQuat OldRotation = GetActorQuat();

	SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);

	const FVector ActualLocation = GetActorLocation();
	const FQuat ActualRotation = GetActorQuat();

	const FVector Delta = ActualLocation - OldLocation;
	const FVector Diff = ActualLocation - TargetLocation;
	const float RotDiff = TargetRotation.AngularDistance(ActualRotation);

	// Only warn if something moved us after SetActorLocationAndRotation (physics, collision, etc.)
	if (Diff.SizeSquared() > 0.01f || RotDiff > 0.001f)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("TrainJitter: %s MOVED AFTER SET! diff=%s rotDiff=%.4f"),
			*GetName(), *Diff.ToString(), RotDiff);
	}
}

void ABBTrainCarriage::UpdateRiders()
{
	CaptureRiderOffsets();
	DetectNewRiders();
	PrePositionRiders();
}

void ABBTrainCarriage::PrePositionRiders()
{
	const FTransform CurrentTransform = GetActorTransform();

	for (auto& Pair : RiderLocalOffsets)
	{
		ACharacter* Character = Pair.Key.Get();
		if (!Character)
		{
			continue;
		}

		const FVector LocalOffset = Pair.Value;
		const FVector TargetWorldPos = CurrentTransform.TransformPosition(LocalOffset);
		const FVector CurrentWorldPos = Character->GetActorLocation();
		const float PosErrorSq = FVector::DistSquared(CurrentWorldPos, TargetWorldPos);

		// Only teleport if the character has drifted >1cm from where they should be.
		// This preserves CMC-driven movement (walking, input) while still syncing
		// the character to the platform's movement.
		static constexpr float TELEPORT_THRESHOLD_SQ = FMath::Square(1.0f);
		if (PosErrorSq > TELEPORT_THRESHOLD_SQ)
		{
			Character->SetActorLocation(TargetWorldPos, false, nullptr, ETeleportType::TeleportPhysics);
		}

		// Reset base so UpdateBasedMovement doesn't double-apply platform delta.
		// DO NOT zero velocity — let the CMC preserve input-driven momentum.
		if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
		{
			CMC->SetBase(nullptr);

			// TEST: If enabled, put rider in flying mode to eliminate FindFloor micro-jitter
			if (bTestFlyingModeForRiders)
			{
				CMC->SetMovementMode(MOVE_Flying);
			}
		}
	}
}

void ABBTrainCarriage::CaptureRiderOffsets()
{
	TArray<ACharacter*> ToRemove;

	for (auto& Pair : RiderLocalOffsets)
	{
		ACharacter* Character = Pair.Key.Get();
		if (!Character)
		{
			ToRemove.Add(nullptr);
			continue;
		}

		const FVector WorldPos = Character->GetActorLocation();
		const FVector LocalOffset = PreviousTransform.InverseTransformPosition(WorldPos);

		if (!IsLocalOffsetOnPlatform(LocalOffset, false))
		{
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("TrainRider: %s LEFT %s (out of bounds: %s)"),
				*Character->GetName(), *GetName(), *LocalOffset.ToString());
			ToRemove.Add(Character);
			continue;
		}

		Pair.Value = LocalOffset;
	}

	for (ACharacter* Character : ToRemove)
	{
		if (Character)
		{
			RemoveRider(Character);
		}
		else
		{
			RiderLocalOffsets.Remove(nullptr);
		}
	}
}

void ABBTrainCarriage::DetectNewRiders()
{
	// Locomotive cannot be ridden — characters phase through it or get killed by front contact
	if (CarriageType == ETrainCarriageType::Locomotive)
	{
		return;
	}

	const FVector2D Bounds = GetPlatformBounds();
	const float DetectionRadius = FMath::Max(Bounds.X, Bounds.Y) + 150.0f;

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(DetectionRadius);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	if (GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectParams, Sphere))
	{
		for (const FOverlapResult& Overlap : Overlaps)
			{
			ACharacter* Character = Cast<ACharacter>(Overlap.GetActor());
			if (!Character || RiderLocalOffsets.Contains(Character))
			{
				continue;
			}

			// Check if character is on/near platform surface
			const FVector LocalOffset = GetActorTransform().InverseTransformPosition(Character->GetActorLocation());
			const bool bOnPlatform = IsLocalOffsetOnPlatform(LocalOffset, true);

			if (!bOnPlatform)
			{
				continue;
			}

			// Don't add falling characters (jumping off, knocked off, etc.)
			if (UCharacterMovementComponent* CMC = Character->GetCharacterMovement())
			{
				if (CMC->IsFalling())
				{
					UE_LOG(LogBreachborne, VeryVerbose, TEXT("TrainRider: %s rejected — falling on %s"), *Character->GetName(), *GetName());
					continue;
				}
			}

			RiderLocalOffsets.Add(Character, LocalOffset);
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("TrainRider: %s BOARDED %s at %s"),
				*Character->GetName(), *GetName(), *LocalOffset.ToString());
		}
	}
}

void ABBTrainCarriage::RemoveRider(ACharacter* Character)
{
	if (!Character)
	{
		return;
	}

	RiderLocalOffsets.Remove(Character);
}

bool ABBTrainCarriage::IsLocalOffsetOnPlatform(const FVector& LocalOffset, bool bLooseCheck) const
{
	const FVector2D Bounds = GetPlatformBounds();
	const float HalfLength = Bounds.X;
	const float HalfWidth = Bounds.Y;

	const float EdgeTolerance = bLooseCheck ? 100.0f : 50.0f;
	const float MinHeight = -5.0f; // Allow small floating-point dips (feet at Z≈0) but reject players who fell through locomotive (Z≈-68)
	const float MaxHeight = bLooseCheck ? 500.0f : 400.0f; // Strict check needs headroom for jumping players (was 300, too tight)

	return FMath::Abs(LocalOffset.X) <= HalfLength + EdgeTolerance &&
	       FMath::Abs(LocalOffset.Y) <= HalfWidth + EdgeTolerance &&
	       LocalOffset.Z >= MinHeight &&
	       LocalOffset.Z <= MaxHeight;
}

FVector2D ABBTrainCarriage::GetPlatformBounds() const
{
	return FVector2D(CarriageLength / 2.0f, 400.0f);
}

bool ABBTrainCarriage::TryActivate(AHunterCharacter* Activator)
{
	if (!HasAuthority() || !Activator)
	{
		return false;
	}

	// Proximity check
	const float DistSq = FVector::DistSquared(GetActorLocation(), Activator->GetActorLocation());
	if (DistSq > FMath::Square(InteractRadius))
	{
		return false;
	}

	switch (CarriageType)
	{
	case ETrainCarriageType::Chest:
		if (ChestComponent)
		{
			// TODO: pass the player's puzzle attempt from UI
			// For now, auto-unlock for testing
			TArray<bool> DummyAttempt;
			ChestComponent->TryUnlock(DummyAttempt);
			return true;
		}
		break;

	case ETrainCarriageType::ReviveBeacon:
	default:
		if (bHasReviveBeacon)
		{
			ABreachbornePlayerState* PS = Activator->GetPlayerState<ABreachbornePlayerState>();
			if (PS)
			{
				ABreachborneGameMode* GM = Cast<ABreachborneGameMode>(GetWorld()->GetAuthGameMode());
				if (GM)
				{
					UE_LOG(LogBreachborne, VeryVerbose, TEXT("TrainCarriage: Revive beacon activated by %s"), *PS->GetPlayerName());
					return true;
				}
			}
		}
		break;
	}

	return false;
}

void ABBTrainCarriage::DebugLogLocomotiveState()
{
	static float NextLogTime = 0.0f;
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextLogTime) return;
	NextLogTime = Now + 0.25f; // Log at 4Hz to avoid spam

	const FVector LocoLoc = GetActorLocation();
	const FQuat LocoQuat = GetActorQuat();

	// Log collision component world bounds
	const FVector SideLoc = SideCollision->GetComponentLocation();
	const FVector SideExtent = SideCollision->GetScaledBoxExtent();
	const FVector FZLoc = FrontKillZone->GetComponentLocation();
	const FVector FZExtent = FrontKillZone->GetScaledBoxExtent();

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("TrainDbg: === %s @ %s | Fwd=%s ==="),
		*GetName(), *LocoLoc.ToString(), *GetActorForwardVector().ToString());
	UE_LOG(LogBreachborne, VeryVerbose, TEXT("TrainDbg: SideCollision world=Loc[%s] Extent[%s] | FrontKillZone world=Loc[%s] Extent[%s]"),
		*SideLoc.ToString(), *SideExtent.ToString(), *FZLoc.ToString(), *FZExtent.ToString());

	// Find all hunters near the locomotive
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(2000.0f);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	if (!GetWorld()->OverlapMultiByObjectType(Overlaps, LocoLoc, FQuat::Identity, ObjectParams, Sphere))
	{
		return;
	}

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AHunterCharacter* Hunter = Cast<AHunterCharacter>(Overlap.GetActor());
		if (!Hunter) continue;

		const FVector WorldPos = Hunter->GetActorLocation();
		const FVector LocalPos = GetActorTransform().InverseTransformPosition(WorldPos);
		const float DistToCenter = FVector::Dist(WorldPos, LocoLoc);

		UCharacterMovementComponent* CMC = Hunter->GetCharacterMovement();
		const FString MoveMode = CMC ? UEnum::GetValueAsString(CMC->MovementMode) : TEXT("NO_CMC");
		const FVector Vel = CMC ? CMC->Velocity : FVector::ZeroVector;
		const bool bIsFalling = CMC ? CMC->IsFalling() : false;
		const UPrimitiveComponent* Base = CMC ? CMC->GetMovementBase() : nullptr;
		const bool bIsRider = RiderLocalOffsets.Contains(Hunter);

		// Check overlap with specific components (rotation-aware)
		const bool bOverlapSide = SideCollision->IsOverlappingActor(Hunter);
		const bool bOverlapFZ = FrontKillZone->IsOverlappingActor(Hunter);

		UE_LOG(LogBreachborne, VeryVerbose, TEXT("TrainDbg: %s | dist=%.0f | local=%s | mode=%s | vel=%s | fall=%d | base=%s | rider=%d | overlapSide=%d | overlapFZ=%d"),
			*Hunter->GetName(), DistToCenter, *LocalPos.ToString(), *MoveMode, *Vel.ToString(),
			bIsFalling ? 1 : 0, Base ? *Base->GetName() : TEXT("NONE"),
			bIsRider ? 1 : 0, bOverlapSide ? 1 : 0, bOverlapFZ ? 1 : 0);
	}
}

void ABBTrainCarriage::CheckFrontKillZoneSweep()
{
	const FVector CurrentBoxPos = FrontKillZone->GetComponentLocation();
	const FVector PreviousBoxPos = bHasPreviousTransform
		? (FrontKillZone->GetRelativeTransform() * PreviousTransform).GetLocation()
		: CurrentBoxPos;
	const FVector BoxExtent = FrontKillZone->GetScaledBoxExtent();
	const FQuat BoxRot = FrontKillZone->GetComponentRotation().Quaternion();

	FCollisionShape Box = FCollisionShape::MakeBox(BoxExtent);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TrainFrontKillSweep), false, this);

	auto KillHunter = [this](AHunterCharacter* Hunter, const TCHAR* Reason, float Dot)
	{
		if (!Hunter)
		{
			return;
		}

		const FVector LocalPos = GetActorTransform().InverseTransformPosition(Hunter->GetActorLocation());
		UCharacterMovementComponent* CMC = Hunter->GetCharacterMovement();
		UE_LOG(LogBreachborne, Warning, TEXT("TrainKillDebug: KILL reason=%s hunter=%s local=%s dot=%.2f vel=%s frontLoc=%s frontExtent=%s"),
			Reason,
			*GetNameSafe(Hunter),
			*LocalPos.ToCompactString(),
			Dot,
			CMC ? *CMC->Velocity.ToCompactString() : TEXT("no-cmc"),
			*FrontKillZone->GetComponentLocation().ToCompactString(),
			*FrontKillZone->GetScaledBoxExtent().ToCompactString());

		ABreachborneGameMode* GM = Cast<ABreachborneGameMode>(GetWorld()->GetAuthGameMode());
		if (GM)
		{
			GM->HandleTrainDeath(Hunter, Hunter->GetActorLocation());
		}
	};

	TArray<FOverlapResult> Overlaps;
	if (GetWorld()->OverlapMultiByObjectType(Overlaps, CurrentBoxPos, BoxRot, ObjectParams, Box))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AHunterCharacter* Hunter = Cast<AHunterCharacter>(Overlap.GetActor());
			if (!Hunter) continue;
			if (RiderLocalOffsets.Contains(Hunter)) continue;

			const float Dot = FVector::DotProduct(
				(Hunter->GetActorLocation() - GetActorLocation()).GetSafeNormal(),
				GetActorForwardVector());

			const bool bWouldKill = IsCharacterInLocomotiveFrontKillBand(Hunter, Dot);
			LogTrainKillCandidate(Hunter, TEXT("front-overlap"), Dot, bWouldKill);
			if (!bWouldKill) continue;

			KillHunter(Hunter, TEXT("front-overlap"), Dot);
		}
	}

	if (!PreviousBoxPos.Equals(CurrentBoxPos, 1.0f))
	{
		TArray<FHitResult> SweepHits;
		if (GetWorld()->SweepMultiByObjectType(SweepHits, PreviousBoxPos, CurrentBoxPos, BoxRot, ObjectParams, Box, QueryParams))
		{
			for (const FHitResult& Hit : SweepHits)
			{
				AHunterCharacter* Hunter = Cast<AHunterCharacter>(Hit.GetActor());
				if (!Hunter) continue;
				if (RiderLocalOffsets.Contains(Hunter)) continue;

				const float Dot = FVector::DotProduct(
					(Hunter->GetActorLocation() - GetActorLocation()).GetSafeNormal(),
					GetActorForwardVector());

				const bool bWouldKill = IsCharacterInLocomotiveFrontKillBand(Hunter, Dot);
				LogTrainKillCandidate(Hunter, TEXT("front-sweep"), Dot, bWouldKill);
				if (!bWouldKill) continue;

				KillHunter(Hunter, TEXT("front-sweep"), Dot);
			}
		}
	}

	TArray<FOverlapResult> NearbyOverlaps;
	const float BodyProbeRadius = FMath::Max(CarriageLength, 800.0f);
	if (!GetWorld()->OverlapMultiByObjectType(NearbyOverlaps, GetActorLocation(), FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(BodyProbeRadius), QueryParams))
	{
		return;
	}

	for (const FOverlapResult& Overlap : NearbyOverlaps)
	{
		AHunterCharacter* Hunter = Cast<AHunterCharacter>(Overlap.GetActor());
		if (!Hunter || RiderLocalOffsets.Contains(Hunter))
		{
			continue;
		}

		if (IsCharacterInLocomotiveNoRideZone(Hunter))
		{
			const float Dot = FVector::DotProduct(
				(Hunter->GetActorLocation() - GetActorLocation()).GetSafeNormal(),
				GetActorForwardVector());
			const bool bWouldKill = IsCharacterInLocomotiveFrontKillBand(Hunter, Dot)
				|| DidLocomotiveRunOverCharacterThisTick(Hunter);
			LogTrainKillCandidate(Hunter, TEXT("body-probe"), Dot, bWouldKill);
			if (bWouldKill)
			{
				KillHunter(Hunter, IsCharacterInLocomotiveFrontKillBand(Hunter, Dot)
					? TEXT("locomotive-body-crush-front-band")
					: TEXT("locomotive-runover-crossing"), Dot);
			}
		}
	}
}

void ABBTrainCarriage::OnCarriageMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character) return;

	const FVector LocalPos = GetActorTransform().InverseTransformPosition(Character->GetActorLocation());
	UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	const FString MoveMode = CMC ? UEnum::GetValueAsString(CMC->MovementMode) : TEXT("NO_CMC");

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("TrainMeshHit: %s HIT CarriageMesh on %s! local=%s | hitNormal=%s | impulse=%s | mode=%s | vel=%s"),
		*Character->GetName(), *GetName(), *LocalPos.ToString(),
		*Hit.ImpactNormal.ToString(), *NormalImpulse.ToString(),
		*MoveMode, CMC ? *CMC->Velocity.ToString() : TEXT("N/A"));
}

bool ABBTrainCarriage::IsCharacterInLocomotiveNoRideZone(const ACharacter* Character) const
{
	if (!Character || CarriageType != ETrainCarriageType::Locomotive)
	{
		return false;
	}

	const FVector LocalPos = GetActorTransform().InverseTransformPosition(Character->GetActorLocation());
	const float HalfLength = CarriageLength * 0.5f;
	const float HalfWidth = 400.0f;
	const float HorizontalTolerance = 80.0f;
	const float MinZ = -120.0f;
	const float MaxZ = LocomotiveWallHeight + 150.0f;

	return FMath::Abs(LocalPos.X) <= HalfLength + HorizontalTolerance
		&& FMath::Abs(LocalPos.Y) <= HalfWidth + HorizontalTolerance
		&& LocalPos.Z >= MinZ
		&& LocalPos.Z <= MaxZ;
}

bool ABBTrainCarriage::IsCharacterInLocomotiveFrontKillBand(const ACharacter* Character, float Dot) const
{
	if (!Character || CarriageType != ETrainCarriageType::Locomotive)
	{
		return false;
	}

	const FVector LocalPos = GetActorTransform().InverseTransformPosition(Character->GetActorLocation());
	const float HalfLength = CarriageLength * 0.5f;
	const float HalfWidth = 400.0f;
	const float FrontBandMinX = HalfLength - FMath::Max(KillZoneDepth, 1.0f);
	const float SideTolerance = 90.0f;
	const float MinZ = -120.0f;
	const float MaxZ = KillZoneCenterZ + KillZoneHalfHeight + 80.0f;

	return LocalPos.X >= FrontBandMinX
		&& FMath::Abs(LocalPos.Y) <= HalfWidth + SideTolerance
		&& LocalPos.Z >= MinZ
		&& LocalPos.Z <= MaxZ
		&& Dot >= 0.5f;
}

bool ABBTrainCarriage::DidLocomotiveRunOverCharacterThisTick(const ACharacter* Character) const
{
	if (!Character || CarriageType != ETrainCarriageType::Locomotive || !bHasPreviousTransform)
	{
		return false;
	}

	const FVector WorldPos = Character->GetActorLocation();
	const FVector PreviousLocalPos = PreviousTransform.InverseTransformPosition(WorldPos);
	const FVector CurrentLocalPos = GetActorTransform().InverseTransformPosition(WorldPos);
	const float HalfWidth = 400.0f;
	const float SideTolerance = 90.0f;
	const float MinZ = -120.0f;
	const float MaxZ = KillZoneCenterZ + KillZoneHalfHeight + 80.0f;

	const bool bInLateralBand = FMath::Abs(CurrentLocalPos.Y) <= HalfWidth + SideTolerance
		&& CurrentLocalPos.Z >= MinZ
		&& CurrentLocalPos.Z <= MaxZ;
	if (!bInLateralBand)
	{
		return false;
	}

	constexpr float RunOverLineX = 0.0f;
	constexpr float CrossingTolerance = 35.0f;
	const bool bCrossedRunOverLine = PreviousLocalPos.X >= RunOverLineX - CrossingTolerance
		&& CurrentLocalPos.X <= RunOverLineX + CrossingTolerance
		&& PreviousLocalPos.X > CurrentLocalPos.X;

	return bCrossedRunOverLine;
}

void ABBTrainCarriage::LogTrainKillCandidate(const ACharacter* Character, const TCHAR* Source, float Dot, bool bWouldKill) const
{
	if (!Character || !GetWorld())
	{
		return;
	}

	static TMap<TWeakObjectPtr<const ACharacter>, float> NextLogTimesByCharacter;
	const float Now = GetWorld()->GetTimeSeconds();
	const float* NextLogTime = NextLogTimesByCharacter.Find(Character);
	if (NextLogTime && Now < *NextLogTime && !bWouldKill)
	{
		return;
	}
	NextLogTimesByCharacter.Add(Character, Now + (bWouldKill ? 0.0f : 0.25f));

	const FVector LocalPos = GetActorTransform().InverseTransformPosition(Character->GetActorLocation());
	const FVector PreviousLocalPos = bHasPreviousTransform
		? PreviousTransform.InverseTransformPosition(Character->GetActorLocation())
		: FVector::ZeroVector;
	const float HalfLength = CarriageLength * 0.5f;
	const float FrontBandMinX = HalfLength - FMath::Max(KillZoneDepth, 1.0f);
	const float MaxZ = KillZoneCenterZ + KillZoneHalfHeight + 80.0f;
	const UCharacterMovementComponent* CMC = Character->GetCharacterMovement();

	UE_LOG(LogBreachborne, Warning, TEXT("TrainKillDebug: CAND source=%s wouldKill=%d hunter=%s local=%s prevLocal=%s frontMinX=%.1f dot=%.2f zRange=[-120.0,%.1f] vel=%s frontLoc=%s frontExtent=%s prevValid=%d"),
		Source,
		bWouldKill ? 1 : 0,
		*GetNameSafe(Character),
		*LocalPos.ToCompactString(),
		*PreviousLocalPos.ToCompactString(),
		FrontBandMinX,
		Dot,
		MaxZ,
		CMC ? *CMC->Velocity.ToCompactString() : TEXT("no-cmc"),
		*FrontKillZone->GetComponentLocation().ToCompactString(),
		*FrontKillZone->GetScaledBoxExtent().ToCompactString(),
		bHasPreviousTransform ? 1 : 0);
}

void ABBTrainCarriage::OnFrontKillZoneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || CarriageType != ETrainCarriageType::Locomotive)
	{
		return;
	}

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(OtherActor);
	if (!Hunter)
	{
		return;
	}

	// Check if the hunter is actually in front of the locomotive (dot product check)
	const FVector ToHunter = (Hunter->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FVector Forward = GetActorForwardVector();
	const float Dot = FVector::DotProduct(ToHunter, Forward);

	// Must be significantly in front (within ~60 degrees of forward direction)
	const bool bWouldKill = IsCharacterInLocomotiveFrontKillBand(Hunter, Dot);
	LogTrainKillCandidate(Hunter, TEXT("front-overlap-event"), Dot, bWouldKill);
	if (!bWouldKill)
	{
		// Not in front — ignore
		return;
	}

	// Instant death — bypass wisp, spawn deathbox immediately
	ABreachborneGameMode* GM = Cast<ABreachborneGameMode>(GetWorld()->GetAuthGameMode());
	if (GM)
	{
		GM->HandleTrainDeath(Hunter, Hunter->GetActorLocation());
		const FVector LocalPos = GetActorTransform().InverseTransformPosition(Hunter->GetActorLocation());
		UE_LOG(LogBreachborne, Warning, TEXT("TrainKillDebug: KILL reason=front-overlap-event hunter=%s local=%s dot=%.2f"),
			*GetNameSafe(Hunter), *LocalPos.ToCompactString(), Dot);
	}
}

void ABBTrainCarriage::OnCircleImmuneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || CarriageType != ETrainCarriageType::CircleImmune)
	{
		return;
	}

	if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(OtherActor))
	{
		ApplyCircleImmunity(Hunter);
	}
}

void ABBTrainCarriage::OnCircleImmuneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority() || CarriageType != ETrainCarriageType::CircleImmune)
	{
		return;
	}

	if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(OtherActor))
	{
		RemoveCircleImmunity(Hunter);
	}
}

void ABBTrainCarriage::SetupCollision()
{
	// Locomotive: thin wall collision (no solid interior), front kill zone active
	// Platform: side collision is the walkable floor
	// Chest/CircleImmune/ReviveBeacon: interaction sphere active, side collision active

	const bool bIsLocomotive = (CarriageType == ETrainCarriageType::Locomotive);

	// CRITICAL: CarriageMesh has non-uniform scale (e.g. 4,8,0.3).
	// Child components inherit this scale, which bakes their world position/extent.
	// We must counteract by dividing relative location and extent by mesh scale.
	const FVector MeshScale = CarriageMesh->GetRelativeScale3D();
	const FVector SafeScale(
		FMath::IsNearlyZero(MeshScale.X) ? 1.0f : MeshScale.X,
		FMath::IsNearlyZero(MeshScale.Y) ? 1.0f : MeshScale.Y,
		FMath::IsNearlyZero(MeshScale.Z) ? 1.0f : MeshScale.Z
	);

	// Ensure mesh NEVER blocks — all gameplay collision comes from components
	CarriageMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CarriageMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	const float HalfLength = CarriageLength / 2.0f;
	const float HalfWidth = 400.0f;
	const float WallThickness = 5.0f;
	const float ZExtent = bIsLocomotive ? LocomotiveWallHeight : 50.0f;

	// -------------------------------------------------------------------------
	// SideCollision: platforms need a floor. Locomotive uses thin walls instead.
	// -------------------------------------------------------------------------
	SideCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f / SafeScale.Z));
	SideCollision->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	SideCollision->SetBoxExtent(FVector(HalfLength, HalfWidth, ZExtent));
	SideCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	if (bIsLocomotive)
	{
		// Disable the giant solid box for locomotive — thin walls handle blocking
		SideCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SideCollision->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Default, 0.0f));
	}
	else
	{
		// Platforms: SideCollision is the walkable floor
		SideCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SideCollision->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Default, 0.0f));
	}

	// -------------------------------------------------------------------------
	// Locomotive thin walls: left, right, back. No solid interior.
	// Front is handled by FrontKillZone (instant death).
	// -------------------------------------------------------------------------
	if (bIsLocomotive)
	{
		// Left wall: thin in Y, full length & height
		LocomotiveLeftWall->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		LocomotiveLeftWall->SetBoxExtent(FVector(HalfLength, WallThickness, ZExtent));
		LocomotiveLeftWall->SetRelativeLocation(FVector(0.0f, -(HalfWidth + WallThickness) / SafeScale.Y, 65.0f / SafeScale.Z));

		// Right wall: thin in Y, full length & height
		LocomotiveRightWall->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		LocomotiveRightWall->SetBoxExtent(FVector(HalfLength, WallThickness, ZExtent));
		LocomotiveRightWall->SetRelativeLocation(FVector(0.0f, (HalfWidth + WallThickness) / SafeScale.Y, 65.0f / SafeScale.Z));

		// Back wall: thin in X, full width & height
		LocomotiveBackWall->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		LocomotiveBackWall->SetBoxExtent(FVector(WallThickness, HalfWidth, ZExtent));
		LocomotiveBackWall->SetRelativeLocation(FVector(-(HalfLength + WallThickness) / SafeScale.X, 0.0f, 65.0f / SafeScale.Z));
	}
	else
	{
		LocomotiveLeftWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LocomotiveRightWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LocomotiveBackWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Front kill zone: no SetAbsolute, so it inherits parent scale. Divide by mesh scale
	// so the world-space volume covers the visible locomotive front and full hunter capsule.
	const float KillZoneHalfDepth = FMath::Max(KillZoneDepth, 1.0f) / 2.0f;
	const float KillZoneCenterHeight = FMath::Max(0.0f, KillZoneCenterZ);
	const float KillZoneZExtent = FMath::Max(50.0f, KillZoneHalfHeight);
	FrontKillZone->SetRelativeLocation(FVector((HalfLength + KillZoneHalfDepth) / SafeScale.X, 0.0f, KillZoneCenterHeight / SafeScale.Z));
	FrontKillZone->SetBoxExtent(FVector(KillZoneHalfDepth / SafeScale.X, HalfWidth / SafeScale.Y, KillZoneZExtent / SafeScale.Z));

	FrontKillZone->SetCollisionEnabled(bIsLocomotive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	FrontKillZone->SetGenerateOverlapEvents(bIsLocomotive);

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("TrainCarriage: %s setup | type=%d | len=%.0f | walls=%s"),
		*GetName(), static_cast<int32>(CarriageType), CarriageLength,
		bIsLocomotive ? TEXT("LEFT+RIGHT+BACK") : TEXT("SIDE_FLOOR"));

	const bool bHasInteract = (CarriageType == ETrainCarriageType::Chest)
		|| (CarriageType == ETrainCarriageType::CircleImmune)
		|| (CarriageType == ETrainCarriageType::ReviveBeacon)
		|| bHasReviveBeacon;
	InteractSphere->SetCollisionEnabled(bHasInteract ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	InteractSphere->SetGenerateOverlapEvents(bHasInteract);
}

void ABBTrainCarriage::ApplyCircleImmunity(AHunterCharacter* Hunter)
{
	if (!Hunter || ImmuneHunters.Contains(Hunter))
	{
		return;
	}

	UAbilitySystemComponent* ASC = Hunter->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// Add a loose gameplay tag for storm immunity
	ASC->AddLooseGameplayTag(BBGameplayTags::State_TrainCircleImmune);
	ImmuneHunters.Add(Hunter);

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("Train: %s gained circle immunity"), *Hunter->GetName());
}

void ABBTrainCarriage::RemoveCircleImmunity(AHunterCharacter* Hunter)
{
	if (!Hunter)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Hunter->GetAbilitySystemComponent();
	if (ASC)
	{
		ASC->RemoveLooseGameplayTag(BBGameplayTags::State_TrainCircleImmune);
	}

	ImmuneHunters.Remove(Hunter);

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("Train: %s lost circle immunity"), *Hunter->GetName());
}
