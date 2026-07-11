#pragma once

#include "CoreMinimal.h"
#include "BBAttributeSetBase.h"
#include "AbilitySystemComponent.h"
#include "BBHealthSet.generated.h"

#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthDepleted, UAbilitySystemComponent*, VictimASC, UAbilitySystemComponent*, KillerASC);

/**
 * Health and Shield attributes. Lives on PlayerState as subobject of ASC.
 * IncomingDamage/IncomingHealing are meta attributes — NOT replicated, used for processing only.
 */
UCLASS()
class BREACHBORNE_API UBBHealthSet : public UBBAttributeSetBase
{
	GENERATED_BODY()

public:
	UBBHealthSet();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	ATTRIBUTE_ACCESSORS(UBBHealthSet, Health);
	ATTRIBUTE_ACCESSORS(UBBHealthSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(UBBHealthSet, Shield);
	ATTRIBUTE_ACCESSORS(UBBHealthSet, MaxShield);
	ATTRIBUTE_ACCESSORS(UBBHealthSet, Armor);
	ATTRIBUTE_ACCESSORS(UBBHealthSet, HealingReceivedMultiplier);
	ATTRIBUTE_ACCESSORS(UBBHealthSet, IncomingDamage);
	ATTRIBUTE_ACCESSORS(UBBHealthSet, IncomingHealing);

	/** Broadcast when Health reaches 0. Server only. */
	UPROPERTY(BlueprintAssignable, Category = "Breachborne|Health")
	FOnHealthDepleted OnHealthDepleted;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Health")
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Health")
	FGameplayAttributeData MaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Health")
	FGameplayAttributeData Shield;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Health")
	FGameplayAttributeData MaxShield;

	/** Flat damage reduction from equipped armor. Set by equipment GEs. */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Health")
	FGameplayAttributeData Armor;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Health")
	FGameplayAttributeData HealingReceivedMultiplier;

	/** Meta attribute — NOT replicated. Set by DamageExecution, processed in PostGameplayEffectExecute. */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Health")
	FGameplayAttributeData IncomingDamage;

	/** Meta attribute — NOT replicated. Set by healing effects, processed in PostGameplayEffectExecute. */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Health")
	FGameplayAttributeData IncomingHealing;

};
