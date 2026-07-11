#pragma once

#include "CoreMinimal.h"
#include "BBItemTypes.generated.h"

// ============================================================================
// Enums
// ============================================================================

/** Which equipment slot an item occupies */
UENUM(BlueprintType)
enum class EEquipmentSlotType : uint8
{
	None		UMETA(DisplayName = "None"),
	Weapon		UMETA(DisplayName = "Weapon"),
	Helmet		UMETA(DisplayName = "Helmet"),
	Boots		UMETA(DisplayName = "Boots")
};

/** Armor tier — determines damage reduction. Found as world drops, repaired with shards. */
UENUM(BlueprintType)
enum class EArmorTier : uint8
{
	None		UMETA(DisplayName = "None"),
	White		UMETA(DisplayName = "White"),
	Green		UMETA(DisplayName = "Green"),
	Blue		UMETA(DisplayName = "Blue"),
	Purple		UMETA(DisplayName = "Purple")
};

/** Evolution path chosen when upgrading equipment (permanent for the match) */
UENUM(BlueprintType)
enum class EEvolutionPath : uint8
{
	None	UMETA(DisplayName = "None"),
	PathA	UMETA(DisplayName = "Path A"),
	PathB	UMETA(DisplayName = "Path B"),
	PathC	UMETA(DisplayName = "Path C")
};

/** Rarity for items — affects stat magnitudes */
UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common		UMETA(DisplayName = "Common"),
	Uncommon	UMETA(DisplayName = "Uncommon"),
	Rare		UMETA(DisplayName = "Rare"),
	Epic		UMETA(DisplayName = "Epic"),
	Legendary	UMETA(DisplayName = "Legendary")
};

/** Power slot index — each hunter has 2 power slots */
UENUM(BlueprintType)
enum class EPowerSlotIndex : uint8
{
	Power1 = 0	UMETA(DisplayName = "Power 1"),
	Power2 = 1	UMETA(DisplayName = "Power 2")
};

/** Consumable type — determines use behavior */
UENUM(BlueprintType)
enum class EConsumableType : uint8
{
	None			UMETA(DisplayName = "None"),
	Food			UMETA(DisplayName = "Food"),
	ArmorShard		UMETA(DisplayName = "Armor Shard"),
	ViveBrew		UMETA(DisplayName = "Vive Brew"),
	UtilityBuff		UMETA(DisplayName = "Utility Buff")
};

// ============================================================================
// Structs — Stat Bonuses
// ============================================================================

/** Flat stat bonuses applied by equipment/powers via additive GE modifiers */
USTRUCT(BlueprintType)
struct FBBStatBonuses
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	float AttackPower = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	float AbilityPower = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	float MaxHealth = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	float MaxShield = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	float MoveSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	float CritChance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	float CooldownReduction = 0.0f;

	bool IsZero() const
	{
		return FMath::IsNearlyZero(AttackPower) && FMath::IsNearlyZero(AbilityPower)
			&& FMath::IsNearlyZero(MaxHealth) && FMath::IsNearlyZero(MaxShield)
			&& FMath::IsNearlyZero(MoveSpeed) && FMath::IsNearlyZero(CritChance)
			&& FMath::IsNearlyZero(CooldownReduction);
	}
};

// ============================================================================
// Structs — Evolution
// ============================================================================

/** One of the 3 evolution paths for an equipment item */
USTRUCT(BlueprintType)
struct FEquipmentEvolution
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FBBStatBonuses BonusStats;
};

// ============================================================================
// Structs — Inventory Slot State (for replication)
// ============================================================================

/** State of a single equipment slot (weapon, helmet, or boots) */
USTRUCT(BlueprintType)
struct FEquipmentSlotState
{
	GENERATED_BODY()

	/** Data asset ID (FPrimaryAssetId serialized as FName). NAME_None = empty slot. */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Items")
	FName ItemID;

	/** Current upgrade tier (0 = base, 1+ = upgraded) */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Items")
	int32 UpgradeTier = 0;

	/** Chosen evolution path (None until evolved) */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Items")
	EEvolutionPath Evolution = EEvolutionPath::None;

	bool IsEmpty() const { return ItemID.IsNone(); }

	void Clear()
	{
		ItemID = NAME_None;
		UpgradeTier = 0;
		Evolution = EEvolutionPath::None;
	}
};

/** State of the armor slot */
USTRUCT(BlueprintType)
struct FArmorState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Items")
	EArmorTier Tier = EArmorTier::None;

	/** Current durability (0 to MaxHP based on tier) */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Items")
	float CurrentHP = 0.0f;

	bool HasArmor() const { return Tier != EArmorTier::None; }
};

/** State of a power slot */
USTRUCT(BlueprintType)
struct FPowerSlotState
{
	GENERATED_BODY()

	/** Data asset ID. NAME_None = empty slot. */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Items")
	FName PowerID;

	bool IsEmpty() const { return PowerID.IsNone(); }

	void Clear() { PowerID = NAME_None; }
};

/** A stack of consumables in a backpack slot */
USTRUCT(BlueprintType)
struct FConsumableStack
{
	GENERATED_BODY()

	/** Data asset ID. NAME_None = empty slot. */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Items")
	FName ConsumableID;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Items")
	int32 Count = 0;

	bool IsEmpty() const { return ConsumableID.IsNone() || Count <= 0; }

	void Clear()
	{
		ConsumableID = NAME_None;
		Count = 0;
	}
};

// ============================================================================
// Structs — Shopkeeper
// ============================================================================

/** A single listing in a shopkeeper's inventory */
USTRUCT(BlueprintType)
struct FShopListing
{
	GENERATED_BODY()

	/** Item data asset ID */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FName ItemID;

	/** Equipment slot type (determines which slot it goes into) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	EEquipmentSlotType SlotType = EEquipmentSlotType::None;

	/** Gold cost to purchase */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	int32 GoldCost = 100;
};
