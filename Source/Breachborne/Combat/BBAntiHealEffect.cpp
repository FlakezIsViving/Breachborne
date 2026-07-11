#include "BBAntiHealEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBHealthSet.h"

UBBAntiHealEffect::UBBAntiHealEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(4.0f));

	UAssetTagsGameplayEffectComponent* AssetTagsComp = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTagsComponent"));
	GEComponents.Add(AssetTagsComp);

	FInheritedTagContainer AssetTags;
	AssetTags.Added.AddTag(BBGameplayTags::Effect_AntiHeal);
	AssetTagsComp->SetAndApplyAssetTagChanges(AssetTags);

	FGameplayModifierInfo& HealingMod = Modifiers.AddDefaulted_GetRef();
	HealingMod.Attribute = UBBHealthSet::GetHealingReceivedMultiplierAttribute();
	HealingMod.ModifierOp = EGameplayModOp::Additive;
	HealingMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-0.5f));

	UTargetTagsGameplayEffectComponent* TagComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TagComp);

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::State_AntiHeal);
	TagComp->SetAndApplyTargetTagChanges(TagContainer);
}
