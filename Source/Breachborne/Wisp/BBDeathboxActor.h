#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Breachborne/Items/BBInventoryTypes.h"
#include "BBDeathboxActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class ABreachbornePlayerState;
class AHunterCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeathboxReviveComplete, ABBDeathboxActor*, Deathbox, AHunterCharacter*, Reviver);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathboxReviveCancelled, ABBDeathboxActor*, Deathbox);

/**
 * Spawned when a wisp's HP hits 0 (full death) or instant abyss death.
 * Contains the victim's inventory snapshot for looting.
 * Allies can channel near it for ReviveChannelDuration to revive the dead player.
 *
 * Abyss death:  instant deathbox, no wisp state. Victim returns at deathbox location.
 * Combat death: wisp → wisp HP drains → deathbox spawns at wisp death location.
 *
 * Server-authoritative. Revive interaction initiated via ABreachbornePlayerController RPC.
 */
UCLASS()
class BREACHBORNE_API ABBDeathboxActor : public AActor
{
	GENERATED_BODY()

public:
	ABBDeathboxActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Initialize the deathbox on server. Call immediately after spawn.
	 * @param InVictimPS The dead player's PlayerState
	 * @param InInventory A snapshot of the victim's inventory at time of death
	 */
	void InitDeathbox(ABreachbornePlayerState* InVictimPS, const FRepInventoryData& InInventory);

	/**
	 * Server: begin the revive channel for a nearby allied hunter.
	 * Validates proximity and team. Starts the 5s channel timer.
	 * Cancels any existing channel first.
	 */
	void ServerBeginReviveChannel(AHunterCharacter* Reviver);

	/** Server: cancel the current revive channel (called when reviver takes damage or moves away) */
	void ServerCancelReviveChannel();

	// --- Getters ---

	UFUNCTION(BlueprintPure, Category = "Breachborne|Deathbox")
	ABreachbornePlayerState* GetVictimPlayerState() const { return VictimPlayerState; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Deathbox")
	const FRepInventoryData& GetInventorySnapshot() const { return InventorySnapshot; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Deathbox")
	float GetReviveProgress() const { return ReviveProgress; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Deathbox")
	bool IsBeingRevived() const { return bIsBeingRevived; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Deathbox")
	const FString& GetVictimName() const { return VictimName; }

	/** How far a reviver must be to start/maintain the channel */
	float GetInteractRadius() const { return InteractRadius; }

	// --- Delegates (server only, bound by GameMode) ---

	/** Fired when the revive channel completes — GameMode should revive the victim */
	FOnDeathboxReviveComplete OnReviveComplete;

	/** Fired when the revive channel is cancelled (reviver took damage, moved away, etc.) */
	FOnDeathboxReviveCancelled OnReviveCancelled;

protected:
	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Deathbox")
	TObjectPtr<USphereComponent> InteractSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Deathbox")
	TObjectPtr<UStaticMeshComponent> DeathboxMesh;

	// --- Tuning ---

	/** Distance (cm) within which an ally can start a revive channel */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Deathbox")
	float InteractRadius = 300.0f;

	/** Seconds required to channel to complete the revive (no interruption) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Deathbox")
	float ReviveChannelDuration = 5.0f;

	/** HP fraction the revived player receives (0.0-1.0) — readable by GameMode */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Deathbox")
	float ReviveHealthFraction = 0.35f;

public:
	float GetReviveHealthFraction() const { return ReviveHealthFraction; }

	/** How long the deathbox persists in the world before auto-destroying (seconds) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Deathbox")
	float DeathboxLifetime = 120.0f;

private:
	/** Server-only: pointer to the victim's PlayerState */
	UPROPERTY()
	TObjectPtr<ABreachbornePlayerState> VictimPlayerState;

	/** Snapshot of the victim's inventory at time of death (replicated for loot UI) */
	UPROPERTY(Replicated)
	FRepInventoryData InventorySnapshot;

	/** Victim's display name — replicated for UI */
	UPROPERTY(Replicated)
	FString VictimName;

	/** Revive channel progress [0,1] — replicated for progress bar UI */
	UPROPERTY(ReplicatedUsing = OnRep_ReviveProgress)
	float ReviveProgress = 0.0f;

	UFUNCTION()
	void OnRep_ReviveProgress();

	/** Whether a revive channel is currently active */
	UPROPERTY(Replicated)
	bool bIsBeingRevived = false;

	/** Current reviver (server only) */
	UPROPERTY()
	TObjectPtr<AHunterCharacter> CurrentReviver;

	/** Elapsed channel time (server only) */
	float ChannelElapsed = 0.0f;

	/** Timer that ticks the revive channel progress */
	FTimerHandle ReviveTickHandle;

	/** Timer that auto-destroys the deathbox */
	FTimerHandle LifetimeHandle;

	/** Delegate handle watching the reviver's health for damage interrupts */
	FDelegateHandle ReviverDamageHandle;

	/** Tick the channel at 10Hz — update ReviveProgress, fire OnReviveComplete if done */
	void ReviveChannelTick();

	/** Called when the currently-reviving hunter takes damage — cancels the channel */
	void OnReviverTakeDamage(const struct FOnAttributeChangeData& Data);
};
