#pragma once

#include "CoreMinimal.h"
#include "BBAttributeSetBase.h"
#include "AbilitySystemComponent.h"
#include "BBWispHealthSet.generated.h"

#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWispHealthDepleted, UAbilitySystemComponent*, OwnerASC);

/**
 * Wisp health attributes. Lives on PlayerState alongside other attribute sets.
 * WispHP drains over time while the player is in wisp form. When it reaches 0,
 * the wisp dies and becomes a deathbox.
 * IncomingWispDrain is a meta attribute used for processing drain/stomp amounts.
 */
UCLASS()
class BREACHBORNE_API UBBWispHealthSet : public UBBAttributeSetBase
{
	GENERATED_BODY()

public:
	UBBWispHealthSet();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	ATTRIBUTE_ACCESSORS(UBBWispHealthSet, WispHP);
	ATTRIBUTE_ACCESSORS(UBBWispHealthSet, MaxWispHP);
	ATTRIBUTE_ACCESSORS(UBBWispHealthSet, IncomingWispDrain);

	/** Broadcast when WispHP reaches 0. Server only. */
	UPROPERTY(BlueprintAssignable, Category = "Breachborne|Wisp")
	FOnWispHealthDepleted OnWispHealthDepleted;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Wisp")
	FGameplayAttributeData WispHP;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Wisp")
	FGameplayAttributeData MaxWispHP;

	/** Meta attribute — NOT replicated. Set by drain/stomp effects, processed in PostGameplayEffectExecute. */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Wisp")
	FGameplayAttributeData IncomingWispDrain;
};
