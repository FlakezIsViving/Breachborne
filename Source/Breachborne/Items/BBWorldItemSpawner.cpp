#include "BBWorldItemSpawner.h"
#include "BBWorldItem.h"
#include "Breachborne/Breachborne.h"

ABBWorldItem* FBBWorldItemSpawner::SpawnBase(UWorld* World, const FVector& Location)
{
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return World->SpawnActor<ABBWorldItem>(ABBWorldItem::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
}

ABBWorldItem* FBBWorldItemSpawner::SpawnEquipmentPickup(UWorld* World, const FVector& Location, FName ItemID, EEquipmentSlotType SlotType)
{
	ABBWorldItem* Item = SpawnBase(World, Location);
	if (Item)
	{
		Item->InitAsEquipment(ItemID, SlotType);
		UE_LOG(LogBreachborne, Log, TEXT("WorldItemSpawner: Spawned equipment %s at %s"), *ItemID.ToString(), *Location.ToString());
	}
	return Item;
}

ABBWorldItem* FBBWorldItemSpawner::SpawnGoldPickup(UWorld* World, const FVector& Location, int32 Amount)
{
	ABBWorldItem* Item = SpawnBase(World, Location);
	if (Item)
	{
		Item->InitAsGold(Amount);
		UE_LOG(LogBreachborne, Log, TEXT("WorldItemSpawner: Spawned %d gold at %s"), Amount, *Location.ToString());
	}
	return Item;
}

ABBWorldItem* FBBWorldItemSpawner::SpawnShardPickup(UWorld* World, const FVector& Location, int32 Amount)
{
	ABBWorldItem* Item = SpawnBase(World, Location);
	if (Item)
	{
		Item->InitAsShards(Amount);
		UE_LOG(LogBreachborne, Log, TEXT("WorldItemSpawner: Spawned %d shards at %s"), Amount, *Location.ToString());
	}
	return Item;
}

ABBWorldItem* FBBWorldItemSpawner::SpawnConsumablePickup(UWorld* World, const FVector& Location, FName ConsumableID, int32 Count)
{
	ABBWorldItem* Item = SpawnBase(World, Location);
	if (Item)
	{
		Item->InitAsConsumable(ConsumableID, Count);
		UE_LOG(LogBreachborne, Log, TEXT("WorldItemSpawner: Spawned consumable %s x%d at %s"), *ConsumableID.ToString(), Count, *Location.ToString());
	}
	return Item;
}

ABBWorldItem* FBBWorldItemSpawner::SpawnPowerPickup(UWorld* World, const FVector& Location, FName PowerID)
{
	ABBWorldItem* Item = SpawnBase(World, Location);
	if (Item)
	{
		Item->InitAsPower(PowerID);
		UE_LOG(LogBreachborne, Log, TEXT("WorldItemSpawner: Spawned power %s at %s"), *PowerID.ToString(), *Location.ToString());
	}
	return Item;
}
