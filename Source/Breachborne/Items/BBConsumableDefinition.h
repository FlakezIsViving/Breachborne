#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BBItemTypes.h"
#include "BBConsumableDefinition.generated.h"

class UGameplayEffect;

/**
 * Data asset defining a consumable item (food, armor shards, vive brews, utility buffs).
 * Consumables stack in backpack slots and are used via hotkey.
 */
UCLASS(BlueprintType)
class BREACHBORNE_API UBBConsumableDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique identifier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FName ConsumableID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	EConsumableType ConsumableType = EConsumableType::None;

	/** Maximum number that can stack in a single backpack slot */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	int32 MaxStackSize = 3;

	/** Instant heal amount (for Food). 0 = no instant heal. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items", meta = (EditCondition = "ConsumableType == EConsumableType::Food"))
	float InstantHealAmount = 0.0f;

	/** Armor bars to repair (for ArmorShard). 0 = no repair. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items", meta = (EditCondition = "ConsumableType == EConsumableType::ArmorShard"))
	float ArmorRepairAmount = 50.0f;

	/** Optional GameplayEffect class to apply on use (for buffs, HoTs, etc.) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	TSubclassOf<UGameplayEffect> OnUseEffectClass;

	/** Gold cost to purchase from a shopkeeper (0 = not sold at shops) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	int32 GoldCost = 0;

	//~ UPrimaryDataAsset
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
