#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "Breachborne/Items/BBItemTypes.h"
#include "BBPowerGameplayAbility.generated.h"

class UBBPowerDefinition;

UCLASS(Abstract)
class BREACHBORNE_API UBBPowerGameplayAbility : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UBBPowerGameplayAbility();

protected:
	const UBBPowerDefinition* GetPowerDefinition() const;
	FName GetPowerID() const;
	EPowerSlotIndex GetPowerSlot() const;
	FGameplayTag GetPowerInputTag() const;
	float GetPowerCooldownSeconds() const;
	void ApplyPowerCooldown(float OverrideDuration = -1.0f) const;
	bool IsPowerOnCooldown() const;
};
