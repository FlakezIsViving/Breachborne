#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBDazeEffect.generated.h"

/**
 * Daze debuff GameplayEffect — applied by Ghost's Spike Grenade (RMB).
 *
 * Grants State.Dazed tag and reduces MoveSpeed by 40% for DazeDuration seconds.
 * Unlike State.Stunned, daze does NOT block combat ability activation — the target
 * can still fight, just moves slower.
 *
 * Duration is set via SetByCaller.DazeDuration by the applying ability.
 * If the SetByCaller magnitude is not set, the effect falls back to 2.0s via ScalableFloat.
 */
UCLASS()
class BREACHBORNE_API UBBDazeEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBDazeEffect();
};
