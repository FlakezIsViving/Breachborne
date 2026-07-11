#include "BBAttributeSetBase.h"
#include "AbilitySystemComponent.h"

UAbilitySystemComponent* UBBAttributeSetBase::GetOwningAbilitySystemComponent() const
{
	if (CachedAbilitySystemComponent)
	{
		return CachedAbilitySystemComponent;
	}
	return UAttributeSet::GetOwningAbilitySystemComponent();
}
