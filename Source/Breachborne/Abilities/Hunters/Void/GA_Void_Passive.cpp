#include "GA_Void_Passive.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"

UGA_Void_Passive::UGA_Void_Passive()
{
	bActivateOnInputHeld = false;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Void_Passive);
	AssetTags.AddTag(BBGameplayTags::Ability_Category_Passive);
	SetAssetTags(AssetTags);
}

void UGA_Void_Passive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	DamageEventHandle = ASC->AddGameplayEventTagContainerDelegate(
		FGameplayTagContainer(BBGameplayTags::Event_Void_DamageDealt),
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UGA_Void_Passive::OnDamageDealt));
}

void UGA_Void_Passive::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (DamageEventHandle.IsValid())
		{
			ASC->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(BBGameplayTags::Event_Void_DamageDealt), DamageEventHandle);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Void_Passive::OnDamageDealt(FGameplayTag Tag, const FGameplayEventData* Payload)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UWorld* World = GetWorld();
	if (!ASC || !World)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (WindowStartWorldTime <= 0.0f || Now - WindowStartWorldTime > HitWindowSeconds)
	{
		WindowStartWorldTime = Now;
		ConfirmedHits = 0;
	}

	++ConfirmedHits;
	if (ConfirmedHits < HitsToEmpower || ASC->HasMatchingGameplayTag(BBGameplayTags::State_Void_Empowered))
	{
		return;
	}

	ConfirmedHits = 0;
	WindowStartWorldTime = Now;
	ASC->AddLooseGameplayTag(BBGameplayTags::State_Void_Empowered);

	if (AHunterCharacter* Hunter = GetHunterCharacter())
	{
		PlayVisualMontage(BBGameplayTags::Ability_Hunter_Void_Passive, EBBAbilityAnimationPhase::PassivePulse);
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Void_Passive_Empowered, Hunter->GetActorLocation());
	}
}
