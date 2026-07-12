#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Crysta_Q.generated.h"

class ABBCrystaBurstZone;
class UGameplayEffect;

UCLASS()
class BREACHBORNE_API UGA_Crysta_Q : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Crysta_Q();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Q")
	float CooldownDuration = 11.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Q")
	float Range = 1700.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Q")
	float Radius = 420.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Q")
	float Duration = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Q")
	float BurstDamage = 95.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Q")
	TSubclassOf<ABBCrystaBurstZone> ZoneClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Q")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Q")
	TSubclassOf<UGameplayEffect> MarkEffectClass;

private:
	void SpawnZone();
	void OnZoneExpired();

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;

	UPROPERTY()
	TWeakObjectPtr<ABBCrystaBurstZone> ActiveZone;

	FTimerHandle ZoneExpiryHandle;
};
