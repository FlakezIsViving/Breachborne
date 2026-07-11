#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "BBAttributeSetBase.generated.h"

class UAbilitySystemComponent;

/**
 * Base class for all Breachborne AttributeSets.
 * Caches the owning ASC pointer to bypass a UE subobject replication issue where
 * GetOuter() on CreateDefaultSubobject UObject subobjects can return the wrong
 * PlayerState on some clients, causing GAMEPLAYATTRIBUTE_REPNOTIFY to update
 * the wrong ASC's internal attribute cache.
 *
 * The cache is set in the PlayerState constructor where the association is
 * guaranteed correct, and persists for the lifetime of the object.
 */
UCLASS(Abstract)
class BREACHBORNE_API UBBAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()

public:
	/** Cache the owning ASC — call this in the PlayerState constructor. */
	void SetCachedAbilitySystemComponent(UAbilitySystemComponent* InASC) { CachedAbilitySystemComponent = InASC; }

	/**
	 * Shadows UAttributeSet::GetOwningAbilitySystemComponent().
	 * Returns the cached ASC if available, otherwise falls back to base class (GetOuter chain).
	 * Because GAMEPLAYATTRIBUTE_REPNOTIFY expands in subclass methods, C++ name lookup
	 * finds this shadowed version first, fixing the routing.
	 */
	UAbilitySystemComponent* GetOwningAbilitySystemComponent() const;

private:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;
};
