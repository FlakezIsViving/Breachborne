#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BreachborneTypes.h"
#include "BreachborneGameMode.generated.h"

class ABBStormManager;
class ABreachborneGameState;
class ABreachbornePlayerState;
class UBBHunterDefinition;
class UAbilitySystemComponent;

/**
 * Server-only GameMode. Handles lobby, team assignment, match phases, spawning,
 * and granting hunter abilities via GAS.
 * Never exists on clients — NEVER call GetGameMode() on clients.
 */
UCLASS()
class BREACHBORNE_API ABreachborneGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABreachborneGameMode();

	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** Set the current match phase (server only, updates GameState) */
	void SetMatchPhase(EMatchPhase NewPhase);

	void RequestHunterSelection(APlayerController* PlayerController, int32 HunterID);
	void SetPlayerReady(APlayerController* PlayerController, bool bReady);
	void RequestJoinLobbySlot(APlayerController* PlayerController, int32 TeamID, int32 SlotIndex);
	void RequestJoinSpectators(APlayerController* PlayerController);
	void RequestAutoFillLobbySlot(APlayerController* PlayerController);
	void RequestSetLobbyTeamSize(APlayerController* PlayerController, int32 TeamSize);
	void RequestSetLobbyDescription(APlayerController* PlayerController, const FString& Description);
	void RequestSetStormShiftPreset(APlayerController* PlayerController, FName PresetID);
	void RequestStartLobbyMatch(APlayerController* PlayerController);
	bool DebugValidateHunterDefinition(int32 HunterID) const;

protected:
	/** Number of players per squad (3 or 4) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Match")
	int32 SquadSize = 4;

	/** Maximum number of teams in the lobby */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Match")
	int32 MaxTeams = 10;

	/** Runtime fallback definitions used when content assets are missing or unsafe to load. */
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UBBHunterDefinition>> NativeHunterDefinitions;

	/** Default hunter ID assigned to players who haven't selected one */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunters")
	int32 DefaultHunterID = 1;

	/** If true, dead hunters auto-respawn after a delay (for dev/test iteration) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Dev")
	bool bDevAutoRespawn = false;

	/** Seconds before a dead hunter auto-respawns (when bDevAutoRespawn is true) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Dev", meta = (EditCondition = "bDevAutoRespawn"))
	float DevRespawnDelay = 3.0f;

private:
	/** Tracks how many players have been assigned so far for round-robin team assignment */
	int32 NextPlayerIndex = 0;

	/** Cached pointer to our GameState — valid after InitGameState */
	UPROPERTY()
	TObjectPtr<ABreachborneGameState> BBGameState;

	/** Cached pointer to the storm manager — spawned in InitGameState */
	UPROPERTY()
	TObjectPtr<ABBStormManager> StormManager;

	float MatchCountdownRemaining = 0.0f;
	float PostMatchRemaining = 0.0f;
	int32 MatchStartAliveTeamCount = 0;

	bool IsEditorQuickPlay() const;
	bool ShouldUseDevAutoRespawn() const;
	bool IsFrontendMatchFlow() const;
	void ApplyConfiguredMatchSettings();
	int32 AssignTeamForNewPlayer() const;
	void InitializeLobbyState();
	bool IsLobbyOwner(const APlayerController* PlayerController) const;
	void AssignLobbyOwnerIfNeeded();
	bool TryAutoFillLobbySlot(ABreachbornePlayerState* PlayerState);
	TArray<ABreachbornePlayerState*> GetOrderedLobbyPlayers() const;
	bool CanStartLobbyMatch(FString& OutReason) const;
	bool IsHunterSelectable(int32 HunterID) const;
	const UBBHunterDefinition* ResolveHunterDefinition(int32 HunterID) const;
	void BuildNativeHunterDefinitions();
	bool AreAllConnectedPlayersReady() const;
	void StartHunterSelect();
	void BeginMatchCountdown();
	void StartGameplayMatch();
	bool ShouldEvaluateWinCondition() const;
	void EnterPostMatch(int32 WinningTeamID);
	void ResetToHunterSelect();
	void SpawnPlayerForMatch(APlayerController* PlayerController);
	FTransform GetDropTransformForPlayer(const ABreachbornePlayerState* PlayerState) const;
	void ClearHunterAbilities(APlayerController* PlayerController);

	/** Grant all abilities from a HunterDefinition to a player's ASC */
	void GrantHunterAbilities(APlayerController* PlayerController, const UBBHunterDefinition* HunterDef);

	/** Called when any hunter's health reaches 0. Disables victim pawn, tracks kills, triggers dev respawn. */
	UFUNCTION()
	void OnHunterKilled(UAbilitySystemComponent* VictimASC, UAbilitySystemComponent* KillerASC);

	/** Dev respawn: reset health, re-enable pawn, teleport to spawn point */
	void DevRespawnHunter(UAbilitySystemComponent* VictimASC);

public:
	void HandleTrainDeath(class AHunterCharacter* Hunter, const FVector& Location);
};
