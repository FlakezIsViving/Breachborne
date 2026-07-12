#include "BBReverberationMarkEffect.h"

#include "Breachborne/Abilities/BBGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UBBReverberationMarkEffect::UBBReverberationMarkEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(4.0f));

	UTargetTagsGameplayEffectComponent* TagComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TagComp);

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::State_Crysta_Reverberation);
	TagComp->SetAndApplyTargetTagChanges(TagContainer);
}
