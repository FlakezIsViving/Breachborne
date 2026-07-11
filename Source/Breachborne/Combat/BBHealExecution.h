#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "BBHealExecution.generated.h"

/**
 * Heal execution calculation. Simple execution that outputs a SetByCaller heal amount
 * directly to the target's IncomingHealing meta attribute.
 */
UCLASS()
class BREACHBORNE_API UBBHealExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UBBHealExecution();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
