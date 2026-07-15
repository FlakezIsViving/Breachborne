#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Crysta_Passive.generated.h"

class UAbilitySystemComponent;

UCLASS()
class BREACHBORNE_API UGA_Crysta_Passive : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Crysta_Passive();
	static bool ReduceActiveCooldownTag(UAbilitySystemComponent* ASC, FGameplayTag CooldownTag, float ReductionSeconds);

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Passive")
	float ShiftCooldownReduction = 1.5f;

private:
	void OnReverberationDetonated(FGameplayTag Tag, const FGameplayEventData* Payload);

	FDelegateHandle DetonationEventHandle;
};
