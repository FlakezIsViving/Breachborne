#include "BBStunTagEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"

UBBStunTagEffect::UBBStunTagEffect()
{
	DurationPolicy    = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.5f));

	UTargetTagsGameplayEffectComponent* TagComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TagComp);

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::State_Stunned);
	TagComp->SetAndApplyTargetTagChanges(TagContainer);
}
