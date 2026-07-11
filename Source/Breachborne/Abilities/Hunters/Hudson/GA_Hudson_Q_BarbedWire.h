#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Hudson_Q_BarbedWire.generated.h"

class ABBHudsonBarbedWireActor;

/** Hudson Q - deploy primitive barbed wire control zone. */
UCLASS()
class BREACHBORNE_API UGA_Hudson_Q_BarbedWire : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Hudson_Q_BarbedWire();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Q")
	float CooldownDuration = 18.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Q")
	float Range = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Q")
	float Duration = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Q")
	float DamagePerSecond = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|Q")
	TSubclassOf<ABBHudsonBarbedWireActor> WireClass;

private:
	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
