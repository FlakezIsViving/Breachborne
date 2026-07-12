#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Ghost_R.generated.h"

class ABBNapalmZone;
class ABBPrimitiveBurstActor;

/**
 * Ghost's R — Napalm Strike.
 * Server-spawned AoE DOT zone at cursor location. Unlocks at Level 5. 90s cooldown. ServerInitiated.
 * bIgnoreLevelGate allows testing at any level during development.
 */
UCLASS()
class BREACHBORNE_API UGA_Ghost_R : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Ghost_R();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|R")
	float CooldownDuration = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|R")
	int32 RequiredLevel = 5;

	/** Dev bypass: skip the level gate for testing. Set false for production. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|R")
	bool bIgnoreLevelGate = true;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|R")
	float DamagePerTick = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|R")
	float MaxRange = 5000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|R")
	TSubclassOf<ABBNapalmZone> NapalmZoneClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|R|Visual")
	TSubclassOf<ABBPrimitiveBurstActor> WarningVisualClass;

private:
	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
