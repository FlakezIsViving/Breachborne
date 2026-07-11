#pragma once

#include "CoreMinimal.h"
#include "BBPowerGameplayAbility.h"
#include "GA_Power_TacticalNuke.generated.h"

class UGameplayEffect;

UCLASS()
class BREACHBORNE_API UGA_Power_TacticalNuke : public UBBPowerGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Power_TacticalNuke();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	float Radius = 850.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	float ImpactDelay = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	float ImpactDamage = 99999.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	float BurnDuration = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	float BurnDamagePerTick = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	float BurnTickInterval = 0.5f;
};
