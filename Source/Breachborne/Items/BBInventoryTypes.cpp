#include "BBInventoryTypes.h"

// Static dummy for returning references when slot type is invalid
static FEquipmentSlotState GDummyEquipmentSlot;
static FPowerSlotState GDummyPowerSlot;

const FEquipmentSlotState& FRepInventoryData::GetSlot(EEquipmentSlotType SlotType) const
{
	switch (SlotType)
	{
	case EEquipmentSlotType::Weapon: return Weapon;
	case EEquipmentSlotType::Helmet: return Helmet;
	case EEquipmentSlotType::Boots:  return Boots;
	default:
		return GDummyEquipmentSlot;
	}
}

FEquipmentSlotState& FRepInventoryData::GetSlotMutable(EEquipmentSlotType SlotType)
{
	switch (SlotType)
	{
	case EEquipmentSlotType::Weapon: return Weapon;
	case EEquipmentSlotType::Helmet: return Helmet;
	case EEquipmentSlotType::Boots:  return Boots;
	default:
		GDummyEquipmentSlot.Clear();
		return GDummyEquipmentSlot;
	}
}

const FPowerSlotState& FRepInventoryData::GetPowerSlot(EPowerSlotIndex Index) const
{
	switch (Index)
	{
	case EPowerSlotIndex::Power1: return Power1;
	case EPowerSlotIndex::Power2: return Power2;
	default:
		return GDummyPowerSlot;
	}
}

FPowerSlotState& FRepInventoryData::GetPowerSlotMutable(EPowerSlotIndex Index)
{
	switch (Index)
	{
	case EPowerSlotIndex::Power1: return Power1;
	case EPowerSlotIndex::Power2: return Power2;
	default:
		GDummyPowerSlot.Clear();
		return GDummyPowerSlot;
	}
}

int32 FRepInventoryData::FindConsumableSlot(FName ConsumableID) const
{
	// First, find an existing stack of the same consumable
	for (int32 i = 0; i < Backpack.Num(); ++i)
	{
		if (Backpack[i].ConsumableID == ConsumableID && Backpack[i].Count > 0)
		{
			return i;
		}
	}

	// If no existing stack, find an empty slot
	return FindEmptyBackpackSlot();
}

int32 FRepInventoryData::FindEmptyBackpackSlot() const
{
	for (int32 i = 0; i < Backpack.Num(); ++i)
	{
		if (Backpack[i].IsEmpty())
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void FRepInventoryData::InitBackpack(int32 MaxSlots)
{
	Backpack.SetNum(MaxSlots);
	for (FConsumableStack& Slot : Backpack)
	{
		Slot.Clear();
	}
}
