#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Eluna_RMB.generated.h"

class ABBProjectile;
class UGameplayEffect;

/**
 * Eluna RMB — Darkside Binding.
 *
 * Fires a slow sticky projectile. After a delay, it explodes and roots enemies in radius.
 * Root uses UBBStunTagEffect (reuse stun effect for root behavior).
 * 10s cooldown. LocalPredicted.
 */
UCLASS()
class BREACHBORNE_API UGA_Eluna_RMB : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Eluna_RMB();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	float BaseDamage = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	float ProjectileSpeed = 800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	float SpawnForwardOffset = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	float ExplosionDelay = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	float RootRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	float RootDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	float CooldownDuration = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	TSubclassOf<ABBProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	TSubclassOf<UGameplayEffect> RootEffectClass;

private:
	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
