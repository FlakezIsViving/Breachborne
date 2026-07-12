#include "BBVoidGravitySlowEffect.h"

#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBMovementSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UBBVoidGravitySlowEffect::UBBVoidGravitySlowEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.5f));

	FGameplayModifierInfo SpeedMod;
	SpeedMod.Attribute = UBBMovementSet::GetMoveSpeedAttribute();
	SpeedMod.ModifierOp = EGameplayModOp::Multiplicitive;
	SpeedMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.94f));
	Modifiers.Add(SpeedMod);

	UTargetTagsGameplayEffectComponent* TagComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TagComp);

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::State_Slowed);
	TagComp->SetAndApplyTargetTagChanges(TagContainer);
}
