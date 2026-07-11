#include "BBAbilityVisualSet.h"

const FBBAbilitySlotVisuals& UBBAbilityVisualSet::FindAbilityVisuals(FGameplayTag AbilityOrInputTag, bool& bFound) const
{
	for (const FBBAbilitySlotVisuals& Visuals : AbilityVisuals)
	{
		if (Visuals.AbilityOrInputTag.IsValid() && Visuals.AbilityOrInputTag.MatchesTagExact(AbilityOrInputTag))
		{
			bFound = true;
			return Visuals;
		}
	}

	static const FBBAbilitySlotVisuals EmptyVisuals;
	bFound = false;
	return EmptyVisuals;
}

UAnimMontage* UBBAbilityVisualSet::FindMontage(FGameplayTag AbilityOrInputTag) const
{
	bool bFound = false;
	const FBBAbilitySlotVisuals& Visuals = FindAbilityVisuals(AbilityOrInputTag, bFound);
	return bFound ? Visuals.Montage : nullptr;
}

UAnimMontage* UBBAbilityVisualSet::FindPhaseMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float& OutPlayRate, bool& bOutLooping) const
{
	return FindMontage(AbilityOrInputTag, Phase, OutPlayRate, bOutLooping);
}

UAnimMontage* UBBAbilityVisualSet::FindMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float& OutPlayRate, bool& bOutLooping) const
{
	OutPlayRate = 1.0f;
	bOutLooping = false;

	bool bFound = false;
	const FBBAbilitySlotVisuals& Visuals = FindAbilityVisuals(AbilityOrInputTag, bFound);
	if (!bFound)
	{
		return nullptr;
	}

	for (const FBBAbilityMontageVisual& PhaseVisual : Visuals.PhaseMontages)
	{
		if (PhaseVisual.Phase == Phase && PhaseVisual.Montage)
		{
			OutPlayRate = PhaseVisual.PlayRate;
			bOutLooping = PhaseVisual.bLooping;
			return PhaseVisual.Montage;
		}
	}

	return Visuals.Montage;
}
