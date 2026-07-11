#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BBItemTypes.h"
#include "BBItemRegistry.generated.h"

class UBBEquipmentDefinition;
class UBBPowerDefinition;
class UBBConsumableDefinition;

/**
 * Central registry for all item definitions. Lives as a GameInstanceSubsystem
 * so it persists across level loads. Scans for data assets on initialization.
 * Provides lookup by ID and random drop generation.
 */
UCLASS()
class BREACHBORNE_API UBBItemRegistry : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Find an equipment definition by ItemID */
	const UBBEquipmentDefinition* FindEquipment(FName ItemID) const;

	/** Find a power definition by PowerID */
	const UBBPowerDefinition* FindPower(FName PowerID) const;

	/** Find a consumable definition by ConsumableID */
	const UBBConsumableDefinition* FindConsumable(FName ConsumableID) const;

	/** Get a random equipment drop (for PvE loot tables) */
	const UBBEquipmentDefinition* GetRandomEquipmentDrop() const;

	/** Get a random power drop */
	const UBBPowerDefinition* GetRandomPowerDrop() const;

	/** Register an equipment definition manually (for testing) */
	void RegisterEquipment(UBBEquipmentDefinition* Def);

	/** Register a power definition manually (for testing) */
	void RegisterPower(UBBPowerDefinition* Def);

	/** Register a consumable definition manually (for testing) */
	void RegisterConsumable(UBBConsumableDefinition* Def);

private:
	UPROPERTY()
	TMap<FName, TObjectPtr<UBBEquipmentDefinition>> EquipmentMap;

	UPROPERTY()
	TMap<FName, TObjectPtr<UBBPowerDefinition>> PowerMap;

	UPROPERTY()
	TMap<FName, TObjectPtr<UBBConsumableDefinition>> ConsumableMap;

	/** Scan the asset manager for all registered item data assets */
	void ScanForDataAssets();

	void RegisterBuiltInPowerDefinitions();
};
