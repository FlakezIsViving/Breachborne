#include "BBConsumableDefinition.h"

FPrimaryAssetId UBBConsumableDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("BBConsumable"), ConsumableID);
}
