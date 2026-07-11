#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Eluna_R.generated.h"

class ABBWispPawn;
class ABBElunaReviveBeam;
class UBBHealEffect;

/**
 * Eluna R — Divine Intervention.
 *
 * Channeled 3.0s ultimate. Eluna links to the nearest ally (wisp or alive).
 * During the channel: heals the target for 50 HP/s.
 * After 3.0s: if the target was a wisp, they are revived.
 * Channel breaks if the target moves out of range.
 * LocalPredicted. 60s cooldown.
 */
UCLASS()
class BREACHBORNE_API UGA_Eluna_R : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Eluna_R();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	/** Total channel duration in seconds */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|R")
	float ChannelDuration = 3.0f;

	/** Heal per second applied during the channel */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|R")
	float HealPerSecond = 50.0f;

	/** Max range the target can move before channel breaks */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|R")
	float MaxChannelRange = 1500.0f;

	/** How far to search for a target */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|R")
	float TargetSearchRange = 1500.0f;

	/** Cooldown duration */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|R")
	float CooldownDuration = 60.0f;

private:
	/** Find the best target: nearest ally wisp, then nearest alive ally below max HP */
	AActor* FindBestTarget() const;

	/** Start the channel on the chosen target */
	void StartChannel(AActor* Target);

	/** Called every tick during the channel */
	void OnChannelTick();

	/** Called when the channel completes successfully */
	void OnChannelComplete();

	/** Break the channel early (out of range, target died, etc.) */
	void BreakChannel();

	/** Clean up channel state and end ability */
	void EndChannel(bool bWasCancelled);

	/** Starts the looping channel montage after the start phase has had time to play. */
	void PlayChannelLoopMontage();

	/** Apply a single heal tick to the current target */
	void ApplyHealTick(float DeltaTime);

	/** Spawn/update the beam visual */
	void SpawnBeam();
	void UpdateBeam();
	void DestroyBeam();

	/** Distance from hunter to current target */
	float GetDistanceToTarget() const;

	/** Target we're currently channeling on */
	TWeakObjectPtr<AActor> ChannelTarget;

	/** If the target is a wisp, cached pointer */
	TWeakObjectPtr<ABBWispPawn> TargetWisp;

	/** Active beam visual actor */
	TWeakObjectPtr<ABBElunaReviveBeam> ActiveBeam;

	/** Timer handles */
	FTimerHandle ChannelTickHandle;
	FTimerHandle ChannelCompleteHandle;
	FTimerHandle ChannelLoopMontageHandle;

	/** Channel state */
	bool bIsChanneling = false;
	float ChannelElapsed = 0.0f;
	static constexpr float ChannelTickInterval = 0.2f;

	/** Cooldown tag container */
	FGameplayTagContainer CooldownTagContainer;

	/** Cached heal effect class */
	TSubclassOf<UBBHealEffect> HealEffectClass;
};
