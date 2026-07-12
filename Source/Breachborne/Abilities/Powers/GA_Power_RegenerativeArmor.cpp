#include "GA_Power_RegenerativeArmor.h"

#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Items/BBInventoryManager.h"
#include "Breachborne/Breachborne.h"

UGA_Power_RegenerativeArmor::UGA_Power_RegenerativeArmor()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::SelfCentered, 0.0f, 120.0f);
}

void UGA_Power_RegenerativeArmor::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->OwnerActor.IsValid() || !ActorInfo->OwnerActor->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	World->GetTimerManager().SetTimer(RepairTimerHandle, this, &UGA_Power_RegenerativeArmor::RepairTick, TickInterval, true);
	RepairTick();
	UE_LOG(LogBreachborne, Log, TEXT("PowerAbility: %s passive armor regeneration active"), *GetPowerID().ToString());
}

void UGA_Power_RegenerativeArmor::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RepairTimerHandle);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Power_RegenerativeArmor::RepairTick()
{
	if (ABreachbornePlayerState* PS = GetBBPlayerState())
	{
		FBBInventoryManager::RepairArmorDurability(PS, RepairFractionPerTick);
	}
}
