#include "BBTrain.h"
#include "BBTrainTrack.h"
#include "BBTrainCarriage.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Breachborne/Breachborne.h"

// Console command: Train.SmoothingSpeed [value] — adjust visual smoothing (0=off, 125=default)
static FAutoConsoleCommand CVarTrainSmoothingSpeed(
	TEXT("Train.SmoothingSpeed"),
	TEXT("Set train visual smoothing speed (0=off, higher=tighter tracking). Default 125 gives ~8ms lag."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		float NewSpeed = 125.0f;
		if (Args.Num() > 0)
		{
			NewSpeed = FCString::Atof(*Args[0]);
		}
		for (TObjectIterator<ABBTrain> It; It; ++It)
		{
			if (ABBTrain* Train = *It)
			{
				Train->VisualSmoothingSpeed = NewSpeed;
			}
		}
		UE_LOG(LogBreachborne, Log, TEXT("Train: Visual smoothing speed set to %.1f"), NewSpeed);
	})
);

// Console command: Train.FlyingMode [0|1] — toggle flying mode for all train riders
static FAutoConsoleCommand CVarTrainFlyingMode(
	TEXT("Train.FlyingMode"),
	TEXT("Toggle flying mode for train riders (0=off, 1=on). Use without args to toggle."),
	FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
	{
		bool bEnable = true;
		if (Args.Num() > 0)
		{
			bEnable = FCString::Atof(*Args[0]) > 0;
		}
		else
		{
			// Toggle: check first train's current state
			for (TObjectIterator<ABBTrain> It; It; ++It)
			{
				if (ABBTrain* Train = *It)
				{
					bEnable = !Train->bTestFlyingModeForRiders;
					break;
				}
			}
		}

		for (TObjectIterator<ABBTrain> It; It; ++It)
		{
			if (ABBTrain* Train = *It)
			{
				Train->bTestFlyingModeForRiders = bEnable;
				for (ABBTrainCarriage* Carriage : Train->GetCarriages())
				{
					if (Carriage)
					{
						Carriage->bTestFlyingModeForRiders = bEnable;
					}
				}
			}
		}
		UE_LOG(LogBreachborne, Log, TEXT("Train: Flying mode %s for all riders"), bEnable ? TEXT("ENABLED") : TEXT("DISABLED"));
	})
);

ABBTrain::ABBTrain()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bReplicates = true;
	bAlwaysRelevant = true;

	// Default root component
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void ABBTrain::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnCarriages();
	}
}

void ABBTrain::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyCarriages();
	Super::EndPlay(EndPlayReason);
}

void ABBTrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		if (bUseFixedTimeStep)
		{
			// Fixed timestep: advance logic at constant rate, extrapolate visuals
			if (!bTestFreezeProgress)
			{
				TimeAccumulator += DeltaTime;
				while (TimeAccumulator >= FixedTimeStep)
				{
					AdvanceTrackProgress(FixedTimeStep);
					TimeAccumulator -= FixedTimeStep;
				}
			}

			// Continuous visual extrapolation: start from the last logic step (TrackProgress)
			// and advance by the distance we would travel in the remaining TimeAccumulator.
			// This eliminates the "one step behind" interpolation lag and produces perfectly
			// uniform per-frame displacements proportional to DeltaTime.
			float VisualProgress = TrackProgress;
			if (TimeAccumulator > KINDA_SMALL_NUMBER && bIsMoving && CurrentTrack)
			{
				const float ExtraDistance = Speed * TimeAccumulator;
				VisualProgress = CurrentTrack->AdvanceProgress(TrackProgress, ExtraDistance, bMovingForward);
			}

			// Frame-rate-independent visual smoothing: low-pass filter on progress.
			// This adds a constant ~8ms (6.4cm) of imperceptible lag that completely
			// masks frame-to-frame displacement jitter at all FPS levels.
			if (VisualSmoothingSpeed > KINDA_SMALL_NUMBER)
			{
				if (!bVisualSmoothingInitialized)
				{
					SmoothedVisualProgress = VisualProgress;
					bVisualSmoothingInitialized = true;
				}
				const bool bIsLoop = CurrentTrack && CurrentTrack->IsLoop();
				if (bIsLoop)
				{
					float Delta = VisualProgress - SmoothedVisualProgress;
					if (Delta > 0.5f) Delta -= 1.0f;
					else if (Delta < -0.5f) Delta += 1.0f;
					const float NewDelta = Delta * (1.0f - FMath::Exp(-VisualSmoothingSpeed * DeltaTime));
					SmoothedVisualProgress += NewDelta;
					if (SmoothedVisualProgress >= 1.0f) SmoothedVisualProgress -= 1.0f;
					else if (SmoothedVisualProgress < 0.0f) SmoothedVisualProgress += 1.0f;
				}
				else
				{
					SmoothedVisualProgress = FMath::FInterpTo(SmoothedVisualProgress, VisualProgress, DeltaTime, VisualSmoothingSpeed);
				}
				VisualProgress = SmoothedVisualProgress;
			}

			UpdateCarriagePositions(VisualProgress);
			UpdateRiders();

			// Confirm fixed timestep is active (log once per second, only when moving)
			if (bIsMoving)
			{
				DeltaLogAccumulator += DeltaTime;
				if (DeltaLogAccumulator >= 1.0f)
				{
					DeltaLogAccumulator = 0.0f;
					const float SmoothLag = FMath::Abs(VisualProgress - TrackProgress) * (CurrentTrack ? CurrentTrack->GetTrackLength() : 0.0f);
					UE_LOG(LogBreachborne, VeryVerbose, TEXT("Train: visual=%.6f | logic=%.6f | smooth=%.6f | lag=%.1f cm | acc=%.2f ms | steps=%.1f"),
						VisualProgress, TrackProgress, SmoothedVisualProgress, SmoothLag, TimeAccumulator * 1000.0f, DeltaTime / FixedTimeStep);
				}
			}
		}
		else
		{
			// Variable delta time modes
			float EffectiveDelta = DeltaTime;
			if (bTestFixed60HzDelta)
			{
				EffectiveDelta = 1.0f / 60.0f;
			}
			else if (bUseSmoothedDeltaTime)
			{
				const float ClampedDelta = FMath::Clamp(DeltaTime, 1.0f / 120.0f, 1.0f / 30.0f);
				SmoothedDeltaTime = FMath::Lerp(SmoothedDeltaTime, ClampedDelta, DeltaTimeSmoothingFactor);
				EffectiveDelta = SmoothedDeltaTime;
			}

			AdvanceTrackProgress(EffectiveDelta);
			UpdateCarriagePositions(TrackProgress);
			UpdateRiders();

			// Periodic diagnostic
			DeltaLogAccumulator += DeltaTime;
			if (DeltaLogAccumulator >= 1.0f)
			{
				DeltaLogAccumulator = 0.0f;
				UE_LOG(LogBreachborne, VeryVerbose, TEXT("Train: DeltaTime raw=%.3f ms | effective=%.3f ms | mode=%s"),
					DeltaTime * 1000.0f, EffectiveDelta * 1000.0f,
					bTestFixed60HzDelta ? TEXT("Fixed60Hz") : (bUseSmoothedDeltaTime ? TEXT("Smoothed") : TEXT("Raw")));
			}
		}
	}
	else
	{
		// Client: use replicated TrackProgress directly
		if (CurrentTrack)
		{
			for (ABBTrainCarriage* Carriage : Carriages)
			{
				if (Carriage)
				{
					Carriage->UpdatePosition(TrackProgress, CurrentTrack);
				}
			}
		}
	}
}


void ABBTrain::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABBTrain, CurrentTrack);
	DOREPLIFETIME(ABBTrain, TrackProgress);
	DOREPLIFETIME(ABBTrain, Speed);
	DOREPLIFETIME(ABBTrain, bIsMoving);
	DOREPLIFETIME(ABBTrain, bMovingForward);
	DOREPLIFETIME(ABBTrain, TrainMode);
}

void ABBTrain::InitializeTrain(ABBTrainTrack* Track, ETrainMode Mode)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentTrack = Track;
	TrainMode = Mode;
	TrackProgress = 0.0f;
	bMovingForward = true;
	SmoothedVisualProgress = 0.0f;
	bVisualSmoothingInitialized = false;

	Speed = (Mode == ETrainMode::BulletTrains)
		? NormalSpeed * BulletTrainsSpeedMultiplier
		: NormalSpeed;

	UE_LOG(LogBreachborne, Log, TEXT("Train: Initialized on track %s, mode=%s, speed=%.0f"),
		Track ? *Track->GetName() : TEXT("NULL"),
		Mode == ETrainMode::BulletTrains ? TEXT("BulletTrains") : TEXT("Normal"),
		Speed);
}

