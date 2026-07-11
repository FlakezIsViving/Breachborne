#include "BBPowerDefinition.h"

FPrimaryAssetId UBBPowerDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("BBPower"), PowerID);
}

void UBBPowerDefinition::PostLoad()
{
	Super::PostLoad();
	NormalizeLegacyFields();
}

#if WITH_EDITOR
void UBBPowerDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	NormalizeLegacyFields();
}
#endif

void UBBPowerDefinition::NormalizeLegacyFields()
{
	if (bDropsOnDeath)
	{
		bDropOnDeath = true;
	}
	else
	{
		bDropsOnDeath = bDropOnDeath;
	}

	bIsPassive = ActivationType == EBBPowerActivationType::Passive;
	if (ActivationType != EBBPowerActivationType::Active && !bAutoActivateOnEquip)
	{
		bAutoActivateOnEquip = true;
	}
}
