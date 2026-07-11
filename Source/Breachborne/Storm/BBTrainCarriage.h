#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBTrainCarriage.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class USphereComponent;
class UBBTrainChestComponent;
class AHunterCharacter;
class ACharacter;

UENUM(BlueprintType)
enum class ETrainCarriageType : uint8
{
	Locomotive     UMETA(DisplayName = "Locomotive"),
	Platform       UMETA(DisplayName = "Platform"),
	Chest          UMETA(DisplayName = "Chest"),
	CircleImmune   UMETA(DisplayName = "Circle Immune"),
	ReviveBeacon   UMETA(DisplayName = "Revive Beacon")
};

/**
 * A single carriage (car) of a train.
 *
 * Locomotive: front kill zone on overlap, wall collision on sides
 * Platform: walkable surface, players can stand/ride
 * Chest: locked puzzle box at front of train
 * CircleImmune: grants storm immunity while overlapping
 * ReviveBeacon: squad revive when activated
 */
UCLASS()
class BREACHBORNE_API ABBTrainCarriage : public AActor
{
	GENERATED_BODY()

public:
	ABBTrainCarriage();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Initialize this carriage with a type and offset from the train's progress */
	void InitializeCarriage(ETrainCarriageType InType, float InOffsetFromTrainProgress);

	/** Get the world-length of a carriage type based on its mesh scale */
	static float GetCarriageLength(ETrainCarriageType Type);

	/** Enable revive beacon functionality on this carriage (e.g. for last platform) */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Train")
	void EnableReviveBeacon();

	/** Update world position based on the owning train's progress */
	void UpdatePosition(float TrainProgress, class ABBTrainTrack* Track);

	/** Called by train after movement. Captures rider offsets, detects new riders, pre-positions for next CMC tick */
	void UpdateRiders();

