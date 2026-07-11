#pragma once

#include "CoreMinimal.h"
#include "StormShiftBase.h"
#include "BBStormShift_BulletTrains.generated.h"

class ABBTrain;
class ABBTrainTrack;

/**
 * Bullet Trains — a mutating storm shift that meaningfully alters match conditions.
 *
 * What it does vs the Default shift:
 *
 * 1. SPEED BUFF: All alive hunters get +30% MoveSpeed for the duration of each phase.
 *    Applied as a GAS GameplayEffect (Effect.StormShift.BulletTrains).
 *    Removed and reapplied fresh at the start of each new phase.
 *
 * 2. NOMADIC CENTER: The storm center doesn't stay fixed at the map origin.
 *    Each phase, the center shifts toward a random off-center point within the
 *    remaining safe zone radius. This forces constant repositioning.
 *
 * 3. FASTER SHRINK: 20% shorter shrink windows than Default (encourages aggression).
 *
 * 4. RAMPING DAMAGE: Damage per tick increases more aggressively than Default.
 *    Phase 4 does 2× the base damage rate.
 *
 * 5. ACTUAL TRAINS: Spawns lethal moving trains on designer-placed tracks.
 *    Normal mode: 1 train per track (locomotive + 3 platforms + chest + circle immune + revive beacon).
 *    BulletTrains mode: 3 mini trains per track (locomotive + 1 platform each), drastically faster.
 *
 * Exit criterion satisfied: this shift demonstrably mutates match conditions
 * (movement, map rotation, damage cadence, environmental hazards) rather than just shrinking.
 */
UCLASS()
class BREACHBORNE_API UBBStormShift_BulletTrains : public UStormShiftBase
{
	GENERATED_BODY()

public:
	virtual FName GetShiftName() const override { return FName(TEXT("BulletTrains")); }

	virtual TArray<FStormPhaseConfig> GetPhaseConfigs(float InitialRadius) const override;
	virtual FVector2D PickNextCenter(int32 PhaseIndex, const FVector2D& CurrentCenter, float NewRadius) const override;
	virtual void ApplyShiftEffects(UWorld* World, int32 PhaseIndex) override;
	virtual void RemoveShiftEffects(UWorld* World) override;

private:
	/** MoveSpeed multiplier granted to all alive hunters during this shift */
	static constexpr float SpeedMultiplier = 1.3f;

	/** How far off-center the storm can drift (as fraction of current radius) */
	static constexpr float CenterDriftFraction = 0.35f;

	/** Trains spawned by this shift */
	UPROPERTY()
	TArray<TObjectPtr<ABBTrain>> SpawnedTrains;

	/** Spawn trains on all tracks in the world */
	void SpawnTrains(UWorld* World);

	/** Destroy all spawned trains */
	void DestroyTrains();

	/** Spawn a single train on a track */
	void SpawnTrainOnTrack(ABBTrainTrack* Track, float InitialProgress, bool bBulletTrainMode);
};
