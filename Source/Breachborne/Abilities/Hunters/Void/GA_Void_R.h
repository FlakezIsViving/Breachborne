#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Void_R.generated.h"

class ABBVoidSingularityActor;
class UGameplayEffect;

UCLASS()
class BREACHBORNE_API UGA_Void_R : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Void_R();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|R")
	float CooldownDuration = 75.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|R")
	float Range = 2300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|R")
	float Radius = 620.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|R")
	float WarningDelay = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|R")
	float ActiveDuration = 1.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|R")
	TSubclassOf<ABBVoidSingularityActor> SingularityClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|R")
	TSubclassOf<UGameplayEffect> StunEffectClass;

private:
	bool ConsumeEmpowered() const;

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
