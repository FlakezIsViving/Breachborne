#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Ghost_Shift.generated.h"

/**
 * Ghost's Shift — Combat Slide.
 * Instantly launches the character in the cursor direction. Works on ground and in air.
 * 6s cooldown. LocalPredicted.
 */
UCLASS()
class BREACHBORNE_API UGA_Ghost_Shift : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Ghost_Shift();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	/** Cooldown in seconds */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|Shift")
	float CooldownDuration = 6.0f;

	/** Horizontal velocity applied in the cursor direction */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|Shift")
	float DashSpeed = 2000.0f;

	/** Small upward velocity to clear small obstacles and feel responsive in air */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|Shift")
	float DashLift = 100.0f;

private:
	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
