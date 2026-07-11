#include "BBDynamicStatGE.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Abilities/BBCombatSet.h"
#include "Breachborne/Abilities/BBMovementSet.h"
#include "Breachborne/Abilities/BBGameplayTags.h"

UBBDynamicStatGE::UBBDynamicStatGE()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// Helper: add an additive modifier driven by a SetByCaller tag.
	// All 7 modifiers always exist on the CDO. Callers set magnitudes on the spec;
	// unset tags default to 0 (additive +0 = no change).
	auto AddSetByCallerMod = [this](const FGameplayAttribute& Attribute, const FGameplayTag& DataTag)
	{
		FGameplayModifierInfo& Mod = Modifiers.AddDefaulted_GetRef();
		Mod.Attribute = Attribute;
		Mod.ModifierOp = EGameplayModOp::Additive;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = DataTag;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	};

	// HealthSet
	AddSetByCallerMod(UBBHealthSet::GetMaxHealthAttribute(), BBGameplayTags::SetByCaller_MaxHealth);
	AddSetByCallerMod(UBBHealthSet::GetMaxShieldAttribute(), BBGameplayTags::SetByCaller_MaxShield);

	// CombatSet
	AddSetByCallerMod(UBBCombatSet::GetAttackPowerAttribute(), BBGameplayTags::SetByCaller_AttackPower);
	AddSetByCallerMod(UBBCombatSet::GetAbilityPowerAttribute(), BBGameplayTags::SetByCaller_AbilityPower);
	AddSetByCallerMod(UBBCombatSet::GetCritChanceAttribute(), BBGameplayTags::SetByCaller_CritChance);
	AddSetByCallerMod(UBBCombatSet::GetCooldownReductionAttribute(), BBGameplayTags::SetByCaller_CooldownReduction);

	// MovementSet
	AddSetByCallerMod(UBBMovementSet::GetMoveSpeedAttribute(), BBGameplayTags::SetByCaller_MoveSpeed);
	AddSetByCallerMod(UBBMovementSet::GetGlideSpeedAttribute(), BBGameplayTags::SetByCaller_GlideSpeed);
}
