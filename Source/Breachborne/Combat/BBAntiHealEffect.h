#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBAntiHealEffect.generated.h"

/**
 * Shared anti-heal GameplayEffect.
 * Applies State.AntiHeal and reduces UBBHealthSet::HealingReceivedMultiplier by 50%.
 * Callers can override duration per spec.
 */
UCLASS()
class BREACHBORNE_API UBBAntiHealEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBAntiHealEffect();
};
