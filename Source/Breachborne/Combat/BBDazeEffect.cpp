#include "BBDazeEffect.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBMovementSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayEffectComponents/AssetTagsGameplayEffectComponent.h"

UBBDazeEffect::UBBDazeEffect()
{
	// 2-second daze; callers may override duration by applying a spec with a custom duration
	// if they need variable timing. For Ghost RMB, 2s is the tuning value.
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(2.0f));

	// --- MoveSpeed debuff: multiply by 0.6 (−40% speed for the daze duration) ---
	FGameplayModifierInfo SpeedMod;
	SpeedMod.Attribute = UBBMovementSet::GetMoveSpeedAttribute();
	SpeedMod.ModifierOp = EGameplayModOp::Multiplicitive; // UE GAS naming (intentional typo in engine)
	SpeedMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.6f));
	Modifiers.Add(SpeedMod);

	// --- Grant State.Dazed tag for the effect's duration ---
	// IMPORTANT: Inside a UObject constructor we MUST use CreateDefaultSubobject, not
	// FindOrAddComponent. FindOrAddComponent internally calls NewObject<T>(this) with an
	// empty name, which UE 5.7 asserts on (UObjectGlobals.cpp ~line 4998:
	// "NewObject with empty name can't be used to create default subobjects").
	// The assert fires during CDO construction at module load and crashes the editor.
	UTargetTagsGameplayEffectComponent* TargetTagsComp = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));
	GEComponents.Add(TargetTagsComp);
	FInheritedTagContainer TargetTags;
	TargetTags.Added.AddTag(BBGameplayTags::State_Dazed);
	TargetTagsComp->SetAndApplyTargetTagChanges(TargetTags);

	// --- Self-identify so other systems can query/remove by tag ---
	// UE 5.7 deprecated InheritableGameplayEffectTags in favor of UAssetTagsGameplayEffectComponent.
	// Same Asset-tag semantics — queries via GetAssetTags() still see Effect.Daze on the GE def.
	UAssetTagsGameplayEffectComponent* AssetTagsComp = CreateDefaultSubobject<UAssetTagsGameplayEffectComponent>(TEXT("AssetTagsComponent"));
	GEComponents.Add(AssetTagsComp);
	FInheritedTagContainer AssetTags;
	AssetTags.Added.AddTag(BBGameplayTags::Effect_Daze);
	AssetTagsComp->SetAndApplyAssetTagChanges(AssetTags);
}
