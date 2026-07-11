#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Ghost_RMB.generated.h"

class ABBGrenade;
class UGameplayEffect;

/**
 * Ghost's RMB — Spike Grenade.
 * Single-fire: spawns an AoE grenade projectile toward the cursor.
 * Grenade explodes on enemy hit, wall hit, or after fuse expires.
 * 10s cooldown. LocalPredicted.
 */
UCLASS()
class BREACHBORNE_API UGA_Ghost_RMB : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Ghost_RMB();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|RMB")
	float CooldownDuration = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|RMB")
	float GrenadeDamage = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|RMB")
	TSubclassOf<ABBGrenade> GrenadeClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|RMB")
	float SpawnForwardOffset = 50.0f;

private:
	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
