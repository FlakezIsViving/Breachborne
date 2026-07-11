#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBTrain.generated.h"

class USplineComponent;
class ABBTrainTrack;
class ABBTrainCarriage;
class UStaticMesh;

UENUM(BlueprintType)
enum class ETrainMode : uint8
{
	Normal      UMETA(DisplayName = "Normal"),      // 1 train, 3 platforms + chest + circle immune + revive beacon
	BulletTrains UMETA(DisplayName = "Bullet Trains") // 3 mini trains, 1 platform each, faster speed
};

/**
 * The main train actor that moves along a BBTrainTrack spline.
 *
 * Server-authoritative movement. Replicates TrackProgress and Speed to clients.
 * Clients interpolate for smooth visuals.
 *
 * Carriages are child actors positioned at offsets along the track.
 */
UCLASS()
class BREACHBORNE_API ABBTrain : public AActor
{
	GENERATED_BODY()

public:
	ABBTrain();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Initialize the train on a specific track */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Train")
	void InitializeTrain(ABBTrainTrack* Track, ETrainMode Mode);

	/** Start moving */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Train")
	void StartMoving();

	/** Stop moving */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Train")
	void StopMoving();

	/** Set the train's speed (units/sec along spline) */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Train")
	void SetSpeed(float NewSpeed);

	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	float GetSpeed() const { return Speed; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	float GetTrackProgress() const { return TrackProgress; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	bool IsMoving() const { return bIsMoving; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	ABBTrainTrack* GetCurrentTrack() const { return CurrentTrack; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	const TArray<ABBTrainCarriage*>& GetCarriages() const { return Carriages; }

	/** TEST: Put riders in MOVE_Flying to eliminate CMC FindFloor micro-adjustments */
	UPROPERTY(EditAnywhere, Category = "Breachborne|Train|Debug")
	bool bTestFlyingModeForRiders = false;

	/** Visual smoothing speed (0 = disabled). Higher = tighter tracking, lower = smoother glide.
	 *  Default 125 gives ~8ms / 6.4cm of constant lag — imperceptible on a train,
	 *  but completely masks frame-to-frame displacement jitter. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Train")
	float VisualSmoothingSpeed = 125.0f;

protected:
	/** The track this train is following */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<ABBTrainTrack> CurrentTrack;

	/** Current progress along the track (0.0 = start, 1.0 = end) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Train")
	float TrackProgress = 0.0f;

	/** Speed in units per second along the spline */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Train")
	float Speed = 800.0f;

	/** Whether the train is currently moving */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Train")
	bool bIsMoving = false;

	/** Direction: true = forward (clockwise), false = backward */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Train")
	bool bMovingForward = true;

	/** Mode: Normal or BulletTrains */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Train")
	ETrainMode TrainMode = ETrainMode::Normal;

	/** Normal speed (units/sec) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train")
	float NormalSpeed = 800.0f;

	/** BulletTrains speed multiplier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train")
	float BulletTrainsSpeedMultiplier = 2.5f;

	/** Mesh for the locomotive */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	TObjectPtr<UStaticMesh> LocomotiveMesh;

	/** Mesh for platform carriages */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	TObjectPtr<UStaticMesh> PlatformMesh;

	/** Mesh for chest carriage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	TObjectPtr<UStaticMesh> ChestMesh;

	/** Mesh for circle immune carriage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	TObjectPtr<UStaticMesh> CircleImmuneMesh;

	/** Mesh for revive beacon carriage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	TObjectPtr<UStaticMesh> ReviveBeaconMesh;

	/** Length of each carriage in world units */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train")
	float CarriageLength = 400.0f;

	/** Gap between carriages */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train")
	float CarriageGap = 300.0f;

private:
	/** All carriages attached to this train */
	UPROPERTY()
	TArray<TObjectPtr<ABBTrainCarriage>> Carriages;

	/** Server: spawn carriages based on train mode */
	void SpawnCarriages();

	/** Server: destroy all carriages */
	void DestroyCarriages();

	/** Apply default primitive mesh + color based on carriage type */
	void ApplyDefaultCarriageVisuals(ABBTrainCarriage* Carriage, uint8 CarriageTypeByte);

	/** Advance TrackProgress by Speed * DeltaTime */
	void AdvanceTrackProgress(float DeltaTime);

	/** Update all carriage positions to the given progress */
	void UpdateCarriagePositions(float Progress);

	/** Capture riders, resolve multi-carriage conflicts, pre-position for next CMC tick */
	void UpdateRiders();

	/** Client: smooth interpolation toward server TrackProgress */
	float ClientTrackProgress = 0.0f;
	float ClientInterpolationSpeed = 5.0f;

	/** Smoothed delta time to eliminate visual jitter from frame pacing variation */
	float SmoothedDeltaTime = 1.0f / 60.0f;

	/** How aggressively to smooth DeltaTime (0 = never adapt, 1 = instant). 0.15 is a good balance. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Train")
	float DeltaTimeSmoothingFactor = 0.15f;

	/** If true, use SmoothedDeltaTime for movement. Disable to test raw DeltaTime. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Train")
	bool bUseSmoothedDeltaTime = true;

	/** TEST: Hardcode DeltaTime to exactly 1/60s to eliminate frame pacing entirely */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Train|Debug")
	bool bTestFixed60HzDelta = false;

	/** TEST: Freeze train progress — carriages still evaluate spline but progress never changes */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Train|Debug")
	bool bTestFreezeProgress = false;

	/** If true, use fixed timestep with visual interpolation for perfectly smooth movement */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Train")
	bool bUseFixedTimeStep = true;

	/** Smoothed visual progress for frame-rate-independent glide */
	float SmoothedVisualProgress = 0.0f;

	/** Fixed timestep rate (default 60Hz) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Train")
	float FixedTimeStep = 1.0f / 60.0f;

	/** Accumulator for fixed timestep */
	float TimeAccumulator = 0.0f;

	/** True once SmoothedVisualProgress has been initialized to TrackProgress */
	bool bVisualSmoothingInitialized = false;

	/** Periodic accumulator for delta-time diagnostic logging */
	float DeltaLogAccumulator = 0.0f;
};
