#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Breachborne/World/BBPowerDestructibleInterface.h"
#include "BBBasecampActor.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UArrowComponent;
class ABreachbornePlayerController;
class ABreachbornePlayerState;
struct FOnAttributeChangeData;

UENUM(BlueprintType)
enum class EBBBasecampInteractionType : uint8
{
	None        UMETA(DisplayName = "None"),
	RepairArmor UMETA(DisplayName = "Repair Armor"),
	BrewVive    UMETA(DisplayName = "Brew Vive")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBasecampCaptured,
	ABBBasecampActor*, Basecamp,
	int32, CapturingTeamID);

/**
 * Basecamp - a contested control point with Open Beta-era utility stations.
 *
 * One actor is intended to be placed over visual assets. Designers can move
 * PlatformZone, RecallLandingPoint, AnvilInteractZone, and CauldronInteractZone
 * in Blueprint to match the authored set dressing.
 */
UCLASS()
class BREACHBORNE_API ABBBasecampActor : public AActor, public IBBPowerDestructibleInterface
{
	GENERATED_BODY()

public:
	ABBBasecampActor();

	/** Fired on server when capture is complete. */
	UPROPERTY(BlueprintAssignable, Category = "Breachborne|PvE")
	FOnBasecampCaptured OnBasecampCaptured;

	/** Owning team ID. -1 means neutral. Replicated for minimap coloring. */
	UPROPERTY(ReplicatedUsing = OnRep_BasecampState, BlueprintReadOnly, Category = "Breachborne|PvE")
	int32 OwningTeamID = -1;

	/** Team currently making capture progress. -1 means no active capturer. */
	UPROPERTY(ReplicatedUsing = OnRep_BasecampState, BlueprintReadOnly, Category = "Breachborne|PvE")
	int32 CaptureTeamID = -1;

	/** Capture progress 0-1. Replicated for progress bar. */
	UPROPERTY(ReplicatedUsing = OnRep_BasecampState, BlueprintReadOnly, Category = "Breachborne|PvE")
	float CaptureProgress = 0.0f;

	/** True when more than one team is inside the platform zone. */
	UPROPERTY(ReplicatedUsing = OnRep_BasecampState, BlueprintReadOnly, Category = "Breachborne|PvE")
	bool bContested = false;

	/** Current basecamp station interaction type. */
	UPROPERTY(ReplicatedUsing = OnRep_BasecampState, BlueprintReadOnly, Category = "Breachborne|PvE")
	EBBBasecampInteractionType ActiveInteractionType = EBBBasecampInteractionType::None;

