#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Breachborne/Core/BreachborneTypes.h"
#include "BBCharacterMovementComponent.generated.h"

USTRUCT(BlueprintType)
struct FBBMantleTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Movement|Mantling")
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Movement|Mantling")
	FVector LandingLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Movement|Mantling")
	float LandingFloorZ = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Movement|Mantling")
	FVector OutwardNormal = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Movement|Mantling")
	FVector InwardDirection = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Movement|Mantling")
	float Duration = 0.45f;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Movement|Mantling")
	float StartTime = 0.0f;

	bool IsValid() const
	{
		return !StartLocation.IsNearlyZero()
			&& !LandingLocation.IsNearlyZero()
			&& Duration > KINDA_SMALL_NUMBER;
	}
};

/**
 * Custom CharacterMovementComponent for Breachborne hunters.
 * Adds custom movement modes for Gliding (reduced gravity, horizontal flight)
 * and Spiked (full gravity, no player control, downward velocity boost).
 * The CMC handles movement physics; GliderComponent manages the state machine.
 * Movement mode replication is handled automatically by CMC.
 */
UCLASS()
class BREACHBORNE_API UBBCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UBBCharacterMovementComponent();

	// --- Custom Movement Mode Accessors ---

	/** Check if currently in the Gliding custom movement mode */
	bool IsGliding() const;

	/** Check if currently in the Spiked custom movement mode */
	bool IsSpiked() const;

	/** Check if currently in the Mantling custom movement mode */
	bool IsMantling() const { return MovementMode == MOVE_Custom && CustomMovementMode == static_cast<uint8>(EBBCustomMovementMode::Mantling); }

	/** Enter mantling movement mode. WallNormal faces outward from the wall; GroundZ is the target height to reach. */
	void EnterMantling(const FBBMantleTarget& InTarget);

	/** Exit mantling — applies hop impulse and returns to Falling */
	void FinishMantling();

	void AbortMantling();

	/** Enter gliding movement mode (called by GliderComponent) */
	void EnterGliding();

	/** Exit gliding back to falling */
	void ExitGliding();

	/** Enter spiked movement mode (called by GliderComponent) */
	void EnterSpiked();

	/** Exit spiked back to falling */
	void ExitSpiked();

protected:
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual float GetMaxSpeed() const override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

private:
	/** Gliding physics: reduced gravity, horizontal movement from input */
	void PhysGliding(float DeltaTime, int32 Iterations);

	bool FindNearbyWalkableGlideTargetZ(const FVector& CurrentLocation, float& OutTargetZ) const;

	/** Spiked physics: full gravity + downward boost, no player control */
	void PhysSpiked(float DeltaTime, int32 Iterations);

	/** Mantle physics: swept diagonal recovery toward a validated landing target */
	void PhysMantling(float DeltaTime, int32 Iterations);

	// --- Tuning ---

	/** Gravity scale while gliding (fraction of normal gravity). Lower = floatier. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlidingGravityScale = 0.15f;

	/** Max horizontal speed while gliding */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlidingMaxSpeed = 800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideTopSearchHeight = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideTopSearchDepth = 700.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideRecoveryClearance = 220.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideNearbyGroundSampleRadius = 1200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideNearbyGroundSampleForwardBias = 550.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideRecoveryImpulse = 1400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideSoftCeilingOffset = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideSoftCeilingPullStrength = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideHeightCorrectionSpeed = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideVerticalVelocityInterpSpeed = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideSolidGroundSinkSpeed = 70.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideGroundProbeDistance = 1600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideMaxRiseSpeed = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlideMaxFallSpeed = 240.0f;

	/** Time in seconds to accelerate from zero to GlidingMaxSpeed (linear ramp) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlidingAccelerationTime = 1.0f;

	/** Deceleration rate (units/sec) when turning more than 90 degrees while gliding.
	 *  Character must brake through ~zero speed before accelerating in the new direction. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float GlidingSharpTurnBraking = 1200.0f;

	/** Speed threshold below which a sharp turn is considered complete and normal acceleration resumes */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Gliding")
	float SharpTurnSpeedThreshold = 50.0f;

	/** Additional downward velocity applied when spiked */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Spiked")
	float SpikedDownwardBoost = -800.0f;

	// --- Mantle Tuning ---

	/** Extra center height above the validated walkable top target for mantle recovery. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Mantling")
	float MantleBoostExtraHeight = 140.0f;

	/** Inward horizontal speed applied by mantle recovery. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Mantling")
	float MantleBoostInwardSpeed = 420.0f;

	/** Minimum upward speed from mantle recovery. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Mantling")
	float MantleMinUpwardBoostSpeed = 900.0f;

	/** Maximum upward speed from mantle recovery. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Mantling")
	float MantleMaxUpwardBoostSpeed = 1700.0f;

	/** Extra impulse applied to the physically required vertical speed for a sharper push-off feel. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Mantling")
	float MantleInitialKickMultiplier = 1.18f;

	/** Max time before forced mantle exit (safety) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Movement|Mantling")
	float MantleMaxDuration = 1.25f;

private:
	FBBMantleTarget MantleTarget;
	FVector MantlePreviousPathLocation = FVector::ZeroVector;
	float GlideTargetZ = 0.0f;
	bool bHasGlideTargetZ = false;
};
