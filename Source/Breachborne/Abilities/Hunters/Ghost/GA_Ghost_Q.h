#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Ghost_Q.generated.h"

class ABBPrimitiveBeamActor;
class ABBPrimitiveBurstActor;

/**
 * Ghost's Q Ability — Piercing Laser.
 * Single-activation line trace that pierces through ALL targets in line.
 * High damage, 8 second cooldown. LocalPredicted.
 */
UCLASS()
class BREACHBORNE_API UGA_Ghost_Q : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Ghost_Q();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Ghost|Q")
	float BaseDamage = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Ghost|Q")
	float MaxRange = 5000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Ghost|Q")
	float CooldownDuration = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|Q|Visual")
	TSubclassOf<ABBPrimitiveBeamActor> BeamVisualClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|Q|Visual")
	TSubclassOf<ABBPrimitiveBurstActor> BurstVisualClass;

private:
	/** Cooldown tags container — built once, returned by GetCooldownTags */
	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};
