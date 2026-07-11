#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BBItemTypes.h"
#include "BBChestLootTable.generated.h"

class ABBWorldItem;

UENUM(BlueprintType)
enum class EBBChestTier : uint8
{
	Common		UMETA(DisplayName = "Common"),
	Armor		UMETA(DisplayName = "Armor"),
	Rare		UMETA(DisplayName = "Rare"),
	VaultLike	UMETA(DisplayName = "Vault Like"),
	SupplyDrop	UMETA(DisplayName = "Supply Drop")
};

UENUM(BlueprintType)
enum class EBBChestLootType : uint8
{
	Power		UMETA(DisplayName = "Power"),
	Consumable	UMETA(DisplayName = "Consumable"),
	Shards		UMETA(DisplayName = "Shards"),
	Gold		UMETA(DisplayName = "Gold"),
	Equipment	UMETA(DisplayName = "Equipment")
};

USTRUCT(BlueprintType)
struct FBBChestLootEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Loot")
	EBBChestLootType LootType = EBBChestLootType::Consumable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Loot")
	FName ItemID = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Loot")
	EEquipmentSlotType EquipmentSlotType = EEquipmentSlotType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Loot", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Loot", meta = (ClampMin = "1"))
	int32 MinAmount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Loot", meta = (ClampMin = "1"))
	int32 MaxAmount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Loot")
	bool bRequireChestTier = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Loot", meta = (EditCondition = "bRequireChestTier"))
	EBBChestTier RequiredChestTier = EBBChestTier::Common;
};

UCLASS(BlueprintType)
class BREACHBORNE_API UBBChestLootTable : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Loot", meta = (ClampMin = "1"))
	int32 MinRolls = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Loot", meta = (ClampMin = "1"))
	int32 MaxRolls = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Loot")
	bool bAllowDuplicateItemIDs = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Loot")
	TArray<FBBChestLootEntry> Entries;

	int32 SpawnLoot(UWorld* World, const FVector& Origin, EBBChestTier ChestTier) const;

	static int32 SpawnFallbackLoot(UWorld* World, const FVector& Origin, EBBChestTier ChestTier);

private:
	int32 SpawnLootFromEntries(UWorld* World, const FVector& Origin, EBBChestTier ChestTier, const TArray<FBBChestLootEntry>& SourceEntries,
		int32 RollMin, int32 RollMax, bool bAllowDuplicates) const;

	const FBBChestLootEntry* ChooseWeightedEntry(const TArray<FBBChestLootEntry>& SourceEntries, EBBChestTier ChestTier,
		const TSet<FName>& UsedItemIDs, bool bAllowDuplicates) const;

	static TArray<FBBChestLootEntry> BuildFallbackEntries(EBBChestTier ChestTier);
	static ABBWorldItem* SpawnEntry(UWorld* World, const FVector& Location, const FBBChestLootEntry& Entry, int32 Amount);
	static FVector GetScatterLocation(const FVector& Origin, int32 Index, int32 Count);
};
