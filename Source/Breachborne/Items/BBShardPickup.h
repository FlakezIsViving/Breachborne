#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBShardPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class AHunterCharacter;
class APlayerState;

/** Current state of the shard pickup — drives visuals and collection logic */
UENUM(BlueprintType)
enum class EShardPickupState : uint8
{
	Idle,
	Magnetized,
	PickedUp
};

/**
 * A red sphere pickup for upgrade shards. Auto-collects on walk-over
 * and magnets toward nearby hunters after a short delay.
 * Scales in size based on shard amount (small for PvE, larger for PvP).
 * Server authoritative — state and collection replicated to all clients.
 */
UCLASS()
class BREACHBORNE_API ABBShardPickup : public AActor
{
	GENERATED_BODY()

public:
	ABBShardPickup();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;

	/** Server: set shard amount, scale mesh, start timers. Source = the player who dropped these (null for NPC). */
	void InitShards(int32 Amount, APlayerState* Source = nullptr);

	/** Server RPC: E-key pickup request — works for any player including the source */
	UFUNCTION(Server, Reliable)
	void ServerRequestPickup(APlayerController* RequestingPlayer);

	/** Returns true if Checker is the player who dropped these shards */
	bool IsOwnShard(const APlayerState* Checker) const;

protected:
	virtual void BeginPlay() override;

	// --- Components ---

	/** Large sphere for magnet attraction detection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Items")
	TObjectPtr<USphereComponent> MagnetSphere;

	/** Small sphere for direct walk-over collection */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Items")
	TObjectPtr<USphereComponent> CollectionSphere;

	/** The visible red sphere mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Items")
	TObjectPtr<UStaticMeshComponent> ShardMesh;

	// --- Replicated State ---

	/** The player who dropped these shards (null for NPC drops). Own shards require E-key to pick up. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Items")
	TObjectPtr<APlayerState> SourcePlayerState;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Items")
	int32 ShardAmount = 1;

	UPROPERTY(ReplicatedUsing = OnRep_PickupState, BlueprintReadOnly, Category = "Breachborne|Items")
	EShardPickupState PickupState = EShardPickupState::Idle;

	UFUNCTION()
	void OnRep_PickupState();

	// --- Tuning ---

	/** Radius for magnet attraction trigger */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Items|Shard")
	float MagnetRadius = 300.0f;

	/** Radius for direct walk-over collection */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Items|Shard")
	float CollectionRadius = 50.0f;

	/** Max speed while being magnetized toward a hunter (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Items|Shard")
	float MagnetSpeed = 1200.0f;

	/** Time before the shard despawns if uncollected (seconds) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Items|Shard")
	float DespawnTime = 60.0f;

	/** Delay before magnet activation after spawn (seconds) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Items|Shard")
	float MagnetActivationDelay = 0.5f;

	/** Base mesh scale for 1 shard */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Items|Shard")
	float BaseVisualScale = 0.15f;

	/** Additional scale per extra shard */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Items|Shard")
	float ScalePerShard = 0.05f;

private:
	/** Overlap callback for magnet sphere */
	UFUNCTION()
	void OnMagnetOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Overlap callback for collection sphere */
	UFUNCTION()
	void OnCollectionOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Server: validate and collect the shard into the hunter's inventory */
	void ServerCollectShard(AHunterCharacter* Hunter);

	/** Timer callback: enable magnet attraction */
	void ActivateMagnet();

	/** Timer callback: despawn the shard */
	void OnDespawnTimer();

	/** Check for any hunters already overlapping the magnet sphere when magnet activates */
	void CheckExistingMagnetOverlaps();

	/** The hunter being magnetized toward (server only) */
	UPROPERTY()
	TObjectPtr<AHunterCharacter> MagnetTarget;

	/** Whether the magnet is active (set after MagnetActivationDelay) */
	bool bMagnetActive = false;

	/** Dynamic material instance for the red color */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RedMaterial;

	FTimerHandle MagnetActivationTimerHandle;
	FTimerHandle DespawnTimerHandle;
};
