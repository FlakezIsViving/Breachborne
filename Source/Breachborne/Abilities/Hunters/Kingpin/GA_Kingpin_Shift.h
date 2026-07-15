#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Kingpin_Shift.generated.h"

/**
 * Kingpin Shift — Bull Rush.
 *
 * Kingpin charges forward for ChargeDuration seconds, becoming unstoppable
 * (State_Charging — immune to knockback/stun). Any enemy contacted during the charge
 * takes ChargeDamage and is knocked back.
 *
 * LocalPredicted. 8s cooldown.
 */
UCLASS()
class BREACHBORNE_API UGA_Kingpin_Shift : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Kingpin_Shift();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Shift")
	float ChargeSpeed = 1600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Shift")
	float ChargeDuration = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Shift")
	float ChargeDamage = 55.0f;

	/** Includes both character capsules so blocking contact still registers as a charge hit. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Shift")
	float ChargeHitRadius = 135.0f;

	/** Impulse applied to knocked-back enemies */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Shift")
	float KnockbackForce = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Shift")
	float CooldownDuration = 8.0f;

private:
	void ChargeTick();
	void EndCharge();
	void PlayChargeLoopMontage();

	FTimerHandle ChargeTickHandle;
	FTimerHandle ChargeEndHandle;
	FTimerHandle ChargeLoopMontageHandle;
	FVector ChargeStartLocation = FVector::ZeroVector;
	FVector ChargeDirection = FVector::ForwardVector;
	FVector PreviousChargeLocation = FVector::ZeroVector;
	float ChargeStartTimeSeconds = 0.0f;
	bool bChargePathBlocked = false;

	TSet<TWeakObjectPtr<AActor>> HitActors; // Track per-charge so we don't double-hit

	FGameplayAbilitySpecHandle  CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo   CachedActivationInfo;

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
