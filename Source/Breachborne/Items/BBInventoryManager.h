#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "BBItemTypes.h"

class ABreachbornePlayerState;
class UBBEquipmentDefinition;
class UBBPowerDefinition;
class UBBConsumableDefinition;

/**
 * Server-authoritative inventory mutation logic. All operations are static methods
 * that validate preconditions, remove old GEs, apply new GEs, and update FRepInventoryData.
 * Callers MUST ensure HasAuthority() before calling.
 */
class BREACHBORNE_API FBBInventoryManager
{
public:
	/**
	 * Equip an item into the appropriate slot. Removes any existing item in that slot first.
	 * @return true if equipped successfully
	 */
	static bool EquipItem(ABreachbornePlayerState* PS, const UBBEquipmentDefinition* ItemDef);

	/**
	 * Unequip an item from the specified slot. Removes GE bonuses.
	 * @return true if slot was occupied and is now empty
	 */
	static bool UnequipSlot(ABreachbornePlayerState* PS, EEquipmentSlotType SlotType);

	/**
	 * Upgrade equipment in the specified slot. Requires sufficient shards.
	 * @return true if upgraded successfully
	 */
	static bool UpgradeEquipment(ABreachbornePlayerState* PS, EEquipmentSlotType SlotType);

	/**
	 * Choose an evolution path for equipment in the specified slot.
	 * Requires the item to be at EvolutionTier and not yet evolved.
	 * @return true if evolved successfully
	 */
	static bool EvolveEquipment(ABreachbornePlayerState* PS, EEquipmentSlotType SlotType, EEvolutionPath Path);

	/**
	 * Equip a power into the specified slot. Removes any existing power first.
	 * @return true if equipped successfully
	 */
	static bool EquipPower(ABreachbornePlayerState* PS, const UBBPowerDefinition* PowerDef, EPowerSlotIndex Slot);

	/** Look up and equip a power by stable PowerID. If no preferred slot is supplied, uses first empty slot or replaces Power1. */
	static bool EquipPowerByID(ABreachbornePlayerState* PS, FName PowerID, EPowerSlotIndex PreferredSlot = EPowerSlotIndex::Power1, bool bUsePreferredSlot = false);

	/**
	 * Remove the power from the specified slot.
	 * @return true if slot was occupied and is now empty
	 */
	static bool UnequipPower(ABreachbornePlayerState* PS, EPowerSlotIndex Slot);

	/**
	 * Set the armor tier. Applies armor GE and sets durability.
	 * @return true if armor tier changed
	 */
	static bool SetArmorTier(ABreachbornePlayerState* PS, EArmorTier NewTier);

	/** Add gold to the player's inventory. Negative amounts are clamped to 0. */
	static void AddGold(ABreachbornePlayerState* PS, int32 Amount);

	/** Spend gold if the player has enough. */
	static bool TrySpendGold(ABreachbornePlayerState* PS, int32 Amount);

	/** Add upgrade shards to the player's inventory. */
	static void AddShards(ABreachbornePlayerState* PS, int32 Amount);

	/**
	 * Add a consumable to the backpack. Stacks with existing if possible.
	 * @return true if added (false if backpack is full and no existing stack)
	 */
	static bool AddConsumable(ABreachbornePlayerState* PS, const UBBConsumableDefinition* ConsumableDef, int32 Count = 1);

	/** Add a named consumable stack without requiring a data asset. */
	static bool AddConsumableStack(ABreachbornePlayerState* PS, FName ConsumableID, int32 Count = 1, int32 MaxStackSize = 5);

	/** True if the backpack contains at least Count of a named consumable. */
	static bool HasConsumable(const ABreachbornePlayerState* PS, FName ConsumableID, int32 Count = 1);

	/** Consume Count of a named consumable from backpack stacks. */
	static bool ConsumeConsumable(ABreachbornePlayerState* PS, FName ConsumableID, int32 Count = 1);

	/**
	 * Use a consumable from the specified backpack slot index.
	 * @return true if consumed successfully
	 */
	static bool UseConsumable(ABreachbornePlayerState* PS, int32 BackpackSlotIndex);

	/** Repair armor durability by a fraction of the current tier's max durability. */
	static bool RepairArmorDurability(ABreachbornePlayerState* PS, float MaxDurabilityFraction);

	/** Reduce armor durability from incoming damage. Returns true when armor durability changed. */
	static bool ApplyArmorDurabilityDamage(ABreachbornePlayerState* PS, float DamageAmount);

private:
	/** Get the GE handle reference for a specific equipment slot */
	static FActiveGameplayEffectHandle& GetSlotGEHandle(ABreachbornePlayerState* PS, EEquipmentSlotType SlotType);

	/** Apply equipment stat bonuses for current slot state */
	static void ApplyEquipmentGE(ABreachbornePlayerState* PS, EEquipmentSlotType SlotType, const UBBEquipmentDefinition* ItemDef);

	/** Get armor value for a given tier */
	static float GetArmorValueForTier(EArmorTier Tier);

	/** Get max durability for a given tier */
	static float GetArmorMaxHP(EArmorTier Tier);

	/** Sync HealthSet Armor attribute from current replicated inventory durability. */
	static void RefreshArmorAttribute(ABreachbornePlayerState* PS);
};
