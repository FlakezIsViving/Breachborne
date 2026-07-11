#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Breachborne/Items/BBChestLootTable.h"
#include "BBTrainChestComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChestUnlocked);

/**
 * Train Box Puzzle component.
 *
 * Attached to the Chest carriage of a train.
 *
 * Puzzle rules (from Supervive):
 * - The front of the locomotive shows 4 black/white squares (the target pattern)
 * - Each train segment has 1 square
 * - Players must match the segment squares to the front pattern order
 * - When solved, the chest unlocks and spawns loot
 *
 * For Breachborne: simplified to a server-validated pattern match.
 * The pattern is randomized at train spawn and resets at day start.
 */
UCLASS(ClassGroup = "Breachborne", meta = (BlueprintSpawnableComponent))
class BREACHBORNE_API UBBTrainChestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBBTrainChestComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Generate a new random pattern. Called when the train spawns or at day reset. */
	void GeneratePattern();

	/** Server: attempt to unlock with a player-provided pattern */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Train")
	bool TryUnlock(const TArray<bool>& PlayerAttempt);

	/** Get the front pattern (4 booleans) — replicated to all clients for UI display */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	const TArray<bool>& GetFrontPattern() const { return FrontPattern; }

	/** Get the segment patterns (1 per segment) — for UI display on each carriage */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	const TArray<bool>& GetSegmentPatterns() const { return SegmentPatterns; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	bool IsLocked() const { return bIsLocked; }

	/** Called when the chest is successfully unlocked */
	FOnChestUnlocked OnUnlocked;

protected:
	/** The target pattern shown on the front of the locomotive (4 squares) */
	UPROPERTY(ReplicatedUsing = OnRep_FrontPattern)
	TArray<bool> FrontPattern;

	/** The pattern on each segment (1 square each) */
	UPROPERTY(ReplicatedUsing = OnRep_SegmentPatterns)
	TArray<bool> SegmentPatterns;

	/** Whether the chest is currently locked */
	UPROPERTY(ReplicatedUsing = OnRep_Locked)
	bool bIsLocked = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Train|Loot")
	EBBChestTier ChestTier = EBBChestTier::Rare;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Train|Loot")
	TObjectPtr<UBBChestLootTable> LootTable;

	UFUNCTION()
	void OnRep_FrontPattern();

	UFUNCTION()
	void OnRep_SegmentPatterns();

	UFUNCTION()
	void OnRep_Locked();

	/** Spawn loot when unlocked. Server only. */
	void SpawnLoot();
};
