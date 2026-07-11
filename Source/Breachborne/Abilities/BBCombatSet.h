#pragma once

#include "CoreMinimal.h"
#include "BBAttributeSetBase.h"
#include "AbilitySystemComponent.h"
#include "BBCombatSet.generated.h"

#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

/**
 * Combat attributes: attack power, ability power, crit stats.
 * Modified by equipment, buffs, and level scaling.
 * NOT replicated directly — PlayerState proxies attribute values to avoid subobject misrouting.
 */
UCLASS()
class BREACHBORNE_API UBBCombatSet : public UBBAttributeSetBase
{
	GENERATED_BODY()

public:
	UBBCombatSet();

	ATTRIBUTE_ACCESSORS(UBBCombatSet, AttackPower);
	ATTRIBUTE_ACCESSORS(UBBCombatSet, AbilityPower);
	ATTRIBUTE_ACCESSORS(UBBCombatSet, CooldownReduction);
	ATTRIBUTE_ACCESSORS(UBBCombatSet, CritChance);
	ATTRIBUTE_ACCESSORS(UBBCombatSet, CritMultiplier);

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Combat")
	FGameplayAttributeData AttackPower;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Combat")
	FGameplayAttributeData AbilityPower;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Combat")
	FGameplayAttributeData CooldownReduction;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Combat")
	FGameplayAttributeData CritChance;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Combat")
	FGameplayAttributeData CritMultiplier;
};
