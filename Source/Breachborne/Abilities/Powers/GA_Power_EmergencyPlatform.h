#pragma once

#include "CoreMinimal.h"
#include "BBPowerGameplayAbility.h"
#include "GA_Power_EmergencyPlatform.generated.h"

UCLASS()
class BREACHBORNE_API UGA_Power_EmergencyPlatform : public UBBPowerGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Power_EmergencyPlatform();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers")
	float SpawnDistance = 260.0f;
};
