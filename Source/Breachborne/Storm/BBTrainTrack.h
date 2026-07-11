#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "BBTrainTrack.generated.h"

class USplineComponent;
class UStaticMeshComponent;
class UStaticMesh;
class ABBTrain;
class UHierarchicalInstancedStaticMeshComponent;

/**
 * A designer-placed train track defined by a spline.
 *
 * Drag into the level, edit spline points (Alt+click to add, drag handles to curve).
 * The track auto-generates rail meshes along the spline in OnConstruction().
 * Trains follow this spline at runtime.
 *
 * This solves the "modular rail snapping" problem by using a single spline actor
 * instead of trying to snap individual rail pieces together.
 */
UCLASS()
class BREACHBORNE_API ABBTrainTrack : public AActor
{
	GENERATED_BODY()

public:
	ABBTrainTrack();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Get the spline component that defines the track path */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	USplineComponent* GetSpline() const { return TrackSpline; }

	/** Total length of the spline in world units */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	float GetTrackLength() const;

	/** Is the spline a closed loop? */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	bool IsLoop() const;

	/** Get a world-space transform at a given distance along the spline */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	FTransform GetTransformAtDistance(float Distance, bool bWorldSpace = true) const;

	/** Get a world-space transform at a given 0-1 progress along the spline */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	FTransform GetTransformAtProgress(float Progress, bool bWorldSpace = true) const;

	/** Convert a world progress (0-1) to a distance along the spline */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	float GetDistanceAtProgress(float Progress) const;

	/** Convert a distance to a world progress (0-1) */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	float GetProgressAtDistance(float Distance) const;

	/** Advance a progress value by DeltaDistance, wrapping for loops */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Train")
	float AdvanceProgress(float CurrentProgress, float DeltaDistance, bool bForward = true) const;

#if WITH_EDITOR
	/** Add a new spline point at the end of the track */
	UFUNCTION(CallInEditor, Category = "Breachborne|Train")
	void AddPointAtEnd();

	/** Remove the last spline point */
	UFUNCTION(CallInEditor, Category = "Breachborne|Train")
	void RemoveLastPoint();

	/** Clear all spline points and reset to default straight line */
	UFUNCTION(CallInEditor, Category = "Breachborne|Train")
	void ResetToDefault();

	/** Manually force a track rebuild (useful if track looks broken) */
	UFUNCTION(CallInEditor, Category = "Breachborne|Train")
	void ForceRebuild();

	/** Spawn a test train on this track for preview */
	UFUNCTION(CallInEditor, Category = "Breachborne|Train")
	void SpawnTestTrain();

	/** Destroy the test train */
	UFUNCTION(CallInEditor, Category = "Breachborne|Train")
	void DestroyTestTrain();
#endif

protected:
	/** The spline that defines the track path */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<USplineComponent> TrackSpline;

	/** Static mesh to use for rail segments. If null, procedural boxes are used. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	TObjectPtr<UStaticMesh> RailMesh;

	/** Static mesh to use for sleepers (ties). If null, procedural boxes are used. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	TObjectPtr<UStaticMesh> SleeperMesh;

	/** Width of the track — distance between the two rails */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	float TrackWidth = 300.0f;

	/** Length of each rail mesh segment. Smaller = smoother curves but more meshes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	float SegmentLength = 200.0f;

	/** Vertical offset for rail meshes (raise/lower relative to spline) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	float RailMeshVerticalOffset = 0.0f;

	/** If true, regenerate rail meshes in OnConstruction (editor-only) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	bool bAutoGenerateRails = true;

	/** Rail height (thickness) — used for procedural rails */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	float RailHeight = 20.0f;

	/** Rail width (thickness) — used for procedural rails */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	float RailThickness = 10.0f;

	/** Sleeper width (cross-track) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	float SleeperWidth = 340.0f;

	/** Sleeper length (along-track) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	float SleeperLength = 40.0f;

	/** Sleeper height (thickness) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	float SleeperHeight = 15.0f;

	/** Spacing between sleepers */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Train|Visual")
	float SleeperSpacing = 200.0f;

private:
	/** Rebuild rail meshes along the track spline */
	void RebuildRailMeshes();

	/** Rebuild sleeper (tie) meshes along the track */
	void RebuildSleepers(float SplineLength);

	/** Clear existing rail mesh components */
	void ClearRailMeshes();

	/** Clear existing sleeper meshes */
	void ClearSleepers();

	/** Default cube mesh for procedural visuals */
	UPROPERTY()
	TObjectPtr<UStaticMesh> DefaultCubeMesh;

	/** Test train spawned for preview */
	UPROPERTY()
	TObjectPtr<class ABBTrain> TestTrain;

protected:
	/** Instanced mesh for left rail segments */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> LeftRailInstances;

	/** Instanced mesh for right rail segments */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RightRailInstances;

	/** Instanced mesh for sleepers (ties) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Train")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> SleeperInstances;

	/** Editor-only: detect spline changes and rebuild rails */
	int32 LastSplinePointCount = -1;
	float LastSplineLength = -1.0f;
#if WITH_EDITOR
	float TimeSinceSplineChanged = 0.0f;
	static constexpr float RebuildDelay = 0.3f; // Rebuild 0.3s after user stops editing
#endif
};