void ABBTrain::StartMoving()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsMoving = true;
	UE_LOG(LogBreachborne, Log, TEXT("Train: Started moving (speed=%.0f)"), Speed);
}

void ABBTrain::StopMoving()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsMoving = false;
	UE_LOG(LogBreachborne, Log, TEXT("Train: Stopped"));
}

void ABBTrain::SetSpeed(float NewSpeed)
{
	if (!HasAuthority())
	{
		return;
	}

	Speed = FMath::Max(0.0f, NewSpeed);
}

void ABBTrain::SpawnCarriages()
{
	DestroyCarriages();

	if (!GetWorld())
	{
		return;
	}

	UWorld* World = GetWorld();
	const float TotalCarriageOffset = CarriageLength + CarriageGap;

	if (TrainMode == ETrainMode::Normal)
	{
		// Normal: Locomotive -> 3 rideable platforms. Platform contents are layered on later.
		const TArray<TPair<ETrainCarriageType, TObjectPtr<UStaticMesh>>> CarriageConfig = {
			{ ETrainCarriageType::Locomotive, LocomotiveMesh },
			{ ETrainCarriageType::Platform, PlatformMesh },
			{ ETrainCarriageType::Platform, PlatformMesh },
			{ ETrainCarriageType::Platform, PlatformMesh }
		};

		float CumulativeOffset = 0.0f;
		for (int32 i = 0; i < CarriageConfig.Num(); ++i)
		{
			ABBTrainCarriage* Carriage = World->SpawnActor<ABBTrainCarriage>(ABBTrainCarriage::StaticClass(), GetActorLocation(), GetActorRotation());
			if (Carriage)
			{
				// Disable movement replication — we manually sync via TrackProgress
				Carriage->SetReplicateMovement(false);

				const float ProgressOffset = CurrentTrack ? (CumulativeOffset / CurrentTrack->GetTrackLength()) : 0.0f;

				Carriage->InitializeCarriage(CarriageConfig[i].Key, ProgressOffset);
				Carriage->bTestFlyingModeForRiders = bTestFlyingModeForRiders;

				// Last carriage (index 3) is the revive beacon platform — paint it green
				const bool bIsLast = (i == CarriageConfig.Num() - 1);
				if (bIsLast && CarriageConfig[i].Key == ETrainCarriageType::Platform)
				{
					Carriage->EnableReviveBeacon();
				}

				UE_LOG(LogBreachborne, Log, TEXT("Train: Spawning carriage %d, Type=%d, Offset=%.0f, CustomMesh=%s"),
					i, static_cast<int32>(CarriageConfig[i].Key), CumulativeOffset,
					CarriageConfig[i].Value ? *CarriageConfig[i].Value->GetName() : TEXT("NULL"));

				// Apply default scale/color first
				ApplyDefaultCarriageVisuals(Carriage, static_cast<uint8>(CarriageConfig[i].Key));

				// Override with custom mesh if provided (must re-apply color after, because SetStaticMesh resets materials)
				if (CarriageConfig[i].Value)
				{
					Carriage->GetCarriageMesh()->SetStaticMesh(CarriageConfig[i].Value);
				}

				// Apply color after any mesh override
				FLinearColor FinalColor = FLinearColor::White;
				switch (CarriageConfig[i].Key)
				{
				case ETrainCarriageType::Locomotive: FinalColor = FLinearColor(0.9f, 0.1f, 0.1f); break;
				case ETrainCarriageType::Chest: FinalColor = FLinearColor(0.8f, 0.6f, 0.1f); break;
				case ETrainCarriageType::Platform: FinalColor = FLinearColor(0.3f, 0.3f, 0.35f); break;
				}
				if (bIsLast)
				{
					FinalColor = FLinearColor(0.2f, 0.9f, 0.3f); // Green for rez beacon
				}
				UStaticMeshComponent* Mesh = Carriage->GetCarriageMesh();
				if (Mesh && Mesh->GetStaticMesh())
				{
					UMaterialInterface* BaseMat = Mesh->GetMaterial(0);
					if (!BaseMat)
					{
						BaseMat = Mesh->GetStaticMesh()->GetMaterial(0);
					}
					if (BaseMat)
					{
						UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, Carriage);
						if (DynMat)
						{
							// Try common parameter names — BasicShapeMaterial may use different names
							static const TArray<FName> ColorParamNames = {
								TEXT("Color"), TEXT("BaseColor"), TEXT("Base Color"), TEXT("Tint"), TEXT("EmissiveColor")
							};

							bool bColorApplied = false;
							for (FName ParamName : ColorParamNames)
							{
								FLinearColor DummyValue;
								if (DynMat->GetVectorParameterValue(ParamName, DummyValue))
								{
									DynMat->SetVectorParameterValue(ParamName, FinalColor);
									bColorApplied = true;
									UE_LOG(LogBreachborne, Log, TEXT("Train: Carriage %d color applied via '%s' = %s"),
										i, *ParamName.ToString(), *FinalColor.ToString());
									break;
								}
							}

							if (!bColorApplied)
							{
								// Log available params for debugging
								TArray<FMaterialParameterInfo> ParamInfos;
								TArray<FGuid> ParamIds;
								DynMat->GetAllVectorParameterInfo(ParamInfos, ParamIds);
								for (const FMaterialParameterInfo& Info : ParamInfos)
								{
									UE_LOG(LogBreachborne, Log, TEXT("Train: Available color param: '%s'"), *Info.Name.ToString());
								}
								UE_LOG(LogBreachborne, Warning, TEXT("Train: No color parameter found on material %s for carriage %d"),
									*BaseMat->GetName(), i);
							}

							Mesh->SetMaterial(0, DynMat);
						}
					}
				}
				Carriages.Add(Carriage);

				// Accumulate spacing: half current + gap + half next
				float CurrentHalf = ABBTrainCarriage::GetCarriageLength(CarriageConfig[i].Key) / 2.0f;
				float NextHalf = (i + 1 < CarriageConfig.Num()) ? ABBTrainCarriage::GetCarriageLength(CarriageConfig[i + 1].Key) / 2.0f : 0.0f;
				CumulativeOffset += CurrentHalf + CarriageGap + NextHalf;
			}
		}
	}
	else // BulletTrains
	{
		// BulletTrains: Locomotive + 1 Platform (mini train)
		const TArray<TPair<ETrainCarriageType, TObjectPtr<UStaticMesh>>> CarriageConfig = {
			{ ETrainCarriageType::Locomotive, LocomotiveMesh },
			{ ETrainCarriageType::Platform, PlatformMesh }
		};

		for (int32 i = 0; i < CarriageConfig.Num(); ++i)
		{
			ABBTrainCarriage* Carriage = World->SpawnActor<ABBTrainCarriage>(ABBTrainCarriage::StaticClass(), GetActorLocation(), GetActorRotation());
			if (Carriage)
			{
				// Disable movement replication — we manually sync via TrackProgress
				Carriage->SetReplicateMovement(false);

				const float Offset = i * TotalCarriageOffset;
				const float ProgressOffset = CurrentTrack ? (Offset / CurrentTrack->GetTrackLength()) : 0.0f;

				Carriage->InitializeCarriage(CarriageConfig[i].Key, ProgressOffset);
				Carriage->bTestFlyingModeForRiders = bTestFlyingModeForRiders;
				if (CarriageConfig[i].Value)
				{
					Carriage->GetCarriageMesh()->SetStaticMesh(CarriageConfig[i].Value);
				}
				else
				{
					ApplyDefaultCarriageVisuals(Carriage, static_cast<uint8>(CarriageConfig[i].Key));
				}
				Carriages.Add(Carriage);
			}
		}
	}

	UE_LOG(LogBreachborne, Log, TEXT("Train: Spawned %d carriages (mode=%s)"),
		Carriages.Num(), TrainMode == ETrainMode::BulletTrains ? TEXT("BulletTrains") : TEXT("Normal"));
}

