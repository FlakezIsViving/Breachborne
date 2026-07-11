#include "GA_Hudson_Passive_Salvager.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBHealEffect.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Items/BBInventoryManager.h"

UGA_Hudson_Passive_Salvager::UGA_Hudson_Passive_Salvager()
{
	bActivateOnInputHeld = false;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Hudson_Passive);
	AssetTags.AddTag(BBGameplayTags::Ability_Category_Passive);
	SetAssetTags(AssetTags);

	HealEffectClass = UBBHealEffect::StaticClass();
}

void UGA_Hudson_Passive_Salvager::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	ASC->AddLooseGameplayTag(BBGameplayTags::Ability_Hunter_Hudson_Passive);
	KillEventHandle = ASC->AddGameplayEventTagContainerDelegate(
		FGameplayTagContainer(BBGameplayTags::Event_Kill),
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UGA_Hudson_Passive_Salvager::OnKillEvent));
	DamageTakenEventHandle = ASC->AddGameplayEventTagContainerDelegate(
		FGameplayTagContainer(BBGameplayTags::Event_DamageTaken),
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UGA_Hudson_Passive_Salvager::OnDamageTaken));

	StartArmorRegen();
}

void UGA_Hudson_Passive_Salvager::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ArmorRegenDelayHandle);
		World->GetTimerManager().ClearTimer(ArmorRegenTickHandle);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(BBGameplayTags::Ability_Hunter_Hudson_Passive);
		if (KillEventHandle.IsValid())
		{
			ASC->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(BBGameplayTags::Event_Kill), KillEventHandle);
		}
		if (DamageTakenEventHandle.IsValid())
		{
			ASC->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(BBGameplayTags::Event_DamageTaken), DamageTakenEventHandle);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Hudson_Passive_Salvager::OnKillEvent(FGameplayTag Tag, const FGameplayEventData* Payload)
{
	ABreachbornePlayerState* PS = GetBBPlayerState();
	if (!PS)
	{
		return;
	}

	if (const UBBHealthSet* HealthSet = PS->GetHealthSet())
	{
		ApplySelfHeal(HealthSet->GetMaxHealth() * RewardMaxHealthFraction);
	}

	FBBInventoryManager::RepairArmorDurability(PS, RewardArmorRepairFraction);

	if (AHunterCharacter* Hunter = GetHunterCharacter())
	{
		PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_Passive, EBBAbilityAnimationPhase::PassivePulse);
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_Passive_Pulse, Hunter->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f));
		Hunter->Multicast_DrawDebugSphere(Hunter->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f), 70.0f, FColor::Green, 0.5f);
		Hunter->Multicast_DrawDebugCircle(Hunter->GetActorLocation(), 155.0f, FColor::Blue, 0.5f, 5.0f);
	}
}

void UGA_Hudson_Passive_Salvager::OnDamageTaken(FGameplayTag Tag, const FGameplayEventData* Payload)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ArmorRegenTickHandle);
		World->GetTimerManager().ClearTimer(ArmorRegenDelayHandle);
		World->GetTimerManager().SetTimer(ArmorRegenDelayHandle, this, &UGA_Hudson_Passive_Salvager::StartArmorRegen, ArmorRegenDelay, false);
	}
}

void UGA_Hudson_Passive_Salvager::StartArmorRegen()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ArmorRegenTickHandle, this, &UGA_Hudson_Passive_Salvager::ArmorRegenTick, 1.0f, true, 0.0f);
	}
}

void UGA_Hudson_Passive_Salvager::ArmorRegenTick()
{
	ABreachbornePlayerState* PS = GetBBPlayerState();
	if (!PS)
	{
		return;
	}

	if (FBBInventoryManager::RepairArmorDurability(PS, ArmorRegenFractionPerSecond))
	{
		if (AHunterCharacter* Hunter = GetHunterCharacter())
		{
			PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_Passive, EBBAbilityAnimationPhase::PassivePulse);
			ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_Passive_Pulse, Hunter->GetActorLocation());
			Hunter->Multicast_DrawDebugCircle(Hunter->GetActorLocation(), 135.0f, FColor::Blue, 0.35f, 3.0f);
		}
	}
}

void UGA_Hudson_Passive_Salvager::ApplySelfHeal(float HealAmount)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !HealEffectClass || HealAmount <= 0.0f)
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(HealEffectClass, 1.0f, ASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_HealAmount, HealAmount);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	}
}
