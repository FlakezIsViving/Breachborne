#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Kingpin_AerialDash.generated.h"

/**
 * Kingpin Aerial Dash — long horizontal dash, AIRBORNE ONLY. 1.5× the distance of GroundDash.
 *
 * Companion to UGA_Kingpin_GroundDash. Both share InputTag.Shift; this ability is granted
 * FIRST so TryActivateAbilityByInputTag tries it ahead of GroundDash. CanActivateAbility
 * rejects when the character is on the ground, which lets the input dispatcher fall
 * through to GroundDash (the "single Shift key, two charges, second is conditional"
 * pattern).
 *
 * In the air with both dashes off-CD: press 1 = Aerial, press 2 = Ground (Aerial now on
 * cooldown so it falls through). On the ground: press 1 = Ground only.
 *
 * LocalPredicted, independent cooldown from GroundDash.
 */
UCLASS()
class BREACHBORNE_API UGA_Kingpin_AerialDash : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Kingpin_AerialDash();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Dash")
	float DashSpeed = 3000.0f;

	/** No vertical lift — already airborne, want pure horizontal travel. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Dash")
	float DashLift = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|Dash")
	float CooldownDuration = 6.0f;

private:
	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
