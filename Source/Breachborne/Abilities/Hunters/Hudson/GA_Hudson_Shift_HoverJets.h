#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Hudson_Shift_HoverJets.generated.h"

class UGameplayEffect;

/** Hudson Shift - burst/hover mobility with hit knockback. */
UCLASS()
class BREACHBORNE_API UGA_Hudson_Shift_HoverJets : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Hudson_Shift_HoverJets();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void OnInputReleased() override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Shift")
	float CooldownDuration = 12.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Shift")
	float MaxHoverDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Shift")
	float BurstSpeed = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Shift")
	float HoverSpeed = 950.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Shift")
	float HitRadius = 105.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Shift")
	float HitDamage = 45.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Shift")
	float KnockbackForce = 1200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Shift")
	float CooldownRefundFractionPerHit = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Shift")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

private:
	void HoverTick();
	void PlayHoverLoopMontage();
	void ReduceOwnCooldown(float Seconds);

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;

	FTimerHandle HoverTickHandle;
	FTimerHandle HoverEndHandle;
	FTimerHandle HoverLoopMontageHandle;
	FVector HoverDirection = FVector::ForwardVector;
	TSet<TWeakObjectPtr<AActor>> HitActors;
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;
};
