#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Kingpin_GroundDash.generated.h"

/**
 * Kingpin Ground Dash — short horizontal dash, usable anywhere (ground OR air).
 *
 * Companion to UGA_Kingpin_AerialDash. Both share InputTag.Shift; the AerialDash is granted
 * first and tries first, so in the air a press goes to AerialDash if available, otherwise
 * falls through to this. On the ground, AerialDash's airborne check rejects it, this fires.
 *
 * Distance is the "base" — AerialDash speed is 1.5× this. Dash direction follows WASD
 * (CMC->GetCurrentAcceleration), with cursor fallback when no movement input.
 *
 * LocalPredicted, independent cooldown from AerialDash.
 */
UCLASS()
class BREACHBORNE_API UGA_Kingpin_GroundDash : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Kingpin_GroundDash();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Dash")
	float DashSpeed = 2000.0f;

	/** Vertical impulse applied alongside horizontal — small lift makes the dash feel snappy off the ground. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Dash")
	float DashLift = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Dash")
	float CooldownDuration = 6.0f;

private:
	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
