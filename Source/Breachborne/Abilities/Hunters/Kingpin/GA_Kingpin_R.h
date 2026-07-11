#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Kingpin_R.generated.h"

/**
 * Kingpin R - Shotgun ultimate.
 *
 * Fires two sawed-off shotgun blasts back to back. Each shell hits enemies in a
 * short forward cone, dealing burst damage and applying anti-heal.
 *
 * ServerInitiated. 45s cooldown.
 */
UCLASS()
class BREACHBORNE_API UGA_Kingpin_R : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Kingpin_R();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|R")
	float ShotgunRange = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|R")
	float ShotgunHalfAngle = 28.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|R")
	float DamagePerShell = 85.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|R")
	float ShellDelay = 0.18f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|R")
	float AntiHealDuration = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Kingpin|R")
	float CooldownDuration = 45.0f;

private:
	void FireShell();
	void ApplyShellHit(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, AActor* InstigatorActor);

	FTimerHandle ShellTimerHandle;

	int32 ShellsFired = 0;

	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
