#pragma once

#include "CoreMinimal.h"
#include "BBAttributeSetBase.h"
#include "AbilitySystemComponent.h"
#include "BBMovementSet.generated.h"

#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

/**
 * Movement-related attributes: speed, glide speed, dash distance.
 * Modified by boots, buffs, and abilities.
 * NOT replicated directly — PlayerState proxies attribute values to avoid subobject misrouting.
 */
UCLASS()
class BREACHBORNE_API UBBMovementSet : public UBBAttributeSetBase
{
	GENERATED_BODY()

public:
	UBBMovementSet();

	ATTRIBUTE_ACCESSORS(UBBMovementSet, MoveSpeed);
	ATTRIBUTE_ACCESSORS(UBBMovementSet, GlideSpeed);
	ATTRIBUTE_ACCESSORS(UBBMovementSet, DashDistance);

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Movement")
	FGameplayAttributeData MoveSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Movement")
	FGameplayAttributeData GlideSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Movement")
	FGameplayAttributeData DashDistance;
};