	/** Get platform half-extents (X = half-length along track, Y = half-width across track) */
	FVector2D GetPlatformBounds() const;

	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	ETrainCarriageType GetCarriageType() const { return CarriageType; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	float GetCarriageLength() const { return CarriageLength; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	float GetOffsetFromTrainProgress() const { return OffsetFromTrainProgress; }

	/** Server: try to activate this carriage (chest unlock, beacon revive, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Train")
	bool TryActivate(AHunterCharacter* Activator);

protected:
	/** The static mesh representing this carriage */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<UStaticMeshComponent> CarriageMesh;

public:
	/** Get the carriage mesh component */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	UStaticMeshComponent* GetCarriageMesh() const { return CarriageMesh; }

	/** Main collision — blocks movement, acts as wall */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<UBoxComponent> SideCollision;

	/** Front kill zone — only active on Locomotive. Overlap = instant death */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<UBoxComponent> FrontKillZone;

	/** Locomotive left wall — thin vertical slab (no interior to get stuck in) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<UBoxComponent> LocomotiveLeftWall;

	/** Locomotive right wall — thin vertical slab */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<UBoxComponent> LocomotiveRightWall;

	/** Locomotive back wall — thin vertical slab */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<UBoxComponent> LocomotiveBackWall;

	/** Interaction sphere — for chest, beacon, etc. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<USphereComponent> InteractSphere;



	/** Type of this carriage */
	UPROPERTY(ReplicatedUsing = OnRep_CarriageType, BlueprintReadOnly, Category = "Breachborne|Train")
	ETrainCarriageType CarriageType = ETrainCarriageType::Platform;

	/** If true, this carriage also acts as a revive beacon (used for last platform) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Train")
	bool bHasReviveBeacon = false;

	/** Length of this carriage in world units (replicated so clients size collision correctly) */
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train")
	float CarriageLength = 400.0f;

	/** How far this carriage is from the train's main progress (0-1 along track) */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Train")
	float OffsetFromTrainProgress = 0.0f;

	/** Interaction radius for chest/beacon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train")
	float InteractRadius = 200.0f;

	/** How deep the front kill zone extends forward from the locomotive mesh edge. Kept generous because carriages move by spline teleport. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train")
	float KillZoneDepth = 140.0f;

	/** Front kill zone vertical center above carriage origin. Should cover a full hunter capsule in front of the locomotive. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train")
	float KillZoneCenterZ = 120.0f;

	/** Front kill zone vertical half-height. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train")
	float KillZoneHalfHeight = 260.0f;

	/** How tall the locomotive's SideCollision extends above the mesh (default 2000cm) — prevents jumping/dashing over */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train")
	float LocomotiveWallHeight = 2000.0f;

	/** TEST: Put riders in MOVE_Flying to eliminate CMC FindFloor micro-adjustments */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Train|Debug")
	bool bTestFlyingModeForRiders = false;

	/** For Chest carriages — the puzzle component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<UBBTrainChestComponent> ChestComponent;

	/** Default meshes for procedural visuals */
	UPROPERTY()
	TObjectPtr<UStaticMesh> DefaultCubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> DefaultCylinderMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> DefaultSphereMesh;

private:
	friend class ABBTrain;

	/** RepNotify: CarriageType replicated from server — re-apply collision setup */
	UFUNCTION()
	void OnRep_CarriageType();

	/** Called when a hunter overlaps the front kill zone */
	/** Sweep-based kill detection (runs every Tick on server) — more reliable than overlap events for teleported volumes */
	void CheckFrontKillZoneSweep();

	bool IsCharacterInLocomotiveNoRideZone(const ACharacter* Character) const;
	bool IsCharacterInLocomotiveFrontKillBand(const ACharacter* Character, float Dot) const;
	bool DidLocomotiveRunOverCharacterThisTick(const ACharacter* Character) const;
	void LogTrainKillCandidate(const ACharacter* Character, const TCHAR* Source, float Dot, bool bWouldKill) const;

	/** DEBUG: Log detailed state of all hunters near locomotive to diagnose collision bugs */
	void DebugLogLocomotiveState();

	UFUNCTION()
	void OnFrontKillZoneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Called when a hunter overlaps the circle immune zone */
	UFUNCTION()
	void OnCircleImmuneOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Called when a hunter ends overlap with circle immune zone */
	UFUNCTION()
	void OnCircleImmuneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Safety net: catch unexpected blocking hits on CarriageMesh */
	UFUNCTION()
	void OnCarriageMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Setup collision based on carriage type */
	void SetupCollision();

	/** Apply circle immunity GE to a hunter */
	void ApplyCircleImmunity(AHunterCharacter* Hunter);

	/** Remove circle immunity GE from a hunter */
	void RemoveCircleImmunity(AHunterCharacter* Hunter);

	/** Set of hunters currently receiving circle immunity from this carriage */
	UPROPERTY()
	TArray<TWeakObjectPtr<AHunterCharacter>> ImmuneHunters;

	// ---- Relative-position rider system ----
	/** Map of riders to their local offset from this carriage */
	UPROPERTY()
	TMap<TWeakObjectPtr<ACharacter>, FVector> RiderLocalOffsets;

	/** Carriage transform from the previous frame (before UpdatePosition) */
	FTransform PreviousTransform = FTransform::Identity;

	bool bHasPreviousTransform = false;

	/** Pre-position riders before CMC runs, and reset CMC base to prevent UpdateBasedMovement double-move */
	void PrePositionRiders();

	/** Capture each rider's local offset after CMC has run (uses PreviousTransform) */
	void CaptureRiderOffsets();

	/** Detect characters standing on/near this carriage and add them as riders */
	void DetectNewRiders();

	/** Remove a rider (they walked off, jumped, etc.) */
	void RemoveRider(ACharacter* Character);

	/** Check if a local offset is still on the platform */
	bool IsLocalOffsetOnPlatform(const FVector& LocalOffset, bool bLooseCheck = false) const;

	/** Direct access to rider map for global conflict resolution by ABBTrain */
	TMap<TWeakObjectPtr<ACharacter>, FVector>& GetRiderLocalOffsets() { return RiderLocalOffsets; }
	const TMap<TWeakObjectPtr<ACharacter>, FVector>& GetRiderLocalOffsets() const { return RiderLocalOffsets; }

	/** Clear all riders (used by train for global conflict resolution) */
	void ClearRiders() { RiderLocalOffsets.Empty(); }
};