void ABBTrain::ApplyDefaultCarriageVisuals(ABBTrainCarriage* Carriage, uint8 CarriageTypeByte)
{
	if (!Carriage)
	{
		return;
	}

	UStaticMeshComponent* Mesh = Carriage->GetCarriageMesh();
	if (!Mesh)
	{
		return;
	}

	// Get default meshes — loaded once via StaticLoadObject (ConstructorHelpers only works in constructors)
	static UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	static UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// All carriages are flat walkable platforms (like Supervive train carts)
	// Scale: X = length along track, Y = width across track, Z = thickness (flat floor)
	FVector Scale = FVector(6.0f, 4.0f, 0.3f);
	FLinearColor Color = FLinearColor::White;

	const ETrainCarriageType Type = static_cast<ETrainCarriageType>(CarriageTypeByte);
	switch (Type)
	{
	case ETrainCarriageType::Locomotive:
		Mesh->SetStaticMesh(CubeMesh);
		Scale = FVector(8.0f, 8.0f, 0.3f); // 800cm long, 800cm wide
		Color = FLinearColor(0.9f, 0.1f, 0.1f); // Red
		break;

	case ETrainCarriageType::Platform:
		Mesh->SetStaticMesh(CubeMesh);
		Scale = FVector(4.0f, 8.0f, 0.3f); // 400cm long, 800cm wide
		Color = FLinearColor(0.3f, 0.3f, 0.35f); // Dark grey
		break;

	case ETrainCarriageType::Chest:
		Mesh->SetStaticMesh(CubeMesh);
		Scale = FVector(4.0f, 8.0f, 0.3f); // 400cm long, 800cm wide
		Color = FLinearColor(0.8f, 0.6f, 0.1f); // Gold
		break;

	case ETrainCarriageType::CircleImmune:
		Mesh->SetStaticMesh(CubeMesh);
		Scale = FVector(4.0f, 8.0f, 0.3f); // 400cm long, 800cm wide
		Color = FLinearColor(0.1f, 0.7f, 0.9f); // Cyan glow
		break;

	case ETrainCarriageType::ReviveBeacon:
		Mesh->SetStaticMesh(CubeMesh);
		Scale = FVector(4.0f, 8.0f, 0.3f); // 400cm long, 800cm wide
		Color = FLinearColor(0.2f, 0.9f, 0.3f); // Green
		break;
	}

	Mesh->SetRelativeScale3D(Scale);

	// Size SideCollision to match the mesh footprint, positioned so FindFloor can hit it
	if (Carriage->SideCollision)
	{
		Carriage->SideCollision->SetAbsolute(false, false, true);
		float HalfLength = Scale.X * 50.0f;
		float HalfWidth = Scale.Y * 50.0f;
		float Thickness = Scale.Z * 100.0f;
		// Tall box: platform top (Thickness/2) up to above capsule center (~100cm)
		Carriage->SideCollision->SetBoxExtent(FVector(HalfLength, HalfWidth, 50.0f));
		Carriage->SideCollision->SetRelativeLocation(FVector(0.0f, 0.0f, Thickness / 2.0f + 50.0f));
	}

	UE_LOG(LogBreachborne, Log, TEXT("Train: Applied default visuals to %s — Scale=%s, Color=%s"),
		*Carriage->GetName(), *Scale.ToString(), *Color.ToString());

	// Create a dynamic material instance to color the mesh
	if (Mesh->GetStaticMesh())
	{
		UMaterialInterface* BaseMaterial = Mesh->GetMaterial(0);
		if (!BaseMaterial)
		{
			// Try to get material from the mesh's first slot
			BaseMaterial = Mesh->GetStaticMesh()->GetMaterial(0);
		}
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMaterial, Carriage);
			if (DynMat)
			{
				// Try common parameter names — BasicShapeMaterial may use different names
				static const TArray<FName> ColorParamNames = {
					TEXT("Color"), TEXT("BaseColor"), TEXT("Base Color"), TEXT("Tint"), TEXT("EmissiveColor")
				};

				bool bColorApplied = false;
				for (FName ParamName : ColorParamNames)
				{
					FLinearColor DummyValue;
					if (DynMat->GetVectorParameterValue(ParamName, DummyValue))
					{
						DynMat->SetVectorParameterValue(ParamName, Color);
						bColorApplied = true;
						UE_LOG(LogBreachborne, Log, TEXT("Train: Default visuals color applied via '%s' = %s"),
							*ParamName.ToString(), *Color.ToString());
						break;
					}
				}

				if (!bColorApplied)
				{
					TArray<FMaterialParameterInfo> ParamInfos;
					TArray<FGuid> ParamIds;
					DynMat->GetAllVectorParameterInfo(ParamInfos, ParamIds);
					for (const FMaterialParameterInfo& Info : ParamInfos)
					{
						UE_LOG(LogBreachborne, Log, TEXT("Train: Available color param: '%s'"), *Info.Name.ToString());
					}
					UE_LOG(LogBreachborne, Warning, TEXT("Train: No color parameter found on material %s"), *BaseMaterial->GetName());
				}

				Mesh->SetMaterial(0, DynMat);
			}
		}
	}
}

