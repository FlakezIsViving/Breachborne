#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "StormTypes.h"
#include "BBStormManager.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UStormShiftBase;

/**
 * Server-authoritative storm manager. Drives phase progression, applies DOT
 * damage to hunters outside the safe zone via the GAS pipeline (BBDamageExecution).
 * Replicates circle state to all clients for debug visualization.
 *
 * Spawned by GameMode in InitGameState. Starts automatically after AutoStartDelay
 * or can be started manually via StartStorm().
 */
UCLASS()
class BREACHBORNE_API ABBStormManager : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABBStormManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Start the storm. Called automatically if bAutoStart, or manually by GameMode. */
	void StartStorm();

	/** Reset all storm phase, radius, center, and damage timer state back to a new-match baseline. */
	void ResetStorm();

protected:
	// --- Editor-configurable ---

	/** Starting radius of the safe zone (world units). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Storm")
	float InitialRadius = 15000.0f;

	/** Starting center of the safe zone (2D world space). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Storm")
	FVector2D InitialCenter = FVector2D::ZeroVector;

	/** If true, storm starts automatically after AutoStartDelay seconds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Storm")
	bool bAutoStart = true;

	/** Seconds after BeginPlay before the storm auto-starts. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Storm", meta = (EditCondition = "bAutoStart"))
	float AutoStartDelay = 5.0f;

	/** Storm shift class to instantiate. Determines phase configs and center selection. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Storm")
	TSubclassOf<UStormShiftBase> StormShiftClass;

	/** Z height for debug draw circles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Storm")
	float DebugDrawZ = 100.0f;

	// --- Replicated state ---

	UPROPERTY(Replicated)
	FVector2D CurrentCenter;

	UPROPERTY(Replicated)
	float CurrentRadius;

	UPROPERTY(Replicated)
	FVector2D TargetCenter;

	UPROPERTY(Replicated)
	float TargetRadius;

	UPROPERTY(Replicated)
	int32 CurrentPhaseIndex;

	UPROPERTY(Replicated)
	EStormPhaseState PhaseState;

private:
	// --- Components ---

	UPROPERTY(VisibleAnywhere, Category = "Breachborne|Storm")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// --- Runtime state (server only) ---

	UPROPERTY()
	TObjectPtr<UStormShiftBase> ActiveShift;

	UPROPERTY()
	TObjectPtr<UGameplayEffect> StormDamageGE;

	TArray<FStormPhaseConfig> PhaseConfigs;

	/** Radius at the start of the current shrink (for lerp). */
	float ShrinkStartRadius = 0.0f;

	/** Center at the start of the current shrink (for lerp). */
	FVector2D ShrinkStartCenter = FVector2D::ZeroVector;

	/** Elapsed time in the current phase state (Shrinking or Holding). */
	float PhaseElapsedTime = 0.0f;

	FTimerHandle DamageTickTimerHandle;
	FTimerHandle AutoStartTimerHandle;

	// --- Server logic ---

	void AdvanceToPhase(int32 PhaseIndex);
	void OnPhaseHoldComplete();
	void ApplyStormDamage();
	void StartDamageTimer(float Interval);

	// --- Client visualization ---

	void DrawDebugStormCircles() const;
};
