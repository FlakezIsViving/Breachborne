#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBDamageEffect.generated.h"

/**
 * Shared instant damage GameplayEffect. Used by all damage-dealing abilities.
 * Base damage is parameterized via SetByCaller (tag: SetByCaller.Damage).
 * Damage calculation runs through UBBDamageExecution.
 */
UCLASS()
class BREACHBORNE_API UBBDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBDamageEffect();
};
