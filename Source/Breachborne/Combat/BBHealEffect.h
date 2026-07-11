#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBHealEffect.generated.h"

/**
 * Shared instant heal GameplayEffect. Used by all healing abilities.
 * Heal amount is parameterized via SetByCaller (tag: SetByCaller.HealAmount).
 * Applies a direct health modifier to the target's Health attribute.
 */
UCLASS()
class BREACHBORNE_API UBBHealEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBHealEffect();
};
