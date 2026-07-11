#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BreachborneTypes.h"
#include "BreachborneGameState.generated.h"

/**
 * Replicated to all clients. Holds match-wide data: phase, storm, teams, elapsed time.
 */
UCLASS()
class BREACHBORNE_API ABreachborneGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ABreachborneGameState();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Initialize team slots (called by GameMode) */
	void InitTeams(int32 NumTeams);

	/** Set the current match phase (server only) */
	void SetMatchPhase(EMatchPhase NewPhase);

	/** Adjust a team's alive count by Delta (server only) */
	void UpdateTeamAliveCount(int32 TeamID, int32 Delta);

	/** Get current match phase */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Match")
	EMatchPhase GetMatchPhase() const { return MatchPhase; }

	/** Get elapsed match time in seconds */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Match")
	float GetElapsedMatchTime() const { return ElapsedMatchTime; }

	/** Get number of teams still alive */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Match")
	int32 GetAliveTeamCount() const { return AliveTeamCount; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Match")
	float GetPhaseTimeRemaining() const { return PhaseTimeRemaining; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Match")
	int32 GetWinningTeamID() const { return WinningTeamID; }

	/** Get the full team info array */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Match")
	const TArray<FTeamInfo>& GetTeams() const { return Teams; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Lobby")
	const TArray<FBBLobbyTeam>& GetLobbyTeams() const { return LobbyTeams; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Lobby")
	const TArray<FBBLobbySlot>& GetSpectatorSlots() const { return SpectatorSlots; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Lobby")
	const FBBLobbySettings& GetLobbySettings() const { return LobbySettings; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Lobby")
	APlayerState* GetLobbyOwnerPlayerState() const { return LobbyOwnerPlayerState; }

	void SetPhaseTimeRemaining(float NewTimeRemaining);
	void SetWinningTeamID(int32 NewWinningTeamID);
	void ResetMatchClock();
	void InitLobby(const FBBLobbySettings& NewSettings);
	void RebuildLobbyPreservingPlayers(const TArray<class ABreachbornePlayerState*>& OrderedPlayers);
	bool SetLobbySlot(int32 TeamID, int32 SlotIndex, APlayerState* PlayerState);
	void ClearLobbyPlayer(APlayerState* PlayerState);
	void AddSpectator(APlayerState* PlayerState);
	void SetLobbyOwner(APlayerState* PlayerState);
	void SetLobbySettings(const FBBLobbySettings& NewSettings);
	int32 GetLobbyActivePlayerCount() const;

	// --- Stubs for in-progress features ---
	EBBDayNightPhase GetDayNightPhase() const { return EBBDayNightPhase::Day; }
	int32 GetDayNightCycle() const { return 0; }
	bool IsNight() const { return false; }
	void AdvanceDayNightPhase() {}

protected:
	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase, BlueprintReadOnly, Category = "Breachborne|Match")
	EMatchPhase MatchPhase = EMatchPhase::WaitingForPlayers;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Match")
	float ElapsedMatchTime = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Match")
	int32 AliveTeamCount = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Match")
	float PhaseTimeRemaining = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Match")
	int32 WinningTeamID = -1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Match")
	TArray<FTeamInfo> Teams;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Lobby")
	TArray<FBBLobbyTeam> LobbyTeams;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Lobby")
	TArray<FBBLobbySlot> SpectatorSlots;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Lobby")
	FBBLobbySettings LobbySettings;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Lobby")
	TObjectPtr<APlayerState> LobbyOwnerPlayerState;

	UFUNCTION()
	void OnRep_MatchPhase();
};
