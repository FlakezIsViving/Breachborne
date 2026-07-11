#include "BBTrainTrack.h"
#include "BBTrain.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Breachborne/Breachborne.h"

ABBTrainTrack::ABBTrainTrack()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	TrackSpline = CreateDefaultSubobject<USplineComponent>(TEXT("TrackSpline"));
	SetRootComponent(TrackSpline);

	// Default to a simple straight line with 2 points
	TrackSpline->ClearSplinePoints();
	TrackSpline->AddSplinePoint(FVector(0.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local);
	TrackSpline->AddSplinePoint(FVector(1000.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local);

	// Try to find a default cube mesh for procedural visuals
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		DefaultCubeMesh = CubeMeshFinder.Object;
	}

	// Hierarchical Instanced Static Mesh components for rails and sleepers
	// One component per mesh type = massive draw call reduction vs individual SMCs
	LeftRailInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("LeftRailInstances"));
	LeftRailInstances->SetupAttachment(TrackSpline);
	LeftRailInstances->SetMobility(EComponentMobility::Static);
	LeftRailInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftRailInstances->SetGenerateOverlapEvents(false);

	RightRailInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RightRailInstances"));
	RightRailInstances->SetupAttachment(TrackSpline);
	RightRailInstances->SetMobility(EComponentMobility::Static);
	RightRailInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightRailInstances->SetGenerateOverlapEvents(false);

	SleeperInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("SleeperInstances"));
	SleeperInstances->SetupAttachment(TrackSpline);
	SleeperInstances->SetMobility(EComponentMobility::Static);
	SleeperInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SleeperInstances->SetGenerateOverlapEvents(false);
}

void ABBTrainTrack::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// One-time cleanup: destroy old individual rail mesh components from previous versions
	// (before HISM refactor, rails were saved as hundreds of UStaticMeshComponent children)
	TArray<UActorComponent*> ComponentsToRemove;
	for (UActorComponent* Comp : GetComponents())
	{
		if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Comp))
		{
			const FString Name = SMC->GetName();
			if (Name.StartsWith(TEXT("RailLeft_")) ||
			    Name.StartsWith(TEXT("RailRight_")) ||
			    Name.StartsWith(TEXT("Sleeper_")))
			{
				ComponentsToRemove.Add(SMC);
			}
		}
	}
	for (UActorComponent* Comp : ComponentsToRemove)
	{
		Comp->DestroyComponent();
	}

	// Only rebuild in editor when the spline actually changed — OnConstruction() fires
	// on every property twitch, and rebuilding 500 instances each time lags the editor.
	if (bAutoGenerateRails && TrackSpline)
	{
		const int32 CurrentPointCount = TrackSpline->GetNumberOfSplinePoints();
		const float CurrentLength = TrackSpline->GetSplineLength();
		if (CurrentPointCount != LastSplinePointCount ||
		    FMath::Abs(CurrentLength - LastSplineLength) > 1.0f)
		{
			RebuildRailMeshes();
			LastSplinePointCount = CurrentPointCount;
			LastSplineLength = CurrentLength;
		}
	}
}

void ABBTrainTrack::BeginPlay()
{
	Super::BeginPlay();

	// Build track visuals at runtime — each client builds its own instances locally
	// (track is static, so no replication needed for visual meshes)
	RebuildRailMeshes();

	// Disable tick in game — only needed in editor
	SetActorTickEnabled(false);
}

void ABBTrainTrack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Auto-rebuild disabled for stability — use ForceRebuild button manually
}

void ABBTrainTrack::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

float ABBTrainTrack::GetTrackLength() const
{
	if (!TrackSpline)
	{
		return 0.0f;
	}
	return TrackSpline->GetSplineLength();
}

bool ABBTrainTrack::IsLoop() const
{
	if (!TrackSpline)
	{
		return false;
	}
	return TrackSpline->IsClosedLoop();
}

