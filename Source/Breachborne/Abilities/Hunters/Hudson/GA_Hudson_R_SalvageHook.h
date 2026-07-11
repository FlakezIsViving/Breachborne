#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Hudson_R_SalvageHook.generated.h"

class UGameplayEffect;

/** Hudson R - hook, anti-heal, and execute threshold. */
UCLASS()
class BREACHBORNE_API UGA_Hudson_R_SalvageHook : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Hudson_R_SalvageHook();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	float CooldownDuration = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	float Range = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	float HookDuration = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	float ExecuteHealthFraction = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	float ExecuteDamage = 5000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	float MaxReelSeconds = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	float MinReelSeconds = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	float ReelTickInterval = 0.03f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	float ReelStopDistance = 140.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	float ExecuteCooldownRefundFraction = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|R")
	TSubclassOf<UGameplayEffect> AntiHealEffectClass;

private:
	void HookTick();
	void StartReel();
	void ReelTarget();
	void FinishExecute();
	void ReduceOwnCooldownByFraction(float Fraction);

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;

	FTimerHandle HookTickHandle;
	FTimerHandle HookEndHandle;
	TWeakObjectPtr<AHunterCharacter> HookedTarget;
	TWeakObjectPtr<UAbilitySystemComponent> HookedTargetASC;
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;
	FVector ReelStartLocation = FVector::ZeroVector;
	TEnumAsByte<EMovementMode> TargetPreviousMovementMode = MOVE_Walking;
	float ReelElapsedSeconds = 0.0f;
	float ReelDurationSeconds = 0.0f;
	bool bReeling = false;
	bool bExecuted = false;
};
