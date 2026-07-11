#include "BBHealEffect.h"
#include "BBHealExecution.h"

UBBHealEffect::UBBHealEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Add the heal execution calculation
	FGameplayEffectExecutionDefinition ExecutionDef;
	ExecutionDef.CalculationClass = UBBHealExecution::StaticClass();
	Executions.Add(ExecutionDef);
}