FTransform ABBTrainTrack::GetTransformAtDistance(float Distance, bool bWorldSpace) const
{
	if (!TrackSpline)
	{
		return FTransform::Identity;
	}

	const float ClampedDist = FMath::Clamp(Distance, 0.0f, GetTrackLength());
	const ESplineCoordinateSpace::Type Space = bWorldSpace ? ESplineCoordinateSpace::World : ESplineCoordinateSpace::Local;

	return TrackSpline->GetTransformAtDistanceAlongSpline(ClampedDist, Space);
}

FTransform ABBTrainTrack::GetTransformAtProgress(float Progress, bool bWorldSpace) const
{
	if (!TrackSpline)
	{
		return FTransform::Identity;
	}

	const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	const ESplineCoordinateSpace::Type Space = bWorldSpace ? ESplineCoordinateSpace::World : ESplineCoordinateSpace::Local;

	return TrackSpline->GetTransformAtTime(ClampedProgress, Space, true);
}

float ABBTrainTrack::GetDistanceAtProgress(float Progress) const
{
	if (!TrackSpline)
	{
		return 0.0f;
	}
	return TrackSpline->GetSplineLength() * FMath::Clamp(Progress, 0.0f, 1.0f);
}

float ABBTrainTrack::GetProgressAtDistance(float Distance) const
{
	const float Length = GetTrackLength();
	if (Length <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}
	return FMath::Clamp(Distance / Length, 0.0f, 1.0f);
}

float ABBTrainTrack::AdvanceProgress(float CurrentProgress, float DeltaDistance, bool bForward) const
{
	const float Length = GetTrackLength();
	if (Length <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const float CurrentDist = CurrentProgress * Length;
	const float NewDist = bForward ? (CurrentDist + DeltaDistance) : (CurrentDist - DeltaDistance);

	if (IsLoop())
	{
		// Wrap around for loops
		const float WrappedDist = FMath::Fmod(NewDist, Length);
		if (WrappedDist < 0.0f)
		{
			return (WrappedDist + Length) / Length;
		}
		return WrappedDist / Length;
	}

	// Clamp for non-loops
	return FMath::Clamp(NewDist / Length, 0.0f, 1.0f);
}

void ABBTrainTrack::RebuildRailMeshes()
{
	ClearRailMeshes();
	ClearSleepers();

	if (!TrackSpline || !LeftRailInstances || !RightRailInstances || !SleeperInstances)
	{
		return;
	}

	const float SplineLength = TrackSpline->GetSplineLength();
	if (SplineLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	UStaticMesh* MeshToUse = RailMesh ? RailMesh.Get() : DefaultCubeMesh.Get();
	if (!MeshToUse)
	{
		return;
	}

	LeftRailInstances->SetStaticMesh(MeshToUse);
	RightRailInstances->SetStaticMesh(MeshToUse);

	const int32 NumSegments = FMath::Max(1, FMath::CeilToInt(SplineLength / SegmentLength));
	if (NumSegments > 500)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("TrainTrack: Too many segments (%d), capping at 500"), NumSegments);
	}
	const int32 ClampedNumSegments = FMath::Min(NumSegments, 500);
	const float ActualSegmentLength = SplineLength / ClampedNumSegments;
	const float HalfTrackWidth = TrackWidth * 0.5f;

	// HISM instances are in component-local space, so we compute world transforms
	// exactly like the old code and convert to relative space explicitly.
	const FTransform HISMToWorld = LeftRailInstances->GetComponentTransform();

	for (int32 i = 0; i < ClampedNumSegments; ++i)
	{
		const float StartDist = i * ActualSegmentLength;
		const float EndDist = (i + 1) * ActualSegmentLength;
		const float MidDist = (StartDist + EndDist) * 0.5f;

		// WORLD-space spline data (same math as old individual-component code)
		const FVector StartPosWorld = TrackSpline->GetLocationAtDistanceAlongSpline(StartDist, ESplineCoordinateSpace::World)
			+ FVector(0.0f, 0.0f, RailMeshVerticalOffset);
		const FVector EndPosWorld = TrackSpline->GetLocationAtDistanceAlongSpline(EndDist, ESplineCoordinateSpace::World)
			+ FVector(0.0f, 0.0f, RailMeshVerticalOffset);
		const FVector MidPosWorld = TrackSpline->GetLocationAtDistanceAlongSpline(MidDist, ESplineCoordinateSpace::World)
			+ FVector(0.0f, 0.0f, RailMeshVerticalOffset);

		const FVector TangentWorld = TrackSpline->GetTangentAtDistanceAlongSpline(MidDist, ESplineCoordinateSpace::World).GetSafeNormal();
		const FVector RightWorld = TrackSpline->GetRightVectorAtDistanceAlongSpline(MidDist, ESplineCoordinateSpace::World);

		// Actual straight-line distance between start and end of this sub-segment
		const float StraightLineDist = (EndPosWorld - StartPosWorld).Size();
		const float RealSegmentLength = FMath::IsFinite(StraightLineDist) ? StraightLineDist * 1.05f : ActualSegmentLength;

		const FQuat Rotation = FRotationMatrix::MakeFromXY(TangentWorld, RightWorld).ToQuat();
		const FVector Scale = !RailMesh
			? FVector(RealSegmentLength / 100.0f, RailThickness / 100.0f, RailHeight / 100.0f)
			: FVector(1.0f, 1.0f, 1.0f);

		// Left rail instance
		{
			const FVector WorldPos = MidPosWorld - RightWorld * HalfTrackWidth;
			FTransform WorldTM(Rotation, WorldPos, Scale);
			LeftRailInstances->AddInstance(WorldTM.GetRelativeTransform(HISMToWorld));
		}

		// Right rail instance
		{
			const FVector WorldPos = MidPosWorld + RightWorld * HalfTrackWidth;
			FTransform WorldTM(Rotation, WorldPos, Scale);
			RightRailInstances->AddInstance(WorldTM.GetRelativeTransform(HISMToWorld));
		}
	}

	// Build sleepers (ties) perpendicular to the track
	RebuildSleepers(SplineLength);
}

