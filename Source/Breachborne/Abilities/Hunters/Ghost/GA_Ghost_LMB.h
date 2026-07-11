#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Ghost_LMB.generated.h"

class UBBDamageEffect;
class ABBProjectile;

/**
 * Ghost's Primary Fire — Automatic projectile rifle.
 * Held input: fires repeatedly while LMB is held. Releases on LMB up.
 * LocalPredicted: server spawns projectiles, clients see them via replication.
 */
UCLASS()
class BREACHBORNE_API UGA_Ghost_LMB : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Ghost_LMB();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** Damage per shot */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Ghost|LMB")
	float BaseDamagePerShot = 25.0f;

	/** Time between shots (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Ghost|LMB")
	float FireInterval = 0.15f;

	/** Projectile class to spawn (defaults to ABBProjectile) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Ghost|LMB")
	TSubclassOf<ABBProjectile> ProjectileClass;

	/** Forward offset from character to spawn the projectile (avoids self-overlap) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Ghost|LMB")
	float SpawnForwardOffset = 50.0f;

private:
	/** Fire one shot: spawn projectile on server */
	void FireShot();

	/** Timer callback for repeated fire */
	void OnFireTimerTick();

	FTimerHandle FireTimerHandle;

	/** Cached damage effect class */
	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};
