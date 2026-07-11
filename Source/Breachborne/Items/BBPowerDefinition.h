#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BBItemTypes.h"
#include "BBPowerDefinition.generated.h"

class UGameplayAbility;

UENUM(BlueprintType)
enum class EBBPowerActivationType : uint8
{
	Active	UMETA(DisplayName = "Active"),
	Passive	UMETA(DisplayName = "Passive"),
	Toggle	UMETA(DisplayName = "Toggle")
};

/**
 * Data asset defining a Power pickup.
 * Powers occupy 1 of 2 power slots. They grant an ability (passive or activatable)
 * and optional stat bonuses. Found in vaults, from bosses, and in deathboxes.
 * Legendary (5-star) powers drop on the ground when the carrier dies.
 */
UCLASS(BlueprintType)
class BREACHBORNE_API UBBPowerDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique identifier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FName PowerID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	EItemRarity Rarity = EItemRarity::Common;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	EBBPowerActivationType ActivationType = EBBPowerActivationType::Active;

	/** The GameplayAbility class granted when this power is equipped */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	TSubclassOf<UGameplayAbility> GrantedAbilityClass;

	/** If true, the ability auto-activates on equip (passive). If false, bound to power input. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	bool bIsPassive = false;

	/** Optional explicit input tag. Empty means Power1 -> F and Power2 -> G. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FGameplayTag PowerInputTagOverride;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	float CooldownSeconds = 0.0f;

	/** Passive stat bonuses granted while this power is equipped */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	FBBStatBonuses StatBonuses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	bool bSoulbound = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	bool bDropOnDeath = false;

	/** If true, this power is dropped on the ground when the carrier dies (legendary powers) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	bool bDropsOnDeath = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	bool bAutoActivateOnEquip = false;

	//~ UPrimaryDataAsset
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	void NormalizeLegacyFields();
};
