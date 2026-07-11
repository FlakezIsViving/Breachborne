#include "BBChestLootTable.h"

#include "BBWorldItem.h"
#include "BBWorldItemSpawner.h"
#include "Breachborne/Breachborne.h"

int32 UBBChestLootTable::SpawnLoot(UWorld* World, const FVector& Origin, EBBChestTier ChestTier) const
{
	return SpawnLootFromEntries(World, Origin, ChestTier, Entries, MinRolls, MaxRolls, bAllowDuplicateItemIDs);
}

int32 UBBChestLootTable::SpawnFallbackLoot(UWorld* World, const FVector& Origin, EBBChestTier ChestTier)
{
	const TArray<FBBChestLootEntry> FallbackEntries = BuildFallbackEntries(ChestTier);
	const UBBChestLootTable* DefaultTable = GetDefault<UBBChestLootTable>();
	const int32 RollMin = ChestTier == EBBChestTier::SupplyDrop ? 4 : (ChestTier == EBBChestTier::VaultLike ? 3 : 2);
	const int32 RollMax = ChestTier == EBBChestTier::SupplyDrop ? 5 : (ChestTier == EBBChestTier::VaultLike ? 4 : 3);
	return DefaultTable->SpawnLootFromEntries(World, Origin, ChestTier, FallbackEntries, RollMin, RollMax, true);
}

int32 UBBChestLootTable::SpawnLootFromEntries(UWorld* World, const FVector& Origin, EBBChestTier ChestTier,
	const TArray<FBBChestLootEntry>& SourceEntries, int32 RollMin, int32 RollMax, bool bAllowDuplicates) const
{
	if (!World || SourceEntries.IsEmpty())
	{
		return 0;
	}

	const int32 RollCount = FMath::RandRange(FMath::Max(1, RollMin), FMath::Max(RollMin, RollMax));
	int32 SpawnedCount = 0;
	TSet<FName> UsedItemIDs;

	for (int32 RollIndex = 0; RollIndex < RollCount; ++RollIndex)
	{
		const FBBChestLootEntry* Entry = ChooseWeightedEntry(SourceEntries, ChestTier, UsedItemIDs, bAllowDuplicates);
		if (!Entry)
		{
			break;
		}

		const int32 Amount = FMath::RandRange(FMath::Max(1, Entry->MinAmount), FMath::Max(Entry->MinAmount, Entry->MaxAmount));
		const FVector SpawnLocation = GetScatterLocation(Origin, SpawnedCount, RollCount);
		if (SpawnEntry(World, SpawnLocation, *Entry, Amount))
		{
			UsedItemIDs.Add(Entry->ItemID);
			SpawnedCount++;
		}
	}

	return SpawnedCount;
}

const FBBChestLootEntry* UBBChestLootTable::ChooseWeightedEntry(const TArray<FBBChestLootEntry>& SourceEntries, EBBChestTier ChestTier,
	const TSet<FName>& UsedItemIDs, bool bAllowDuplicates) const
{
	float TotalWeight = 0.0f;
	for (const FBBChestLootEntry& Entry : SourceEntries)
	{
		if (Entry.Weight <= 0.0f)
		{
			continue;
		}
		if (Entry.bRequireChestTier && Entry.RequiredChestTier != ChestTier)
		{
			continue;
		}
		if (!bAllowDuplicates && !Entry.ItemID.IsNone() && UsedItemIDs.Contains(Entry.ItemID))
		{
			continue;
		}
		TotalWeight += Entry.Weight;
	}

	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	for (const FBBChestLootEntry& Entry : SourceEntries)
	{
		if (Entry.Weight <= 0.0f)
		{
			continue;
		}
		if (Entry.bRequireChestTier && Entry.RequiredChestTier != ChestTier)
		{
			continue;
		}
		if (!bAllowDuplicates && !Entry.ItemID.IsNone() && UsedItemIDs.Contains(Entry.ItemID))
		{
			continue;
		}

		Roll -= Entry.Weight;
		if (Roll <= 0.0f)
		{
			return &Entry;
		}
	}

	return nullptr;
}

