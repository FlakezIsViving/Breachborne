#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Void_Shift.generated.h"

class ABBVoidSwapProjectile;

UCLASS()
class BREACHBORNE_API UGA_Void_Shift : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Void_Shift();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Shift")
	float CooldownDuration = 14.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Shift")
	float Range = 1900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Shift")
	float Radius = 360.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Shift")
	float EmpoweredRadius = 430.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Shift")
	float TravelDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Shift")
	float EmpoweredTravelDuration = 1.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Shift")
	float SpawnForwardOffset = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|Shift")
	TSubclassOf<ABBVoidSwapProjectile> ProjectileClass;

private:
	bool ConsumeEmpowered() const;

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
