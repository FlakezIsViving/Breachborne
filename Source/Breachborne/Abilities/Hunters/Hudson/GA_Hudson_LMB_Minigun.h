#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Hudson_LMB_Minigun.generated.h"

class UGameplayEffect;

/** Hudson LMB - held minigun fire with primitive tracer readability. */
UCLASS()
class BREACHBORNE_API UGA_Hudson_LMB_Minigun : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Hudson_LMB_Minigun();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|LMB")
	float BaseDamage = 35.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|LMB")
	float NormalRange = 1150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|LMB")
	float SpunUpRange = 1300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|LMB")
	float NormalFireInterval = 0.14f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|LMB")
	float SpunUpFireInterval = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|LMB")
	float InitialSpreadDegrees = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|LMB")
	float MinimumSpreadDegrees = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|LMB")
	float AccuracyRampSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|LMB")
	float SpunUpHealMaxHealthFraction = 0.003f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|LMB")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|LMB")
	TSubclassOf<UGameplayEffect> HealEffectClass;

private:
	void FireShot();
	void ScheduleNextShot();
	bool ResolveTarget(AActor* Actor, UAbilitySystemComponent*& OutASC, int32& OutTeamID) const;
	void ApplyDamage(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float Damage);
	void ApplySelfHeal(UAbilitySystemComponent* SourceASC, float HealAmount);

	FTimerHandle FireTimerHandle;
	float FireStartTime = 0.0f;
};
