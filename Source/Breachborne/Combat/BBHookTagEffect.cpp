#include "BBHookTagEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"

UBBHookTagEffect::UBBHookTagEffect()
{
	DurationPolicy    = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.5f));

	// State_Hooked tag. CreateDefaultSubobject (NOT FindOrAddComponent) — see BBDazeEffect for
	// why: FindOrAddComponent calls NewObject<T>(this) with empty name from inside a UObject
	// constructor, which UE 5.7 fatal-asserts on at module load.
	UTargetTagsGameplayEffectComponent* TagComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TagComp);

	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::State_Hooked);
	TagComp->SetAndApplyTargetTagChanges(TagContainer);
}
