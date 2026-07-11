#include "BBEquipmentGEHelper.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayAbilitySpec.h"
#include "BBItemTypes.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Abilities/BBCombatSet.h"
#include "Breachborne/Abilities/BBMovementSet.h"
#include "Breachborne/Breachborne.h"

FActiveGameplayEffectHandle FBBEquipmentGEHelper::ApplyStatBonuses(
	UAbilitySystemComponent* ASC,
	const FBBStatBonuses& Stats,
	const FGameplayTag& EffectTag,
	const FString& DebugName)
{
	if (!ASC)
	{
		return FActiveGameplayEffectHandle();
	}

	if (Stats.IsZero())
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("EquipmentGEHelper: Skipping %s — all stat bonuses are zero"), *DebugName);
		return FActiveGameplayEffectHandle();
	}

	// Create a programmatic Infinite-duration GE (same pattern as StormManager's damage GE)
	UGameplayEffect* GE = NewObject<UGameplayEffect>(ASC->GetOwner(), *FString::Printf(TEXT("GE_%s"), *DebugName));
	GE->DurationPolicy = EGameplayEffectDurationType::Infinite;

	// Tag the effect for identification
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(EffectTag);
	GE->InheritableOwnedTagsContainer = TagContainer;

	// Add modifiers for each non-zero stat bonus
	auto AddModifier = [GE](const FGameplayAttribute& Attribute, float Value)
	{
		if (!FMath::IsNearlyZero(Value))
		{
			FGameplayModifierInfo& Mod = GE->Modifiers.AddDefaulted_GetRef();
			Mod.Attribute = Attribute;
			Mod.ModifierOp = EGameplayModOp::Additive;

			FScalableFloat ScalableValue;
			ScalableValue.Value = Value;
			Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(ScalableValue);
		}
	};

	// HealthSet
	AddModifier(UBBHealthSet::GetMaxHealthAttribute(), Stats.MaxHealth);
	AddModifier(UBBHealthSet::GetMaxShieldAttribute(), Stats.MaxShield);

	// CombatSet
	AddModifier(UBBCombatSet::GetAttackPowerAttribute(), Stats.AttackPower);
	AddModifier(UBBCombatSet::GetAbilityPowerAttribute(), Stats.AbilityPower);
	AddModifier(UBBCombatSet::GetCritChanceAttribute(), Stats.CritChance);
	AddModifier(UBBCombatSet::GetCooldownReductionAttribute(), Stats.CooldownReduction);

	// MovementSet
	AddModifier(UBBMovementSet::GetMoveSpeedAttribute(), Stats.MoveSpeed);

	// Apply the GE
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(ASC->GetOwner());

	FGameplayEffectSpec Spec(GE, Context, 1.0f);
	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(Spec);

	UE_LOG(LogBreachborne, Log, TEXT("EquipmentGEHelper: Applied %s (AtkPwr=%.0f, AbPwr=%.0f, MaxHP=%.0f, MaxShield=%.0f, MoveSpd=%.0f, Crit=%.2f, CDR=%.2f)"),
		*DebugName,
		Stats.AttackPower, Stats.AbilityPower,
		Stats.MaxHealth, Stats.MaxShield,
		Stats.MoveSpeed, Stats.CritChance, Stats.CooldownReduction);

	return Handle;
}

void FBBEquipmentGEHelper::RemoveStatBonuses(UAbilitySystemComponent* ASC, FActiveGameplayEffectHandle& Handle)
{
	if (!ASC || !Handle.IsValid())
	{
		return;
	}

	ASC->RemoveActiveGameplayEffect(Handle);
	UE_LOG(LogBreachborne, Log, TEXT("EquipmentGEHelper: Removed GE handle"));
	Handle.Invalidate();
}

FGameplayAbilitySpecHandle FBBEquipmentGEHelper::GrantPowerAbility(
	UAbilitySystemComponent* ASC,
	TSubclassOf<UGameplayAbility> AbilityClass,
	UObject* SourceObject,
	const FGameplayTag& InputTag)
{
	if (!ASC || !AbilityClass)
	{
		return FGameplayAbilitySpecHandle();
	}

	FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, SourceObject);
	if (InputTag.IsValid())
	{
		Spec.DynamicAbilityTags.AddTag(InputTag);
	}
	FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);

	UE_LOG(LogBreachborne, Verbose, TEXT("PowerAbility: Granted %s on %s"), *AbilityClass->GetName(), *InputTag.ToString());

	return Handle;
}

void FBBEquipmentGEHelper::RemovePowerAbility(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle& Handle)
{
	if (!ASC || !Handle.IsValid())
	{
		return;
	}

	ASC->ClearAbility(Handle);
	UE_LOG(LogBreachborne, Verbose, TEXT("PowerAbility: Removed power ability"));
	Handle = FGameplayAbilitySpecHandle();
}
