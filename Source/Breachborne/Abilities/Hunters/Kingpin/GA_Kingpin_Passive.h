#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Kingpin_Passive.generated.h"

/**
 * Kingpin Passive — Iron Will.
 *
 * Each time Kingpin applies a CC effect to an enemy (State_Stunned, State_Hooked,
 * State_Charging knockback), Kingpin's active cooldowns are all reduced by
 * CooldownReduction seconds. This rewards aggressive, multi-CC play.
 *
 * Listens for BBGameplayTags::Event_CCApplied, dispatched by any Kingpin ability
 * that successfully lands a CC.
 *
 * ServerOnly. Passive — always active.
 */
UCLASS()
class BREACHBORNE_API UGA_Kingpin_Passive : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Kingpin_Passive();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** Seconds removed from each active cooldown when CC is applied */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Passive")
	float CooldownReduction = 1.5f;

private:
	// FGameplayEventTagMulticastDelegate signature: Tag + Payload pointer
	void OnCCApplied(FGameplayTag Tag, const FGameplayEventData* Payload);

	FDelegateHandle CCEventHandle;
};
