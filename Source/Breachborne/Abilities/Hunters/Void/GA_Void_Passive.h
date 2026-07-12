#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Void_Passive.generated.h"

UCLASS()
class BREACHBORNE_API UGA_Void_Passive : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Void_Passive();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Passive")
	int32 HitsToEmpower = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Passive")
	float HitWindowSeconds = 6.0f;

private:
	void OnDamageDealt(FGameplayTag Tag, const FGameplayEventData* Payload);

	FDelegateHandle DamageEventHandle;
	float WindowStartWorldTime = 0.0f;
	int32 ConfirmedHits = 0;
};
