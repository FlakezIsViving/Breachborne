#include "StormShift_Default.h"

TArray<FStormPhaseConfig> UStormShift_Default::GetPhaseConfigs(float InitialRadius) const
{
	TArray<FStormPhaseConfig> Phases;

	// Phase 1: 75% of initial radius
	{
		FStormPhaseConfig Phase;
		Phase.TargetRadiusFraction = 0.75f;
		Phase.ShrinkDuration = 60.0f;
		Phase.HoldDuration = 30.0f;
		Phase.DamagePerTick = 5.0f;
		Phase.DamageTickInterval = 1.0f;
		Phases.Add(Phase);
	}

	// Phase 2: 50% of initial radius
	{
		FStormPhaseConfig Phase;
		Phase.TargetRadiusFraction = 0.50f;
		Phase.ShrinkDuration = 45.0f;
		Phase.HoldDuration = 25.0f;
		Phase.DamagePerTick = 10.0f;
		Phase.DamageTickInterval = 1.0f;
		Phases.Add(Phase);
	}

	// Phase 3: 25% of initial radius
	{
		FStormPhaseConfig Phase;
		Phase.TargetRadiusFraction = 0.25f;
		Phase.ShrinkDuration = 30.0f;
		Phase.HoldDuration = 20.0f;
		Phase.DamagePerTick = 20.0f;
		Phase.DamageTickInterval = 1.0f;
		Phases.Add(Phase);
	}

	// Phase 4: full collapse
	{
		FStormPhaseConfig Phase;
		Phase.TargetRadiusFraction = 0.0f;
		Phase.ShrinkDuration = 20.0f;
		Phase.HoldDuration = 0.0f;
		Phase.DamagePerTick = 40.0f;
		Phase.DamageTickInterval = 0.5f;
		Phases.Add(Phase);
	}

	return Phases;
}

FVector2D UStormShift_Default::PickNextCenter(int32 PhaseIndex, const FVector2D& CurrentCenter, float NewRadius) const
{
	// Default shift: always shrink toward map center (0,0)
	return FVector2D::ZeroVector;
}
