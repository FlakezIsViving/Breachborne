#pragma once

#include "CoreMinimal.h"
#include "BBCreepCharacter.h"
#include "BBBossCharacter.generated.h"

class ABBWorldItem;

/**
 * Boss character — a large PvE enemy.
 *
 * Extends ABBCreepCharacter with:
 * - Phase 2 enrage at 50% HP: attack speed doubles
 * - On death: drops contested loot (a WorldItem) at death location
 * - Boss camp registers a "contestable" flag so all teams can see it on minimap
 *
 * Configure MaxHealth / rewards / ItemDropClass in the BP subclass.
 */
UCLASS()
class BREACHBORNE_API ABBBossCharacter : public ABBCreepCharacter
{
	GENERATED_BODY()

public:
	ABBBossCharacter();

protected:
	virtual void BeginPlay() override;

	/** Equipment item class to drop when the boss dies */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Boss")
	TSubclassOf<ABBWorldItem> ItemDropClass;

	/** Item ID for the drop — set on the spawned WorldItem */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Boss")
	FName DropItemID = FName(TEXT("BossDrop_Power"));

	/** HP fraction at which the boss enrages */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Boss")
	float EnrageThreshold = 0.5f;

	/** Multiplier applied to AttackDamage on enrage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Boss")
	float EnrageDamageMultiplier = 1.75f;

private:
	bool bEnraged = false;

	FDelegateHandle BossHealthChangedHandle;
	void OnBossHealthChanged(const FOnAttributeChangeData& Data);

	/** Spawn the loot drop at the boss's death location */
	void SpawnLootDrop();
};
