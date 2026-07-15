#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Ghost_Passive.generated.h"

class UAbilityTask_WaitGameplayEvent;
class ABBPrimitiveBurstActor;

/**
 * Ghost's Passive — Kill Resets Shift Cooldown.
 * ServerOnly: activated on grant, stays active permanently.
 * Listens for Event.Kill gameplay events on the owning ASC.
 * On each kill: removes the active Shift cooldown GE so Shift is immediately usable.
 */
UCLASS()
class BREACHBORNE_API UGA_Ghost_Passive : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Ghost_Passive();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** Called when Event.Kill fires on our ASC */
	UFUNCTION()
	void OnKillEvent(FGameplayEventData Payload);

	/** Start (or restart) listening for the next kill event */
	void WaitForNextKill();

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|Passive|Visual")
	TSubclassOf<ABBPrimitiveBurstActor> PulseVisualClass;
};