void ABBTrainTrack::RebuildSleepers(float SplineLength)
{
	if (!TrackSpline || !SleeperInstances)
	{
		return;
	}

	UStaticMesh* MeshToUse = SleeperMesh ? SleeperMesh.Get() : DefaultCubeMesh.Get();
	if (!MeshToUse)
	{
		return;
	}

	SleeperInstances->SetStaticMesh(MeshToUse);

	const int32 NumSleepers = FMath::Max(1, FMath::FloorToInt(SplineLength / SleeperSpacing));

	// HISM instances are in component-local space — convert from world explicitly
	const FTransform HISMToWorld = SleeperInstances->GetComponentTransform();

	for (int32 i = 0; i <= NumSleepers; ++i)
	{
		const float Distance = i * SleeperSpacing;
		if (Distance > SplineLength)
		{
			break;
		}

		// WORLD-space track data (same math as old individual-component code)
		const FVector TrackPosWorld = TrackSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World)
			+ FVector(0.0f, 0.0f, RailMeshVerticalOffset);
		const FVector TrackTangentWorld = TrackSpline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World).GetSafeNormal();
		const FVector TrackRightWorld = TrackSpline->GetRightVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);

		const FQuat Rotation = FRotationMatrix::MakeFromXY(TrackTangentWorld, TrackRightWorld).ToQuat();
		const FVector Scale(SleeperLength / 100.0f, SleeperWidth / 100.0f, SleeperHeight / 100.0f);

		FTransform WorldTM(Rotation, TrackPosWorld, Scale);
		SleeperInstances->AddInstance(WorldTM.GetRelativeTransform(HISMToWorld));
	}
}

