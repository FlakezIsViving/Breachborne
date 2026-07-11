#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBCreepCamp.generated.h"

class ABBCreepCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCampCleared, ABBCreepCamp*, Camp);

/**
 * Creep camp spawner.
 *
 * - Spawns CreepCount creeps at BeginPlay (server only)
 * - Tracks how many are still alive; fires OnCampCleared when count hits 0
 * - Respawns after RespawnDelay seconds
 * - All creeps are parented to this camp for leash / home logic
 */
UCLASS()
class BREACHBORNE_API ABBCreepCamp : public AActor
{
	GENERATED_BODY()

public:
	ABBCreepCamp();

	/** Fired on server when all creeps in this camp are dead */
	UPROPERTY(BlueprintAssignable, Category = "Breachborne|PvE")
	FOnCampCleared OnCampCleared;

	/** Called by each creep when it dies */
	void OnCreepDied(ABBCreepCharacter* Creep);

protected:
	virtual void BeginPlay() override;

	/** Creep class to spawn — set in Blueprint subclass */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	TSubclassOf<ABBCreepCharacter> CreepClass;

	/** How many creeps this camp spawns per wave */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	int32 CreepCount = 3;

	/** Radius around the camp origin within which creeps spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	float SpawnRadius = 300.0f;

	/** Seconds after camp is cleared before it respawns */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	float RespawnDelay = 90.0f;

	/** If false, the camp does not respawn after being cleared */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	bool bRespawns = true;

private:
	/** Spawn all creeps for this camp */
	void SpawnCreeps();

	/** Called when the respawn timer fires */
	void OnRespawnTimerFired();

	/** Currently alive creeps managed by this camp */
	UPROPERTY()
	TArray<TObjectPtr<ABBCreepCharacter>> LiveCreeps;

	int32 AliveCreepCount = 0;

	FTimerHandle RespawnTimerHandle;
};
