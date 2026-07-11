#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Kingpin_RMB.generated.h"

/**
 * Kingpin RMB — Iron Hook.
 *
 * Fires a hook in the aim direction (instant line trace). On hit, the target is
 * tagged State_Hooked, then the actual pull is handed off to AHunterCharacter::
 * BeginHookPull — see the comment in HunterCharacter.cpp for why the pull state
 * lives there instead of on this ability instance.
 *
 * The ability ends as soon as the trace + handoff completes; the character drives
 * the pull tick, draws the chain indicator, and applies damage on impact.
 *
 * LocalPredicted. 12s cooldown.
 */
UCLASS()
class BREACHBORNE_API UGA_Kingpin_RMB : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Kingpin_RMB();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|RMB")
	float MaxRange = 2500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|RMB")
	float HookDamage = 60.0f;

	/** Speed at which the hooked target is dragged toward Kingpin (cm/s, applied to CMC->Velocity each pull tick) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|RMB")
	float PullSpeed = 2200.0f;

	/** Distance (cm) at which the pull is considered "complete" — damage fires here. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|RMB")
	float ImpactDistance = 250.0f;

	/** Hard cap on pull duration — if target hasn't reached Kingpin in this many seconds, hook releases (no damage). */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|RMB")
	float MaxPullDuration = 3.0f;

	/** Tick interval for the pull loop (seconds). 0.02 = 50Hz, fine-grained enough that drag feels smooth. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|RMB")
	float PullTickInterval = 0.02f;

	/** Duration (seconds) the State_Hooked tag remains on the target */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|RMB")
	float HookTagDuration = 3.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|RMB")
	float CooldownDuration = 12.0f;

private:
	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
