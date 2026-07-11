#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Eluna_LMB.generated.h"

class ABBProjectile;
class UGameplayEffect;

/**
 * Eluna LMB — Crescent Bolt.
 *
 * Repeating projectile auto-attack. Tap for single shot, hold for continuous fire.
 * Each subsequent shot (while held) loses 10% range but gains 15% damage (stacks up to 3x).
 * LocalPredicted. bActivateOnInputHeld = true.
 */
UCLASS()
class BREACHBORNE_API UGA_Eluna_LMB : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Eluna_LMB();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Eluna|LMB")
	float BaseDamage = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Eluna|LMB")
	float FireInterval = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Eluna|LMB")
	float ProjectileSpeed = 3500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Eluna|LMB")
	float SpawnForwardOffset = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Eluna|LMB")
	int32 MaxHoldStacks = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Eluna|LMB")
	float RangeDecayPerStack = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Eluna|LMB")
	float DamageBoostPerStack = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Eluna|LMB")
	TSubclassOf<ABBProjectile> ProjectileClass;

private:
	void FireShot();
	void OnFireTimerTick();

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY()
	FTimerHandle FireTimerHandle;

	int32 CurrentHoldStacks = 0;
};
