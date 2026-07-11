#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Hudson_Passive_Salvager.generated.h"

class UGameplayEffect;

/** Hudson passive - sustain, armor repair, and back-hit vulnerability marker. */
UCLASS()
class BREACHBORNE_API UGA_Hudson_Passive_Salvager : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Hudson_Passive_Salvager();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Passive")
	float RewardMaxHealthFraction = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Passive")
	float RewardArmorRepairFraction = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Passive")
	float ArmorRegenDelay = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Passive")
	float ArmorRegenFractionPerSecond = 0.01f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Passive")
	TSubclassOf<UGameplayEffect> HealEffectClass;

private:
	void OnKillEvent(FGameplayTag Tag, const FGameplayEventData* Payload);
	void OnDamageTaken(FGameplayTag Tag, const FGameplayEventData* Payload);
	void StartArmorRegen();
	void ArmorRegenTick();
	void ApplySelfHeal(float HealAmount);

	FDelegateHandle KillEventHandle;
	FDelegateHandle DamageTakenEventHandle;
	FTimerHandle ArmorRegenDelayHandle;
	FTimerHandle ArmorRegenTickHandle;
};
