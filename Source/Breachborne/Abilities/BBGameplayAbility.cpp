#include "BBGameplayAbility.h"
#include "BBAbilitySystemComponent.h"
#include "BBAbilityVisualSet.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBCooldownEffect.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "GameplayCueInterface.h"

UBBGameplayAbility::UBBGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Block activation when Dead, Stunned, Spiked, Gliding, or Mantling
	ActivationBlockedTags.AddTag(BBGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(BBGameplayTags::State_Stunned);
	ActivationBlockedTags.AddTag(BBGameplayTags::State_Spiked);
	ActivationBlockedTags.AddTag(BBGameplayTags::State_Gliding);
	ActivationBlockedTags.AddTag(BBGameplayTags::State_Mantling);
}

void UBBGameplayAbility::OnInputReleased()
{
	// Base implementation: if ability is active and meant for held input, end it
	if (bActivateOnInputHeld && IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UBBGameplayAbility::ConfigureRangeIndicator(EBBRangeIndicatorMode Mode, float CastRange, float EffectRadius)
{
	RangeIndicatorInfo.Mode = Mode;
	RangeIndicatorInfo.CastRange = FMath::Max(0.0f, CastRange);
	RangeIndicatorInfo.EffectRadius = FMath::Max(0.0f, EffectRadius);
}

bool UBBGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (AbilityInputTag.MatchesTagExact(BBGameplayTags::InputTag_Shift)
		&& ActorInfo
		&& ActorInfo->AbilitySystemComponent.IsValid()
		&& ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(BBGameplayTags::State_Grounded))
	{
		return false;
	}

	return true;
}

AHunterCharacter* UBBGameplayAbility::GetHunterCharacter() const
{
	return CurrentActorInfo ? Cast<AHunterCharacter>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
}

ABreachbornePlayerState* UBBGameplayAbility::GetBBPlayerState() const
{
	return CurrentActorInfo ? Cast<ABreachbornePlayerState>(CurrentActorInfo->OwnerActor.Get()) : nullptr;
}

UBBAbilitySystemComponent* UBBGameplayAbility::GetBBAbilitySystemComponent() const
{
	return CurrentActorInfo ? Cast<UBBAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr;
}

FVector UBBGameplayAbility::GetAimLocation() const
{
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		return Hunter->GetAimLocation();
	}
	return FVector::ZeroVector;
}

FVector UBBGameplayAbility::GetAimDirection() const
{
	const AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		return FVector::ForwardVector;
	}

	const FVector AimLoc = Hunter->GetAimLocation();
	const FVector CharLoc = Hunter->GetActorLocation();
	FVector Direction = (AimLoc - CharLoc).GetSafeNormal2D();

	if (Direction.IsNearlyZero())
	{
		Direction = Hunter->GetActorForwardVector().GetSafeNormal2D();
	}

	return Direction;
}

void UBBGameplayAbility::ApplyBBCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, float Duration) const
{
	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	ApplyBBCooldownWithTags(Handle, ActorInfo, ActivationInfo, Duration,
		CooldownTags ? *CooldownTags : FGameplayTagContainer());
}

void UBBGameplayAbility::ApplyBBCooldownWithTags(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	float Duration, const FGameplayTagContainer& GrantedTags) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || Duration <= 0.0f)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = BuildBBCooldownSpec(ActorInfo->AbilitySystemComponent.Get(), Duration,
		GrantedTags, GetAbilityLevel(Handle, ActorInfo));
	if (SpecHandle.IsValid())
	{
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
}

FGameplayEffectSpecHandle UBBGameplayAbility::BuildBBCooldownSpec(UAbilitySystemComponent* ASC, float Duration,
	const FGameplayTagContainer& GrantedTags, float AbilityLevel)
{
	if (!ASC || Duration <= 0.0f)
	{
		return FGameplayEffectSpecHandle();
	}

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(UBBCooldownEffect::StaticClass(), AbilityLevel,
		ASC->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetDuration(Duration, true);
		SpecHandle.Data->DynamicGrantedTags.AppendTags(GrantedTags);
		SpecHandle.Data->DynamicGrantedTags.AddTag(BBGameplayTags::Effect_Cooldown);
	}
	return SpecHandle;
}

