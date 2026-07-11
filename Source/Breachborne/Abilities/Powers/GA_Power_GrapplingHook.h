#pragma once

#include "CoreMinimal.h"
#include "BBPowerGameplayAbility.h"
#include "GA_Power_GrapplingHook.generated.h"

UCLASS()
class BREACHBORNE_API UGA_Power_GrapplingHook : public UBBPowerGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Power_GrapplingHook();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|GrapplingHook")
	float Range = 2400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|GrapplingHook")
	float InitialPullSpeed = 1900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|GrapplingHook")
	float MaxPullSpeed = 4400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|GrapplingHook")
	float StopDistance = 145.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|GrapplingHook")
	float MaxPullDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|GrapplingHook")
	float PullTickInterval = 0.02f;
};
