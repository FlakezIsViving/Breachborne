#include "BBAbilityVisualSet.h"

#include "Animation/AnimMontage.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/DataValidation.h"

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

#if WITH_EDITOR
EDataValidationResult UBBAbilityVisualSet::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto AddError = [&Context, &Result](const FString& Message)
	{
		Context.AddError(FText::FromString(Message));
		Result = EDataValidationResult::Invalid;
	};

	const USkeleton* TargetSkeleton = SkeletalMesh ? SkeletalMesh->GetSkeleton() : nullptr;
	TSet<FGameplayTag> SeenAbilityTags;

	auto ValidateMontage = [this, TargetSkeleton, &AddError](
		const UAnimMontage* Montage,
		const FString& EntryName,
		bool bExpectedLoop)
	{
		if (!Montage)
		{
			AddError(FString::Printf(TEXT("%s has no montage."), *EntryName));
			return;
		}

		const USkeleton* MontageSkeleton = Montage->GetSkeleton();
		if (!TargetSkeleton)
		{
			AddError(FString::Printf(TEXT("%s cannot be validated because the visual set has no skeletal mesh/skeleton."), *EntryName));
		}
		else if (!MontageSkeleton || MontageSkeleton != TargetSkeleton)
		{
			AddError(FString::Printf(TEXT("%s uses skeleton %s instead of %s."),
				*EntryName, *GetNameSafe(MontageSkeleton), *GetNameSafe(TargetSkeleton)));
		}

		if (!RequiredMontageSlotName.IsNone() && !Montage->IsValidSlot(RequiredMontageSlotName))
		{
			AddError(FString::Printf(TEXT("%s does not contain required slot %s."),
				*EntryName, *RequiredMontageSlotName.ToString()));
		}
		if (Montage->HasRootMotion())
		{
			AddError(FString::Printf(TEXT("%s contains root motion; Breachborne ability movement must remain code-owned."),
				*EntryName));
		}

		if (bExpectedLoop)
		{
			const bool bHasLoopingSection = Montage->CompositeSections.ContainsByPredicate(
				[](const FCompositeSection& Section)
				{
					return !Section.SectionName.IsNone() && Section.NextSectionName == Section.SectionName;
				});
			if (!bHasLoopingSection)
			{
				AddError(FString::Printf(TEXT("%s is marked looping but has no self-looping montage section."),
					*EntryName));
			}
		}
	};

	for (int32 AbilityIndex = 0; AbilityIndex < AbilityVisuals.Num(); ++AbilityIndex)
	{
		const FBBAbilitySlotVisuals& Visuals = AbilityVisuals[AbilityIndex];
		const FString AbilityLabel = FString::Printf(TEXT("AbilityVisuals[%d]"), AbilityIndex);
		if (!Visuals.AbilityOrInputTag.IsValid())
		{
			AddError(FString::Printf(TEXT("%s has no ability/input tag."), *AbilityLabel));
		}
		else if (SeenAbilityTags.Contains(Visuals.AbilityOrInputTag))
		{
			AddError(FString::Printf(TEXT("%s duplicates tag %s."),
				*AbilityLabel, *Visuals.AbilityOrInputTag.ToString()));
		}
		SeenAbilityTags.Add(Visuals.AbilityOrInputTag);

		if (Visuals.Montage)
		{
			ValidateMontage(Visuals.Montage,
				FString::Printf(TEXT("%s fallback montage"), *AbilityLabel), false);
		}

		TSet<EBBAbilityAnimationPhase> SeenPhases;
		for (int32 PhaseIndex = 0; PhaseIndex < Visuals.PhaseMontages.Num(); ++PhaseIndex)
		{
			const FBBAbilityMontageVisual& PhaseVisual = Visuals.PhaseMontages[PhaseIndex];
			const FString PhaseLabel = FString::Printf(TEXT("%s.PhaseMontages[%d]"), *AbilityLabel, PhaseIndex);
			if (SeenPhases.Contains(PhaseVisual.Phase))
			{
				AddError(FString::Printf(TEXT("%s duplicates animation phase %d."),
					*PhaseLabel, static_cast<int32>(PhaseVisual.Phase)));
			}
			SeenPhases.Add(PhaseVisual.Phase);

			if (PhaseVisual.PlayRate <= 0.0f)
			{
				AddError(FString::Printf(TEXT("%s has non-positive play rate %.3f."),
					*PhaseLabel, PhaseVisual.PlayRate));
			}
			const bool bIsLoopPhase = PhaseVisual.Phase == EBBAbilityAnimationPhase::Loop;
			if (PhaseVisual.bLooping != bIsLoopPhase)
			{
				AddError(FString::Printf(TEXT("%s looping flag must be true only for the Loop phase."), *PhaseLabel));
			}
			ValidateMontage(PhaseVisual.Montage, PhaseLabel, bIsLoopPhase);
		}
	}

	return Result == EDataValidationResult::Invalid ? Result : EDataValidationResult::Valid;
}
#endif
