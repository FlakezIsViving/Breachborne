#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Kingpin_Q.generated.h"

/**
 * Kingpin Q — Headcracker.
 *
 * Kingpin rears back and delivers a devastating overhead slam in a short cone.
 * Enemies hit are stunned (State_Stunned) for StunDuration seconds.
 * Deals moderate damage.
 *
 * LocalPredicted. 10s cooldown.
 */
UCLASS()
class BREACHBORNE_API UGA_Kingpin_Q : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Kingpin_Q();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Q")
	float SlamRange = 300.0f;

	/** Half-angle of the slam cone in degrees */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Q")
	float SlamHalfAngle = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Q")
	float BaseDamage = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Q")
	float StunDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Q")
	float CooldownDuration = 10.0f;

private:
	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
