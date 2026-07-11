#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StormTypes.h"
#include "StormShiftBase.generated.h"

/**
 * Abstract base for storm shift mutators.
 * Override virtuals to customize phase configs, center selection, and damage.
 * The default implementations provide a minimal single-phase storm that
 * shrinks to center — subclass to create named shifts (Nomadic, BulletTrains, etc.).
 */
UCLASS(Abstract, Blueprintable)
class BREACHBORNE_API UStormShiftBase : public UObject
{
	GENERATED_BODY()

public:
	/** Display name for this shift (used in logs and future UI). */
	virtual FName GetShiftName() const { return FName(TEXT("Default")); }

	/**
	 * Return the array of phase configs for this shift.
	 * @param InitialRadius The starting radius of the storm circle.
	 */
	virtual TArray<FStormPhaseConfig> GetPhaseConfigs(float InitialRadius) const
	{
		FStormPhaseConfig SinglePhase;
		SinglePhase.TargetRadiusFraction = 0.0f;
		SinglePhase.ShrinkDuration = 60.0f;
		SinglePhase.HoldDuration = 0.0f;
		SinglePhase.DamagePerTick = 10.0f;
		SinglePhase.DamageTickInterval = 1.0f;
		return { SinglePhase };
	}

	/**
	 * Choose where the next safe zone center should be.
	 * @param PhaseIndex      Which phase is about to start.
	 * @param CurrentCenter   The current circle center.
	 * @param NewRadius       The radius the circle will shrink to.
	 * @return                World-space 2D center for the next circle.
	 */
	virtual FVector2D PickNextCenter(int32 PhaseIndex, const FVector2D& CurrentCenter, float NewRadius) const
	{
		return CurrentCenter;
	}

	/** Called when a new phase starts. Override for shift-specific events. */
	virtual void OnPhaseStarted(int32 PhaseIndex) {}

	/**
	 * Modify damage before it's applied to a hunter outside the circle.
	 * @param BaseDamage    The phase's configured DamagePerTick.
	 * @param PhaseIndex    Current phase index.
	 * @return              Modified damage value.
	 */
	virtual float ModifyDamage(float BaseDamage, int32 PhaseIndex) const
	{
		return BaseDamage;
	}

	/** Apply shift-specific effects when a phase begins */
	virtual void ApplyShiftEffects(UWorld* World, int32 PhaseIndex) {}

	/** Remove shift-specific effects when the shift ends */
	virtual void RemoveShiftEffects(UWorld* World) {}
};
