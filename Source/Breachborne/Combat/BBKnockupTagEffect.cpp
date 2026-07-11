#include "BBKnockupTagEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"

UBBKnockupTagEffect::UBBKnockupTagEffect()
{
	DurationPolicy    = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.8f));

	UTargetTagsGameplayEffectComponent* TagComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TagComp);

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::Effect_Knockup);
	TagComp->SetAndApplyTargetTagChanges(TagContainer);
}
