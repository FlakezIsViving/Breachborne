#include "BBSlowEffect.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBMovementSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UBBSlowEffect::UBBSlowEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.0f));

	FGameplayModifierInfo SpeedMod;
	SpeedMod.Attribute = UBBMovementSet::GetMoveSpeedAttribute();
	SpeedMod.ModifierOp = EGameplayModOp::Multiplicitive;
	SpeedMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.55f));
	Modifiers.Add(SpeedMod);

	UTargetTagsGameplayEffectComponent* TagComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TagComp);

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::State_Slowed);
	TagComp->SetAndApplyTargetTagChanges(TagContainer);
}
