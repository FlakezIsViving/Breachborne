#pragma once

#include "CoreMinimal.h"
#include "BBItemTypes.h"
#include "BBInventoryTypes.generated.h"

/**
 * Replicated inventory data struct. Lives on PlayerState as a single replicated property.
 * Server mutates, clients receive via OnRep. Entire struct replicates on any change.
 * With max 4 consumable slots + 2 power slots + 3 equipment slots, this is <200 bytes.
 */
USTRUCT(BlueprintType)
struct FRepInventoryData
{
	GENERATED_BODY()

	// --- Equipment Slots ---

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Inventory")
	FEquipmentSlotState Weapon;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Inventory")
	FEquipmentSlotState Helmet;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Inventory")
	FEquipmentSlotState Boots;

	// --- Armor ---

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Inventory")
	FArmorState Armor;

	// --- Power Slots (max 2) ---

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Inventory")
	FPowerSlotState Power1;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Inventory")
	FPowerSlotState Power2;

	// --- Consumable Backpack (max 4 slots) ---

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Inventory")
	TArray<FConsumableStack> Backpack;

	// --- Economy ---

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Inventory")
	int32 Gold = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Inventory")
	int32 UpgradeShards = 0;

	// --- Helpers ---

	/** Get equipment slot state by slot type */
	const FEquipmentSlotState& GetSlot(EEquipmentSlotType SlotType) const;

	/** Get mutable equipment slot state by slot type (server only) */
	FEquipmentSlotState& GetSlotMutable(EEquipmentSlotType SlotType);

	/** Get power slot by index */
	const FPowerSlotState& GetPowerSlot(EPowerSlotIndex Index) const;
	FPowerSlotState& GetPowerSlotMutable(EPowerSlotIndex Index);

	/** Find a backpack slot with this consumable ID, or empty slot. Returns INDEX_NONE if full. */
	int32 FindConsumableSlot(FName ConsumableID) const;

	/** Find first empty backpack slot. Returns INDEX_NONE if full. */
	int32 FindEmptyBackpackSlot() const;

	/** Initialize backpack with MaxSlots empty entries */
	void InitBackpack(int32 MaxSlots = 4);
};