void ABBTrain::DestroyCarriages()
{
	for (ABBTrainCarriage* Carriage : Carriages)
	{
		if (Carriage)
		{
			Carriage->Destroy();
		}
	}
	Carriages.Empty();
}

void ABBTrain::AdvanceTrackProgress(float DeltaTime)
{
	if (!bIsMoving || !CurrentTrack)
	{
		return;
	}

	const float TrackLength = CurrentTrack->GetTrackLength();
	if (TrackLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (!bTestFreezeProgress)
	{
		const float DeltaDistance = Speed * DeltaTime;
		TrackProgress = CurrentTrack->AdvanceProgress(TrackProgress, DeltaDistance, bMovingForward);
	}
}

void ABBTrain::UpdateCarriagePositions(float Progress)
{
	for (ABBTrainCarriage* Carriage : Carriages)
	{
		if (Carriage)
		{
			Carriage->UpdatePosition(Progress, CurrentTrack);
		}
	}
}

void ABBTrain::UpdateRiders()
{
	// Capture existing rider offsets and detect new riders for all carriages
	for (ABBTrainCarriage* Carriage : Carriages)
	{
		if (Carriage)
		{
			Carriage->CaptureRiderOffsets();
			Carriage->DetectNewRiders();
		}
	}

	// CRITICAL: A character must only be a rider on ONE carriage.
	// Detection spheres overlap between adjacent carriages, so a character
	// can be detected by multiple carriages. Assign to the first carriage
	// that claims them (front-most carriage wins).
	TSet<ACharacter*> LocomotiveBlockedRiders;
	for (ABBTrainCarriage* Carriage : Carriages)
	{
		if (!Carriage || Carriage->GetCarriageType() != ETrainCarriageType::Locomotive)
		{
			continue;
		}

		for (ABBTrainCarriage* CandidateCarriage : Carriages)
		{
			if (!CandidateCarriage)
			{
				continue;
			}

			for (const auto& Pair : CandidateCarriage->RiderLocalOffsets)
			{
				ACharacter* Character = Pair.Key.Get();
				if (Character && Carriage->IsCharacterInLocomotiveNoRideZone(Character))
				{
					LocomotiveBlockedRiders.Add(Character);
				}
			}
		}
	}

	if (!LocomotiveBlockedRiders.IsEmpty())
	{
		for (ABBTrainCarriage* Carriage : Carriages)
		{
			if (!Carriage)
			{
				continue;
			}

			for (ACharacter* Character : LocomotiveBlockedRiders)
			{
				Carriage->RiderLocalOffsets.Remove(Character);
			}
		}
	}

	TSet<ACharacter*> AssignedRiders;
	for (ABBTrainCarriage* Carriage : Carriages)
	{
		if (!Carriage)
		{
			continue;
		}
		for (auto It = Carriage->RiderLocalOffsets.CreateIterator(); It; ++It)
		{
			ACharacter* Character = It->Key.Get();
			if (!Character)
			{
				It.RemoveCurrent();
				continue;
			}
			if (AssignedRiders.Contains(Character))
			{
				// Already assigned to an earlier carriage — remove from this one
				It.RemoveCurrent();
				UE_LOG(LogBreachborne, Verbose, TEXT("Train: Rider conflict resolved — %s stays on earlier carriage, removed from %s"),
					*Character->GetName(), *Carriage->GetName());
			}
			else
			{
				AssignedRiders.Add(Character);
			}
		}
	}

	// Pre-position riders for next frame's CMC tick
	for (ABBTrainCarriage* Carriage : Carriages)
	{
		if (Carriage)
		{
			Carriage->PrePositionRiders();
		}
	}
}
