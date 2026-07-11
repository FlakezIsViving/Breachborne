#include "GA_Ghost_Shift.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Breachborne/Breachborne.h"

UGA_Ghost_Shift::UGA_Ghost_Shift()
{
	AbilityInputTag = BBGameplayTags::InputTag_Shift;
	bActivateOnInputHeld = false;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Ghost_Shift);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Ghost_Shift);
}

void UGA_Ghost_Shift::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Dash in cursor direction (character forward = cursor aim)
	const FVector DashDir = Hunter->GetActorForwardVector().GetSafeNormal2D();
	FVector LaunchVelocity = DashDir * DashSpeed;
	LaunchVelocity.Z = DashLift;

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Ghost_Shift, EBBAbilityAnimationPhase::Start);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Ghost_Shift_Start, Hunter->GetActorLocation(), DashDir);
	Hunter->LaunchCharacter(LaunchVelocity, true, true);

	UE_LOG(LogBreachborne, Log, TEXT("Ghost Shift: Dash toward cursor on %s"), *Hunter->GetName());

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Ghost_Shift::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Ghost_Shift::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = NewObject<UGameplayEffect>(GetTransientPackage());
	CooldownGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	CooldownGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(CooldownDuration));
	UTargetTagsGameplayEffectComponent& TagComp = CooldownGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::Cooldown_Hunter_Ghost_Shift);
	TagComp.SetAndApplyTargetTagChanges(TagContainer);

	ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CooldownGE, GetAbilityLevel());
}
