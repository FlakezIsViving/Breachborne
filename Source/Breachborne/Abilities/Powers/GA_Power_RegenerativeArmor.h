#pragma once

#include "CoreMinimal.h"
#include "BBPowerGameplayAbility.h"
#include "GA_Power_RegenerativeArmor.generated.h"

UCLASS()
class BREACHBORNE_API UGA_Power_RegenerativeArmor : public UBBPowerGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Power_RegenerativeArmor();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers")
	float RepairFractionPerTick = 0.025f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers")
	float TickInterval = 2.0f;

private:
	void RepairTick();

	FTimerHandle RepairTimerHandle;
};
