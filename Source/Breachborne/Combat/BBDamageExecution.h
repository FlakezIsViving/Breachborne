#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "BBDamageExecution.generated.h"

/**
 * Damage execution calculation. The ONLY path damage flows through in Breachborne.
 * Captures source AttackPower/AbilityPower/CritChance/CritMultiplier and applies
 * a SetByCaller base damage through the crit pipeline, outputting to target's IncomingDamage.
 */
UCLASS()
class BREACHBORNE_API UBBDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UBBDamageExecution();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