void UBBGameplayAbility::ExecuteVisualCue(FGameplayTag CueTag, const FVector& Location, const FVector& Normal) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !CueTag.IsValid())
	{
		return;
	}

	FGameplayCueParameters CueParams;
	AActor* Avatar = GetAvatarActorFromActorInfo();
	CueParams.Instigator = Avatar;
	CueParams.EffectCauser = Avatar;
	CueParams.SourceObject = Avatar;
	CueParams.AbilityLevel = GetAbilityLevel();
	CueParams.Location = Location.IsNearlyZero() && Avatar ? Avatar->GetActorLocation() : Location;
	CueParams.Normal = Normal;

	ASC->ExecuteGameplayCue(CueTag, CueParams);
}

void UBBGameplayAbility::ExecuteBeamVisualCue(FGameplayTag CueTag, const FVector& StartLocation,
	const FVector& EndLocation) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !CueTag.IsValid())
	{
		return;
	}

	const FVector BeamDelta = EndLocation - StartLocation;
	if (BeamDelta.IsNearlyZero())
	{
		return;
	}

	FGameplayCueParameters CueParams;
	AActor* Avatar = GetAvatarActorFromActorInfo();
	CueParams.Instigator = Avatar;
	CueParams.EffectCauser = Avatar;
	CueParams.SourceObject = Avatar;
	CueParams.AbilityLevel = GetAbilityLevel();
	CueParams.Location = StartLocation;
	CueParams.Normal = BeamDelta.GetSafeNormal();
	CueParams.RawMagnitude = BeamDelta.Size();

	ASC->ExecuteGameplayCue(CueTag, CueParams);
}

void UBBGameplayAbility::AddVisualCue(FGameplayTag CueTag, const FVector& Location, const FVector& Normal) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !CueTag.IsValid())
	{
		return;
	}

	FGameplayCueParameters CueParams;
	AActor* Avatar = GetAvatarActorFromActorInfo();
	CueParams.Instigator = Avatar;
	CueParams.EffectCauser = Avatar;
	CueParams.SourceObject = Avatar;
	CueParams.AbilityLevel = GetAbilityLevel();
	CueParams.Location = Location.IsNearlyZero() && Avatar ? Avatar->GetActorLocation() : Location;
	CueParams.Normal = Normal;

	ASC->AddGameplayCue(CueTag, CueParams);
}

void UBBGameplayAbility::RemoveVisualCue(FGameplayTag CueTag) const
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo(); ASC && CueTag.IsValid())
	{
		ASC->RemoveGameplayCue(CueTag);
	}
}

float UBBGameplayAbility::PlayVisualMontage(FGameplayTag AbilityOrInputTag, float PlayRate) const
{
	return PlayVisualMontage(AbilityOrInputTag, EBBAbilityAnimationPhase::Start, PlayRate);
}

float UBBGameplayAbility::PlayVisualMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float PlayRateOverride) const
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		return 0.0f;
	}

	float Duration = Hunter->PlayAbilityMontage(AbilityOrInputTag, Phase, PlayRateOverride);
	if (Duration <= 0.0f && AbilityInputTag.IsValid() && !AbilityInputTag.MatchesTagExact(AbilityOrInputTag))
	{
		Duration = Hunter->PlayAbilityMontage(AbilityInputTag, Phase, PlayRateOverride);
	}

	return Duration;
}

void UBBGameplayAbility::StopVisualMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float BlendOutTime) const
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		return;
	}

	Hunter->StopAbilityMontage(AbilityOrInputTag, Phase, BlendOutTime);
	if (AbilityInputTag.IsValid() && !AbilityInputTag.MatchesTagExact(AbilityOrInputTag))
	{
		Hunter->StopAbilityMontage(AbilityInputTag, Phase, BlendOutTime);
	}
}
