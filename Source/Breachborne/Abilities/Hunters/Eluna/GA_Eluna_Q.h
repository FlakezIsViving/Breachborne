#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Eluna_Q.generated.h"

class ABBMoonlightZone;

/**
 * Eluna Q — Moonlight Blessing.
 *
 * Press Q: attaches a healing zone to Eluna.
 * Press Q again while active: tosses the zone to aim location (boosts healing 50%).
 * The tossed zone attaches to the first eligible ally or allied wisp in its path.
 * If an attached hunter dies, the zone drops at that location for its remaining duration.
 * Zone lasts 2.5s then expires with a burst heal.
 * Heals allies in radius. Less effective on self. 500% effective on Wisps.
 * LocalPredicted. 12s cooldown.
 */
UCLASS()
class BREACHBORNE_API UGA_Eluna_Q : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Eluna_Q();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Q")
	float HealPerTick = 12.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Q")
	float HealTickInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Q")
	float HealRadius = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Q")
	float ZoneDuration = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Q")
	float SelfHealFraction = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Q")
	float WispHealMultiplier = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Q")
	float BurstHeal = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Q")
	float ThrowRange = 2000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Q")
	float CooldownDuration = 12.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Q")
	TSubclassOf<ABBMoonlightZone> ZoneClass;

private:
	void SpawnZone();
	void TossZone();
	void OnZoneExpired();

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;

	FTimerHandle ZoneExpiryTimerHandle;
	TWeakObjectPtr<ABBMoonlightZone> ActiveZone;
};
