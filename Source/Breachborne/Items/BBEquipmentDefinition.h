#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BBItemTypes.h"
#include "BBEquipmentDefinition.generated.h"

/**
 * Data asset defining an equipment item (weapon, helmet, or boots).
 * Contains base stats, shard costs per upgrade tier, and 3 evolution paths.
 * Create instances in the editor Content Browser → Miscellaneous → Data Asset → BBEquipmentDefinition.
 */
UCLASS(BlueprintType)
class BREACHBORNE_API UBBEquipmentDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique identifier used in inventory state and lookups */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FName ItemID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FText Description;

	/** Which equipment slot this item occupies */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	EEquipmentSlotType SlotType = EEquipmentSlotType::Weapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	EItemRarity Rarity = EItemRarity::Common;

	/** Base stat bonuses at tier 0 (before any upgrades) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FBBStatBonuses BaseStats;

	/** Shards required to upgrade from tier N to tier N+1. Index 0 = cost for tier 0→1. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	TArray<int32> ShardCostPerTier;

	/** The 3 evolution paths. Choose one when upgrading past the evolution threshold. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FEquipmentEvolution EvolutionA;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FEquipmentEvolution EvolutionB;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FEquipmentEvolution EvolutionC;

	/** Tier at which evolution must be chosen (0-indexed). Typically tier 2. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	int32 EvolutionTier = 2;

	/** Get the evolution data for a specific path */
	const FEquipmentEvolution& GetEvolution(EEvolutionPath Path) const;

	/** Get shard cost to upgrade from CurrentTier to CurrentTier+1. Returns -1 if max tier. */
	int32 GetUpgradeCost(int32 CurrentTier) const;

	/** Get the total stat bonuses for a given tier + evolution */
	FBBStatBonuses GetTotalStats(int32 Tier, EEvolutionPath Evolution) const;

	//~ UPrimaryDataAsset
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
