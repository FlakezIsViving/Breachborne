#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Void_LMB.generated.h"

class ABBVoidOrbProjectile;
class UGameplayEffect;

UCLASS()
class BREACHBORNE_API UGA_Void_LMB : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Void_LMB();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|LMB")
	float FireInterval = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|LMB")
	float ChargedIdleSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|LMB")
	float BaseDamage = 42.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|LMB")
	float ChargedDamage = 70.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|LMB")
	float SpawnForwardOffset = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|LMB")
	TSubclassOf<ABBVoidOrbProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|LMB")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|LMB")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

private:
	void FireShot();
	void OnFireTimerTick();

	FTimerHandle FireTimerHandle;
	float LastFireWorldTime = -1000.0f;
};