#if WITH_EDITOR
void ABBTrainTrack::AddPointAtEnd()
{
	if (!TrackSpline)
	{
		return;
	}

	const int32 NumPoints = TrackSpline->GetNumberOfSplinePoints();
	if (NumPoints == 0)
	{
		TrackSpline->AddSplinePoint(FVector(0.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local);
		TrackSpline->AddSplinePoint(FVector(1000.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local);
	}
	else
	{
		// Get the last point's position and tangent
		const FVector LastPos = TrackSpline->GetLocationAtSplinePoint(NumPoints - 1, ESplineCoordinateSpace::Local);
		const FVector LastTangent = TrackSpline->GetTangentAtSplinePoint(NumPoints - 1, ESplineCoordinateSpace::Local);
		FVector Direction = LastTangent.GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = FVector(1.0f, 0.0f, 0.0f);
		}

		// Add new point 1000 units past the last point
		const FVector NewPos = LastPos + Direction * 1000.0f;
		TrackSpline->AddSplinePoint(NewPos, ESplineCoordinateSpace::Local);

		// Update the spline so the new point is set to Curve type (has tangent handles)
		const int32 NewPointIndex = TrackSpline->GetNumberOfSplinePoints() - 1;
		TrackSpline->SetSplinePointType(NewPointIndex, ESplinePointType::Curve);
	}

	// Rebuild immediately
	RebuildRailMeshes();
	LastSplinePointCount = TrackSpline->GetNumberOfSplinePoints();
	LastSplineLength = TrackSpline->GetSplineLength();
	TimeSinceSplineChanged = RebuildDelay; // Prevent double-rebuild from Tick
}

void ABBTrainTrack::RemoveLastPoint()
{
	if (!TrackSpline)
	{
		return;
	}

	const int32 NumPoints = TrackSpline->GetNumberOfSplinePoints();
	if (NumPoints <= 2)
	{
		return;
	}

	TrackSpline->RemoveSplinePoint(NumPoints - 1, true);

	RebuildRailMeshes();
	LastSplinePointCount = TrackSpline->GetNumberOfSplinePoints();
	LastSplineLength = TrackSpline->GetSplineLength();
	TimeSinceSplineChanged = RebuildDelay;
}

void ABBTrainTrack::ResetToDefault()
{
	if (!TrackSpline)
	{
		return;
	}

	TrackSpline->ClearSplinePoints();
	TrackSpline->AddSplinePoint(FVector(0.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local);
	TrackSpline->AddSplinePoint(FVector(1000.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local);

	RebuildRailMeshes();
	LastSplinePointCount = TrackSpline->GetNumberOfSplinePoints();
	LastSplineLength = TrackSpline->GetSplineLength();
	TimeSinceSplineChanged = RebuildDelay;
}

void ABBTrainTrack::ForceRebuild()
{
	RebuildRailMeshes();
	if (TrackSpline)
	{
		LastSplinePointCount = TrackSpline->GetNumberOfSplinePoints();
		LastSplineLength = TrackSpline->GetSplineLength();
	}
	TimeSinceSplineChanged = RebuildDelay;
}

void ABBTrainTrack::SpawnTestTrain()
{
	if (TestTrain)
	{
		TestTrain->Destroy();
		TestTrain = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TestTrain = GetWorld()->SpawnActor<ABBTrain>(ABBTrain::StaticClass(), GetActorLocation(), GetActorRotation(), SpawnParams);
	if (TestTrain)
	{
		TestTrain->InitializeTrain(this, ETrainMode::Normal);
		TestTrain->StartMoving();
	}
}

void ABBTrainTrack::DestroyTestTrain()
{
	if (TestTrain)
	{
		TestTrain->Destroy();
		TestTrain = nullptr;
	}
}
#endif

void ABBTrainTrack::ClearRailMeshes()
{
	if (LeftRailInstances)
	{
		LeftRailInstances->ClearInstances();
	}
	if (RightRailInstances)
	{
		RightRailInstances->ClearInstances();
	}
}

void ABBTrainTrack::ClearSleepers()
{
	if (SleeperInstances)
	{
		SleeperInstances->ClearInstances();
	}
}
