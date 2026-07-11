#pragma once

#include "CoreMinimal.h"
#include "StormTypes.generated.h"

/** Current state within a storm phase. */
UENUM(BlueprintType)
enum class EStormPhaseState : uint8
{
	Waiting,    // Storm hasn't started yet
	Shrinking,  // Circle is closing toward the target
	Holding,    // Circle is stationary between shrinks
	Finished    // All phases complete, damage continues at final rate
};

/** Configuration for a single storm phase. */
USTRUCT(BlueprintType)
struct FStormPhaseConfig
{
	GENERATED_BODY()

	/** Target radius as a fraction (0.0-1.0) of initial radius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Storm")
	float TargetRadiusFraction = 1.0f;

	/** Seconds for the circle to shrink from current to target radius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Storm")
	float ShrinkDuration = 60.0f;

	/** Seconds to hold after shrinking before the next phase begins. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Storm")
	float HoldDuration = 30.0f;

	/** Damage applied per tick to hunters outside the safe zone. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Storm")
	float DamagePerTick = 5.0f;

	/** Seconds between damage ticks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Storm")
	float DamageTickInterval = 1.0f;
};