TArray<FBBChestLootEntry> UBBChestLootTable::BuildFallbackEntries(EBBChestTier ChestTier)
{
	auto MakeEntry = [](EBBChestLootType Type, FName ItemID, EEquipmentSlotType Slot, float Weight, int32 MinAmount, int32 MaxAmount)
	{
		FBBChestLootEntry Entry;
		Entry.LootType = Type;
		Entry.ItemID = ItemID;
		Entry.EquipmentSlotType = Slot;
		Entry.Weight = Weight;
		Entry.MinAmount = MinAmount;
		Entry.MaxAmount = MaxAmount;
		return Entry;
	};

	TArray<FBBChestLootEntry> Entries;
	Entries.Add(MakeEntry(EBBChestLootType::Shards, NAME_None, EEquipmentSlotType::None, 35.0f, 2, 5));
	Entries.Add(MakeEntry(EBBChestLootType::Gold, NAME_None, EEquipmentSlotType::None, 20.0f, 25, 80));
	Entries.Add(MakeEntry(EBBChestLootType::Consumable, FName(TEXT("ViveBean")), EEquipmentSlotType::None, 20.0f, 1, 2));
	Entries.Add(MakeEntry(EBBChestLootType::Consumable, FName(TEXT("ViveBrew")), EEquipmentSlotType::None, 8.0f, 1, 1));
	Entries.Add(MakeEntry(EBBChestLootType::Consumable, FName(TEXT("ArmorShard_Small")), EEquipmentSlotType::None, 12.0f, 1, 2));
	Entries.Add(MakeEntry(EBBChestLootType::Equipment, FName(TEXT("ChestWeapon")), EEquipmentSlotType::Weapon, 6.0f, 1, 1));
	Entries.Add(MakeEntry(EBBChestLootType::Equipment, FName(TEXT("ChestHelmet")), EEquipmentSlotType::Helmet, 5.0f, 1, 1));
	Entries.Add(MakeEntry(EBBChestLootType::Equipment, FName(TEXT("ChestBoots")), EEquipmentSlotType::Boots, 4.0f, 1, 1));
	Entries.Add(MakeEntry(EBBChestLootType::Power, FName(TEXT("EmergencyPlatform")), EEquipmentSlotType::None, 3.0f, 1, 1));

	if (ChestTier == EBBChestTier::Armor)
	{
		Entries.Add(MakeEntry(EBBChestLootType::Shards, NAME_None, EEquipmentSlotType::None, 45.0f, 4, 8));
		Entries.Add(MakeEntry(EBBChestLootType::Consumable, FName(TEXT("ArmorShard_Large")), EEquipmentSlotType::None, 35.0f, 1, 2));
	}
	else if (ChestTier == EBBChestTier::Rare || ChestTier == EBBChestTier::VaultLike || ChestTier == EBBChestTier::SupplyDrop)
	{
		Entries.Add(MakeEntry(EBBChestLootType::Power, FName(TEXT("GrapplingHook")), EEquipmentSlotType::None, 8.0f, 1, 1));
		Entries.Add(MakeEntry(EBBChestLootType::Equipment, FName(TEXT("RareChestWeapon")), EEquipmentSlotType::Weapon, 10.0f, 1, 1));
	}

	if (ChestTier == EBBChestTier::SupplyDrop)
	{
		Entries.Add(MakeEntry(EBBChestLootType::Power, FName(TEXT("TacticalNuke")), EEquipmentSlotType::None, 5.0f, 1, 1));
		Entries.Add(MakeEntry(EBBChestLootType::Shards, NAME_None, EEquipmentSlotType::None, 35.0f, 8, 14));
		Entries.Add(MakeEntry(EBBChestLootType::Gold, NAME_None, EEquipmentSlotType::None, 20.0f, 100, 220));
	}

	return Entries;
}

ABBWorldItem* UBBChestLootTable::SpawnEntry(UWorld* World, const FVector& Location, const FBBChestLootEntry& Entry, int32 Amount)
{
	switch (Entry.LootType)
	{
	case EBBChestLootType::Power:
		return Entry.ItemID.IsNone() ? nullptr : FBBWorldItemSpawner::SpawnPowerPickup(World, Location, Entry.ItemID);
	case EBBChestLootType::Consumable:
		return Entry.ItemID.IsNone() ? nullptr : FBBWorldItemSpawner::SpawnConsumablePickup(World, Location, Entry.ItemID, Amount);
	case EBBChestLootType::Shards:
		return FBBWorldItemSpawner::SpawnShardPickup(World, Location, Amount);
	case EBBChestLootType::Gold:
		return FBBWorldItemSpawner::SpawnGoldPickup(World, Location, Amount);
	case EBBChestLootType::Equipment:
		return Entry.ItemID.IsNone() || Entry.EquipmentSlotType == EEquipmentSlotType::None
			? nullptr
			: FBBWorldItemSpawner::SpawnEquipmentPickup(World, Location, Entry.ItemID, Entry.EquipmentSlotType);
	default:
		return nullptr;
	}
}

FVector UBBChestLootTable::GetScatterLocation(const FVector& Origin, int32 Index, int32 Count)
{
	const float AngleStep = Count > 0 ? (2.0f * PI) / Count : 2.0f * PI;
	const float Angle = AngleStep * Index + 0.35f;
	const float Radius = 150.0f + (Index % 2) * 45.0f;
	return Origin + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 55.0f);
}
