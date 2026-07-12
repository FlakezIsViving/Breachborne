#include "BBDamageExecution.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBCombatSet.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Items/BBInventoryManager.h"

// Declare attribute capture definitions
struct FBBDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(AbilityPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritMultiplier);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);

	FBBDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBBCombatSet, AttackPower, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBBCombatSet, AbilityPower, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBBCombatSet, CritChance, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBBCombatSet, CritMultiplier, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBBHealthSet, Armor, Target, false);
	}
};

static const FBBDamageStatics& DamageStatics()
{
	static FBBDamageStatics Statics;
	return Statics;
}

UBBDamageExecution::UBBDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().AbilityPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().CritChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CritMultiplierDef);
	RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
}

void UBBDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// Get base damage from SetByCaller
	float BaseDamage = Spec.GetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, false, 0.0f);
	if (BaseDamage <= 0.0f)
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("DamageExecution: BaseDamage is %.1f — aborting"), BaseDamage);
		return;
	}

	// Capture source attributes
	float AttackPower = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, FAggregatorEvaluateParameters(), AttackPower);

	float CritChance = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CritChanceDef, FAggregatorEvaluateParameters(), CritChance);

	float CritMultiplier = 1.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CritMultiplierDef, FAggregatorEvaluateParameters(), CritMultiplier);

	// Scale base damage by attack power (simple linear for Phase 2 — armor/resistance added in Phase 5)
	float FinalDamage = BaseDamage + (AttackPower * 0.1f);

	// Crit check
	bool bWasCrit = false;
	if (CritChance > 0.0f)
	{
		const float Roll = FMath::FRand();
		if (Roll < CritChance)
		{
			FinalDamage *= CritMultiplier;
			bWasCrit = true;
		}
	}

	// Armor: flat damage reduction from target's Armor attribute
	float TargetArmor = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef, FAggregatorEvaluateParameters(), TargetArmor);

	if (TargetArmor > 0.0f)
	{
		if (UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent())
		{
			if (ABreachbornePlayerState* TargetPS = Cast<ABreachbornePlayerState>(TargetASC->GetOwnerActor()))
			{
				FBBInventoryManager::ApplyArmorDurabilityDamage(TargetPS, FMath::Max(1.0f, FinalDamage));
			}
		}
		FinalDamage -= TargetArmor;
	}

	bool bTargetVulnerable = false;
	if (const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent())
	{
		bTargetVulnerable = TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Vulnerable);
		if (bTargetVulnerable)
		{
			FinalDamage *= 2.0f;
		}

		if (TargetASC->HasMatchingGameplayTag(BBGameplayTags::Ability_Hunter_Hudson_Passive))
		{
			const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
			const AActor* SourceAvatar = SourceASC ? SourceASC->GetAvatarActor() : Spec.GetEffectContext().GetInstigator();
			AHunterCharacter* TargetHunter = Cast<AHunterCharacter>(TargetASC->GetAvatarActor());
			if (SourceAvatar && TargetHunter)
			{
				const FVector ToSource = (SourceAvatar->GetActorLocation() - TargetHunter->GetActorLocation()).GetSafeNormal2D();
				const FVector TargetForward = TargetHunter->GetActorForwardVector().GetSafeNormal2D();
				const bool bHitFromBack = FVector::DotProduct(TargetForward, ToSource) < -0.35f;
				if (bHitFromBack)
				{
					FinalDamage *= 1.25f;
				}
			}
		}
	}

	// Minimum 1 damage (armor cannot fully negate an attack)
	FinalDamage = FMath::Max(FinalDamage, 1.0f);

	UE_LOG(LogBreachborne, Verbose, TEXT("DamageExecution: Base=%.1f | AtkPwr=%.1f(+%.1f) | Crit=%.0f%%(%s) | Armor=%.0f | Vulnerable=%s | Final=%.1f → IncomingDamage"),
		BaseDamage, AttackPower, AttackPower * 0.1f,
		CritChance * 100.0f, bWasCrit ? TEXT("CRIT!") : TEXT("no"),
		TargetArmor, bTargetVulnerable ? TEXT("YES") : TEXT("no"), FinalDamage);

	// Output to target's IncomingDamage meta attribute
	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(
			UBBHealthSet::GetIncomingDamageAttribute(),
			EGameplayModOp::Additive,
			FinalDamage
		)
	);
}
