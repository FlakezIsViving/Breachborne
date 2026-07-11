#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Breachborne/Core/BreachborneTypes.h"
#include "BBWispPawn.generated.h"

class USphereComponent;
class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class AHunterCharacter;
class ABreachbornePlayerState;
class ABBWispPawn;

UENUM(BlueprintType)
enum class EWispState : uint8
{
	Active        UMETA(DisplayName = "Active"),
	BeingRevived  UMETA(DisplayName = "BeingRevived"),
	BeingExecuted UMETA(DisplayName = "BeingExecuted"),
	Revived       UMETA(DisplayName = "Revived"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWispReviveReady, ABBWispPawn*, Wisp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWispDied, ABBWispPawn*, Wisp, bool, bWasExecuted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWispExecuted, ABBWispPawn*, Wisp, AHunterCharacter*, Executor);

/**
 * The wisp pawn — spawned when a hunter is downed by combat damage.
 * Inherits from ACharacter so CharacterMovementComponent handles client→server
 * movement replication automatically.
 * Slow-moving, proximity-based rez/stomp, execute mechanic.
 * All gameplay logic is server-authoritative; state replicates to all clients.
 */
UCLASS()
class BREACHBORNE_API ABBWispPawn : public ACharacter
{
	GENERATED_BODY()

public:
	ABBWispPawn();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** Server: initialize the wisp with its owning player state */
	void InitWisp(ABreachbornePlayerState* OwnerPS);

	/** Server: begin execute sequence from an enemy hunter */
	void ServerBeginExecute(AHunterCharacter* Executor);

	/** Server: cancel execute (executor took damage or died) */
	void ServerCancelExecute();

	// --- Getters ---

	UFUNCTION(BlueprintPure, Category = "Breachborne|Wisp")
	EWispState GetWispState() const { return WispState; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Wisp")
	float GetRezBarProgress() const { return RezBarProgress; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Wisp")
	float GetCurrentWispHP() const { return ReplicatedWispHP; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Wisp")
	float GetMaxWispHP() const { return ReplicatedMaxWispHP; }

	/** Apply healing to this wisp (converted to revive progress). Server only. */
	void ApplyHeal(float HealAmount);

	UFUNCTION(BlueprintPure, Category = "Breachborne|Wisp")
	ABreachbornePlayerState* GetOwningPlayerState() const { return OwningPlayerState; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Wisp")
	AHunterCharacter* GetExecutingHunter() const { return ExecutingHunter; }

	/** Set the hunter carrying this wisp (Eluna passive). nullptr = drop. */
	void SetCarrier(AHunterCharacter* Carrier);

	UFUNCTION(BlueprintPure, Category = "Breachborne|Wisp")
	AHunterCharacter* GetCarrier() const { return CarriedBy.Get(); }

	/** The hunter currently carrying this wisp (Eluna passive) */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Wisp")
	TObjectPtr<AHunterCharacter> CarriedBy;

	/** The player who knocked this hunter into wisp state (for kill credit on wisp death) */
	UPROPERTY()
	TObjectPtr<ABreachbornePlayerState> KnockerPlayerState;

	/** The victim's last grounded location before entering wisp form (for abyss death shard spawning) */
	FVector PreWispGroundedLocation = FVector::ZeroVector;

	/** True if the wisp died by falling into the abyss */
	bool bDiedFromAbyss = false;

	/** Server: kill the wisp because it fell into the abyss */
	void ServerKillFromAbyss();

	float GetProximityRadius() const { return ProximityRadius; }
	float GetExecuteRadius() const { return ExecuteRadius; }

	// --- Delegates (server only, bound by GameMode) ---

	/** Fired when rez bar reaches 1.0 — GameMode should trigger revive */
	FOnWispReviveReady OnWispReviveReady;

	/** Fired when wisp HP reaches 0 (natural drain or stomp) — GameMode should trigger death */
	FOnWispDied OnWispDied;

	/** Fired when execute animation completes — GameMode should handle victim death + executor heal */
	FOnWispExecuted OnWispExecuted;

protected:
	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Wisp")
	TObjectPtr<USphereComponent> ProximitySphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Wisp")
	TObjectPtr<USphereComponent> ExecuteSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Wisp")
	TObjectPtr<UStaticMeshComponent> WispMesh;

	/** World-space UMG widget showing decay + revive bars above the wisp */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Wisp|UI")
	TObjectPtr<UWidgetComponent> IndicatorWidgetComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// --- Replicated State ---

	UPROPERTY(ReplicatedUsing = OnRep_WispState, BlueprintReadOnly, Category = "Breachborne|Wisp")
	EWispState WispState = EWispState::Active;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Wisp")
	float RezBarProgress = 0.0f;

	/** Replicated wisp HP for the decay bar (synced from WispHealthSet) */
	UPROPERTY(ReplicatedUsing = OnRep_WispHP, BlueprintReadOnly, Category = "Breachborne|Wisp")
	float ReplicatedWispHP = 100.0f;

	/** Replicated max wisp HP for the decay bar */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Wisp")
	float ReplicatedMaxWispHP = 100.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Wisp")
	TObjectPtr<ABreachbornePlayerState> OwningPlayerState;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Wisp")
	TObjectPtr<AHunterCharacter> ExecutingHunter;

	UFUNCTION()
	void OnRep_WispState();

	UFUNCTION()
	void OnRep_WispHP();

	// --- Tuning Constants ---

	/** Wisp move speed (cm/s) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Wisp|Tuning")
	float WispMoveSpeed = 150.0f;

	/** Base HP drain per second while in wisp form */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Wisp|Tuning")
	float BaseDrainRate = 2.0f;

	/** Multiplier on HP drain when an enemy is stomping */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Wisp|Tuning")
	float StompDrainMultiplier = 3.0f;

	/** Rez bar fill rate per second when an ally is nearby (1.0 = full) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Wisp|Tuning")
	float RezFillRate = 0.20f;

	/** Rez bar decay rate per second when NO ally is nearby (~50% of fill rate) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Wisp|Tuning")
	float RezDecayRate = 0.165f;

	/** Duration of the execute animation in seconds */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Wisp|Tuning")
	float ExecuteDuration = 2.0f;

	/** Proximity sphere radius for rez/stomp detection (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Wisp|Tuning")
	float ProximityRadius = 400.0f;

	/** Execute sphere radius for E-key execute detection (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Wisp|Tuning")
	float ExecuteRadius = 150.0f;

	/** How much revive progress (0-1) each point of healing contributes */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Wisp|Tuning")
	float HealToReviveConversion = 0.005f;

	/** Max revive multiplier (2.0 = 1/2 regular revive time) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Wisp|Tuning")
	float MaxReviveMultiplier = 2.0f;

private:
	/** Server: periodic tick for HP drain + proximity detection */
	void ServerWispTick();

	/** Server: execute animation completed */
	void ServerCompleteExecute();

	/** Called when the executing hunter takes damage — cancels the execute */
	void OnExecutorDamaged(const struct FOnAttributeChangeData& Data);

	/** Timer handles */
	FTimerHandle WispTickTimerHandle;
	FTimerHandle ExecuteTimerHandle;

	/** Delegate handle for executor damage monitoring */
	FDelegateHandle ExecutorDamageDelegate;

	/** Tick rate for wisp logic (seconds between ticks) */
	static constexpr float WispTickInterval = 0.2f;

	/** Timestamp when wisp entered BeingRevived state (for timing diagnostics) */
	float ReviveStartTime = -1.0f;

	/** Healing accumulated since last tick (converted to revive progress) */
	float HealAccumulated = 0.0f;
};
