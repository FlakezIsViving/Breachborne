#include "BBHealExecution.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"

struct FBBHealStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(HealingReceivedMultiplier);

	FBBHealStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBBHealthSet, HealingReceivedMultiplier, Target, false);
	}
};

static const FBBHealStatics& HealStatics()
{
	static FBBHealStatics Statics;
	return Statics;
}

UBBHealExecution::UBBHealExecution()
{
	RelevantAttributesToCapture.Add(HealStatics().HealingReceivedMultiplierDef);
}

void UBBHealExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	float HealAmount = Spec.GetSetByCallerMagnitude(BBGameplayTags::SetByCaller_HealAmount, false, 0.0f);
	if (HealAmount <= 0.0f)
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("HealExecution: HealAmount is %.1f - aborting"), HealAmount);
		return;
	}

	float HealingReceivedMultiplier = 1.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		HealStatics().HealingReceivedMultiplierDef,
		FAggregatorEvaluateParameters(),
		HealingReceivedMultiplier);

	const float FinalHealAmount = HealAmount * FMath::Clamp(HealingReceivedMultiplier, 0.0f, 1.0f);
	if (FinalHealAmount <= 0.0f)
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("HealExecution: %.1f healing reduced to 0 by anti-heal"), HealAmount);
		return;
	}

	UE_LOG(LogBreachborne, Log, TEXT("HealExecution: %.1f healing x %.2f = %.1f -> IncomingHealing"),
		HealAmount, HealingReceivedMultiplier, FinalHealAmount);

	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(
			UBBHealthSet::GetIncomingHealingAttribute(),
			EGameplayModOp::Additive,
			FinalHealAmount
		)
	);
}
