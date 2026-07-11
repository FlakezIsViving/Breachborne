#pragma once

#include "CoreMinimal.h"
#include "BBItemTypes.h"

class ABBWorldItem;

/**
 * Static helpers for spawning world item pickups. Server only.
 */
class BREACHBORNE_API FBBWorldItemSpawner
{
public:
	/** Spawn an equipment pickup at the given location */
	static ABBWorldItem* SpawnEquipmentPickup(UWorld* World, const FVector& Location, FName ItemID, EEquipmentSlotType SlotType);

	/** Spawn a gold pickup at the given location */
	static ABBWorldItem* SpawnGoldPickup(UWorld* World, const FVector& Location, int32 Amount);

	/** Spawn an upgrade shard pickup at the given location */
	static ABBWorldItem* SpawnShardPickup(UWorld* World, const FVector& Location, int32 Amount);

	/** Spawn a consumable pickup at the given location */
	static ABBWorldItem* SpawnConsumablePickup(UWorld* World, const FVector& Location, FName ConsumableID, int32 Count = 1);

	/** Spawn a power pickup at the given location */
	static ABBWorldItem* SpawnPowerPickup(UWorld* World, const FVector& Location, FName PowerID);

private:
	/** Internal: spawn a world item actor at location */
	static ABBWorldItem* SpawnBase(UWorld* World, const FVector& Location);
};
