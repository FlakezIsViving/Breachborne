#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Kingpin_LMB.generated.h"

/**
 * Kingpin LMB — Chain Strike.
 *
 * Short-range melee strike. If the primary target is within ChainRadius of another
 * enemy hunter, the hit also bounces to that secondary target for 60% of the base damage.
 * LocalPredicted. No cooldown (auto-attack rate limited by FireInterval).
 */
UCLASS()
class BREACHBORNE_API UGA_Kingpin_LMB : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Kingpin_LMB();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** Range of the primary melee hit */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|LMB")
	float MeleeRange = 250.0f;

	/** Damage per strike */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|LMB")
	float BaseDamage = 40.0f;

	/** Radius within which the strike bounces to a second nearby enemy */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|LMB")
	float ChainRadius = 400.0f;

	/** Fraction of BaseDamage dealt to the chained secondary target */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|LMB")
	float ChainDamageFraction = 0.6f;

	/** Seconds between strikes while input is held */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|LMB")
	float FireInterval = 0.45f;

private:
	void PerformStrike();
	void ApplyDamageToTarget(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float Damage);

	FTimerHandle StrikeTimerHandle;
};
