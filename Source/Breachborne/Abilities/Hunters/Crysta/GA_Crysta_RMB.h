#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Crysta_RMB.generated.h"

class ABBCrystaFluxSnareProjectile;
class UAbilityTask_WaitInputRelease;
class UGameplayEffect;

UCLASS()
class BREACHBORNE_API UGA_Crysta_RMB : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Crysta_RMB();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void OnInputReleased() override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|RMB")
	float CooldownDuration = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|RMB")
	float SpawnForwardOffset = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|RMB")
	float HoldReturnThreshold = 0.12f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|RMB")
	TSubclassOf<ABBCrystaFluxSnareProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|RMB")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|RMB")
	TSubclassOf<UGameplayEffect> MarkEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|RMB")
	TSubclassOf<UGameplayEffect> GroundedEffectClass;

private:
	UFUNCTION()
	void HandleInputRelease(float TimeHeld);

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;

	UPROPERTY()
	TWeakObjectPtr<ABBCrystaFluxSnareProjectile> ActiveProjectile;

	float ActivationWorldTime = 0.0f;
	bool bReleaseHandled = false;
};
