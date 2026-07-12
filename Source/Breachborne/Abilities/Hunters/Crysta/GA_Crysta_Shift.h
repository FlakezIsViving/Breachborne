#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Crysta_Shift.generated.h"

class ABBPrimitiveBeamActor;

UCLASS()
class BREACHBORNE_API UGA_Crysta_Shift_Primary : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Crysta_Shift_Primary();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Shift")
	float DashSpeed = 1650.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Shift")
	float DashLift = 35.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Shift")
	float CooldownDuration = 12.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|Shift|Visuals")
	TSubclassOf<ABBPrimitiveBeamActor> DashTrailClass;

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};

UCLASS()
class BREACHBORNE_API UGA_Crysta_Shift_Secondary : public UGA_Crysta_Shift_Primary
{
	GENERATED_BODY()

public:
	UGA_Crysta_Shift_Secondary();
};
