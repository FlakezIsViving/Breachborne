#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Eluna_Passive.generated.h"

/**
 * Eluna Passive — Soul Pack.
 *
 * Heals nearby allies every 5s.
 * Automatically carries a nearby allied wisp. Carry freezes normal decay and
 * continues revival through enemy contest, but hard CC drops the wisp.
 * Passive (always active, no input required).
 */
UCLASS()
class BREACHBORNE_API UGA_Eluna_Passive : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Eluna_Passive();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Passive")
	float HealAmount = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Passive")
	float HealInterval = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Passive")
	float HealRadius = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Passive")
	float WispPickupRadius = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Passive")
	float WispDropDistance = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Passive")
	TSubclassOf<UGameplayEffect> HealEffectClass;

private:
	void TickHeal();
	void TickWispPickup();

	FTimerHandle HealTimerHandle;
	FTimerHandle WispPickupTimerHandle;
	TWeakObjectPtr<class ABBWispPawn> CarriedWisp;
};
