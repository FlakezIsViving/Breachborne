#include "BBPowerGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Items/BBPowerDefinition.h"

UBBPowerGameplayAbility::UBBPowerGameplayAbility()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

const UBBPowerDefinition* UBBPowerGameplayAbility::GetPowerDefinition() const
{
	if (const UBBAbilitySystemComponent* ASC = GetBBAbilitySystemComponent())
	{
		if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle))
		{
			return Cast<UBBPowerDefinition>(Spec->SourceObject.Get());
		}
	}
	return nullptr;
}

FName UBBPowerGameplayAbility::GetPowerID() const
{
	if (const UBBPowerDefinition* PowerDef = GetPowerDefinition())
	{
		return PowerDef->PowerID;
	}
	return NAME_None;
}

EPowerSlotIndex UBBPowerGameplayAbility::GetPowerSlot() const
{
	const FGameplayTag InputTag = GetPowerInputTag();
	return InputTag.MatchesTagExact(BBGameplayTags::InputTag_Power2)
		? EPowerSlotIndex::Power2
		: EPowerSlotIndex::Power1;
}

FGameplayTag UBBPowerGameplayAbility::GetPowerInputTag() const
{
	if (const UBBAbilitySystemComponent* ASC = GetBBAbilitySystemComponent())
	{
		if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle))
		{
			if (Spec->DynamicAbilityTags.HasTagExact(BBGameplayTags::InputTag_Power2))
			{
				return BBGameplayTags::InputTag_Power2;
			}
			if (Spec->DynamicAbilityTags.HasTagExact(BBGameplayTags::InputTag_Power1))
			{
				return BBGameplayTags::InputTag_Power1;
			}
		}
	}

	return AbilityInputTag.IsValid() ? AbilityInputTag : BBGameplayTags::InputTag_Power1;
}

float UBBPowerGameplayAbility::GetPowerCooldownSeconds() const
{
	if (const UBBPowerDefinition* PowerDef = GetPowerDefinition())
	{
		return PowerDef->CooldownSeconds;
	}
	return 0.0f;
}

void UBBPowerGameplayAbility::ApplyPowerCooldown(float OverrideDuration) const
{
	const float Duration = OverrideDuration >= 0.0f ? OverrideDuration : GetPowerCooldownSeconds();
	if (Duration <= 0.0f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	UGameplayEffect* CooldownGE = NewObject<UGameplayEffect>(ASC->GetOwner(), TEXT("GE_PowerCooldown"));
	CooldownGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	CooldownGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(GetPowerInputTag().MatchesTagExact(BBGameplayTags::InputTag_Power2)
		? BBGameplayTags::Cooldown_Power2
		: BBGameplayTags::Cooldown_Power1);
	CooldownGE->InheritableOwnedTagsContainer = TagContainer;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetPowerDefinition());
	FGameplayEffectSpec Spec(CooldownGE, Context, 1.0f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
}

bool UBBPowerGameplayAbility::IsPowerOnCooldown() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return false;
	}

	const FGameplayTag CooldownTag = GetPowerInputTag().MatchesTagExact(BBGameplayTags::InputTag_Power2)
		? BBGameplayTags::Cooldown_Power2
		: BBGameplayTags::Cooldown_Power1;
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(CooldownTag));
	return ASC->GetActiveEffects(Query).Num() > 0;
}
