#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Void_Q.generated.h"

class ABBVoidAnomalyPuddle;
class UGameplayEffect;

UCLASS()
class BREACHBORNE_API UGA_Void_Q : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Void_Q();

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
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Q")
	float CooldownDuration = 11.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Q")
	float Range = 1650.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Q")
	float Radius = 460.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Q")
	float EmpoweredRadius = 620.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Q")
	float Duration = 3.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Q")
	float TickDamage = 18.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Q")
	float BurstDamage = 85.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Q")
	TSubclassOf<ABBVoidAnomalyPuddle> PuddleClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Q")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

private:
	void SpawnPuddle();
	void OnPuddleExpired();
	bool ConsumeEmpowered() const;

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;

	UPROPERTY()
	TWeakObjectPtr<ABBVoidAnomalyPuddle> ActivePuddle;

	FTimerHandle PuddleExpiryHandle;
};