	/** Active station channel progress 0-1. */
	UPROPERTY(ReplicatedUsing = OnRep_BasecampState, BlueprintReadOnly, Category = "Breachborne|PvE")
	float StationProgress = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_BasecampState, BlueprintReadOnly, Category = "Breachborne|PvE")
	bool bDestroyed = false;

	/** Player currently channeling an anvil/cauldron interaction. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|PvE")
	TObjectPtr<ABreachbornePlayerState> ActiveInteractingPlayer;

	/** Called by player controller when Interact is pressed near the basecamp. */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|PvE")
	void ServerRequestInteract(ABreachbornePlayerController* InteractingController);

	virtual void ApplyPowerDestruction_Implementation(const FBBPowerDestructionContext& Context) override;

	UFUNCTION(BlueprintPure, Category = "Breachborne|PvE")
	bool IsDestroyed() const { return bDestroyed; }

	/** Recall landing hook for a future recall ability. */
	UFUNCTION(BlueprintPure, Category = "Breachborne|PvE")
	FTransform GetRecallLandingTransform() const;

	/** Whether this team currently owns the basecamp and can use it as a recall target. */
	UFUNCTION(BlueprintPure, Category = "Breachborne|PvE")
	bool CanTeamRecallHere(int32 TeamID) const;

	/** Clears ownership if this camp is the active camp for TeamID. Used when another camp is captured. */
	void ClearOwnershipForTeam(int32 TeamID);

	/** True when an actor is inside either station interaction zone. */
	UFUNCTION(BlueprintPure, Category = "Breachborne|PvE")
	bool IsActorInsideAnyStation(const AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "Breachborne|PvE|HUD")
	FVector GetPlatformWorldLocation() const;

	UFUNCTION(BlueprintPure, Category = "Breachborne|PvE|HUD")
	FVector GetAnvilWorldLocation() const;

	UFUNCTION(BlueprintPure, Category = "Breachborne|PvE|HUD")
	FVector GetCauldronWorldLocation() const;

	UFUNCTION(BlueprintPure, Category = "Breachborne|PvE|HUD")
	float GetPlatformRadius() const;

	UFUNCTION(BlueprintPure, Category = "Breachborne|PvE|HUD")
	float GetAnvilRadius() const;

	UFUNCTION(BlueprintPure, Category = "Breachborne|PvE|HUD")
	float GetCauldronRadius() const;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Radius players must stand inside to capture or heal. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	float CaptureRadius = 500.0f;

	/** Seconds for a single player to capture alone. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	float CaptureDuration = 6.0f;

	/** Health restored per second to owning team members standing on the platform. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	float OwnedHealPerSecond = 20.0f;

	/** Anvil channel duration in seconds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	float AnvilChannelDuration = 2.0f;

	/** Cauldron channel duration in seconds. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	float CauldronChannelDuration = 2.0f;

	/** Portion of max armor durability restored by one anvil repair. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	float ArmorRepairFraction = 0.20f;

	/** Distance leeway from station before a channel is cancelled. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	float StationCancelLeeway = 1.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE")
	TObjectPtr<UStaticMeshComponent> BasecampMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE")
	TObjectPtr<USphereComponent> PlatformZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE")
	TObjectPtr<USceneComponent> RecallLandingPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE")
	TObjectPtr<USphereComponent> AnvilInteractZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE")
	TObjectPtr<USphereComponent> CauldronInteractZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE|Editor")
	TObjectPtr<UTextRenderComponent> AnvilEditorLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE|Editor")
	TObjectPtr<UTextRenderComponent> CauldronEditorLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE|Editor")
	TObjectPtr<UTextRenderComponent> RecallEditorLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE|Editor")
	TObjectPtr<UArrowComponent> AnvilPlacementArrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE|Editor")
	TObjectPtr<UArrowComponent> CauldronPlacementArrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE|Editor")
	TObjectPtr<UArrowComponent> RecallPlacementArrow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE|Runtime")
	TObjectPtr<UTextRenderComponent> BasecampStateLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE|Runtime")
	TObjectPtr<UTextRenderComponent> AnvilPromptLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE|Runtime")
	TObjectPtr<UTextRenderComponent> CauldronPromptLabel;

private:
	UFUNCTION()
	void OnRep_BasecampState();

	/** 5Hz platform tick for capture and owned-team healing. */
	void PlatformTick();

	/** 10Hz station channel tick. */
	void StationTick();

	void BeginStationInteraction(ABreachbornePlayerState* PlayerState, EBBBasecampInteractionType InteractionType);
	void CancelStationInteraction(const TCHAR* Reason);
	void CompleteStationInteraction();
	void ClearStationInteractionState();

	bool CanPlayerUseStations(const ABreachbornePlayerState* PlayerState) const;
	bool IsPlayerInsideZone(const ABreachbornePlayerState* PlayerState, const USphereComponent* Zone, float RadiusScale = 1.0f) const;
	bool GetNearestStationForPlayer(const ABreachbornePlayerState* PlayerState, EBBBasecampInteractionType& OutType, float& OutDistSq) const;
	float GetChannelDuration(EBBBasecampInteractionType InteractionType) const;
	int32 GetArmorRepairCost(const ABreachbornePlayerState* PlayerState) const;
	void ApplyPlatformHeal(ABreachbornePlayerState* PlayerState, float DeltaSeconds);
	void OnInteractorHealthChanged(const FOnAttributeChangeData& Data);
	void ClaimOwnershipForTeam(int32 TeamID);
	void UpdateRuntimeIndicators();
	void ApplyDestroyedPresentation();

	FTimerHandle PlatformTickHandle;
	FTimerHandle StationTickHandle;
	FDelegateHandle InteractorHealthHandle;

	FVector StationStartLocation = FVector::ZeroVector;
	float StationElapsed = 0.0f;
};
