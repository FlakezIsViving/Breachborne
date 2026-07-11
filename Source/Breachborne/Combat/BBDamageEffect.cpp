#include "BBDamageEffect.h"
#include "BBDamageExecution.h"
#include "Breachborne/Abilities/BBGameplayTags.h"

UBBDamageEffect::UBBDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Add the damage execution calculation
	FGameplayEffectExecutionDefinition ExecutionDef;
	ExecutionDef.CalculationClass = UBBDamageExecution::StaticClass();
	Executions.Add(ExecutionDef);
}
