#include "BreachborneGameState.h"
#include "Breachborne/Core/BBGameInstance.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Breachborne/Breachborne.h"

ABreachborneGameState::ABreachborneGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	// Tick at a reduced rate — 1Hz is sufficient for match time tracking
	PrimaryActorTick.TickInterval = 1.0f;
}

void ABreachborneGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABreachborneGameState, MatchPhase);
	DOREPLIFETIME(ABreachborneGameState, ElapsedMatchTime);
	DOREPLIFETIME(ABreachborneGameState, AliveTeamCount);
	DOREPLIFETIME(ABreachborneGameState, PhaseTimeRemaining);
	DOREPLIFETIME(ABreachborneGameState, WinningTeamID);
	DOREPLIFETIME(ABreachborneGameState, Teams);
	DOREPLIFETIME(ABreachborneGameState, LobbyTeams);
	DOREPLIFETIME(ABreachborneGameState, SpectatorSlots);
	DOREPLIFETIME(ABreachborneGameState, LobbySettings);
	DOREPLIFETIME(ABreachborneGameState, LobbyOwnerPlayerState);
}

void ABreachborneGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority() && MatchPhase == EMatchPhase::Playing)
	{
		ElapsedMatchTime += DeltaSeconds;
	}
}

void ABreachborneGameState::InitTeams(int32 NumTeams)
{
	if (!HasAuthority())
	{
		return;
	}

	Teams.Empty(NumTeams);
	for (int32 i = 0; i < NumTeams; ++i)
	{
		FTeamInfo NewTeam;
		NewTeam.TeamID = i;
		NewTeam.AliveCount = 0;
		NewTeam.KillCount = 0;
		Teams.Add(NewTeam);
	}

	AliveTeamCount = 0;
}

void ABreachborneGameState::SetMatchPhase(EMatchPhase NewPhase)
{
	if (!HasAuthority())
	{
		return;
	}

	if (MatchPhase != NewPhase)
	{
		MatchPhase = NewPhase;
		OnRep_MatchPhase();

		UE_LOG(LogBreachborne, Log, TEXT("Match phase changed to: %d"), static_cast<int32>(NewPhase));
	}
}

void ABreachborneGameState::SetPhaseTimeRemaining(float NewTimeRemaining)
{
	if (HasAuthority())
	{
		PhaseTimeRemaining = FMath::Max(0.0f, NewTimeRemaining);
	}
}

void ABreachborneGameState::SetWinningTeamID(int32 NewWinningTeamID)
{
	if (HasAuthority())
	{
		WinningTeamID = NewWinningTeamID;
	}
}

void ABreachborneGameState::ResetMatchClock()
{
	if (HasAuthority())
	{
		ElapsedMatchTime = 0.0f;
		PhaseTimeRemaining = 0.0f;
		WinningTeamID = -1;
	}
}

void ABreachborneGameState::InitLobby(const FBBLobbySettings& NewSettings)
{
	if (!HasAuthority())
	{
		return;
	}

	LobbySettings = NewSettings;
	LobbySettings.TeamSize = FMath::Clamp(LobbySettings.TeamSize, 1, 4);
	LobbySettings.MaxPlayers = FMath::Clamp(LobbySettings.MaxPlayers, 1, 40);
	LobbySettings.MaxTeams = FMath::Max(1, FMath::CeilToInt(static_cast<float>(LobbySettings.MaxPlayers) / static_cast<float>(LobbySettings.TeamSize)));

	LobbyTeams.Empty(LobbySettings.MaxTeams);
	for (int32 TeamID = 0; TeamID < LobbySettings.MaxTeams; ++TeamID)
	{
		FBBLobbyTeam Team;
		Team.TeamID = TeamID;
		Team.DisplayName = FString::Printf(TEXT("Team %d"), TeamID + 1);
		for (int32 SlotIndex = 0; SlotIndex < LobbySettings.TeamSize; ++SlotIndex)
		{
			FBBLobbySlot Slot;
			Slot.TeamID = TeamID;
			Slot.SlotIndex = SlotIndex;
			Slot.SlotType = EBBLobbyPlayerSlotType::Open;
			Team.Slots.Add(Slot);
		}
		LobbyTeams.Add(Team);
	}

	SpectatorSlots.Reset();
	ForceNetUpdate();
}

void ABreachborneGameState::RebuildLobbyPreservingPlayers(const TArray<ABreachbornePlayerState*>& OrderedPlayers)
{
	if (!HasAuthority())
	{
		return;
	}

	FBBLobbySettings Settings = LobbySettings;
	InitLobby(Settings);

	for (ABreachbornePlayerState* PlayerState : OrderedPlayers)
	{
		if (!PlayerState)
		{
			continue;
		}

		bool bPlaced = false;
		for (FBBLobbyTeam& Team : LobbyTeams)
		{
			for (FBBLobbySlot& Slot : Team.Slots)
			{
				if (Slot.SlotType == EBBLobbyPlayerSlotType::Open)
				{
					Slot.PlayerState = PlayerState;
					Slot.SlotType = EBBLobbyPlayerSlotType::Player;
					PlayerState->SetTeamID(Team.TeamID);
					PlayerState->SetLobbySlotIndex(Slot.SlotIndex);
					PlayerState->SetIsSpectator(false);
					bPlaced = true;
					break;
				}
			}
			if (bPlaced)
			{
				break;
			}
		}

		if (!bPlaced && LobbySettings.bAllowSpectators)
		{
			AddSpectator(PlayerState);
		}
	}

	ForceNetUpdate();
}

bool ABreachborneGameState::SetLobbySlot(int32 TeamID, int32 SlotIndex, APlayerState* PlayerState)
{
	if (!HasAuthority() || !PlayerState || !LobbyTeams.IsValidIndex(TeamID) || !LobbyTeams[TeamID].Slots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	FBBLobbySlot& TargetSlot = LobbyTeams[TeamID].Slots[SlotIndex];
	if (TargetSlot.SlotType != EBBLobbyPlayerSlotType::Open)
	{
		return false;
	}

	ClearLobbyPlayer(PlayerState);

	TargetSlot.PlayerState = PlayerState;
	TargetSlot.SlotType = EBBLobbyPlayerSlotType::Player;

	if (ABreachbornePlayerState* BBPS = Cast<ABreachbornePlayerState>(PlayerState))
	{
		BBPS->SetTeamID(TeamID);
		BBPS->SetLobbySlotIndex(SlotIndex);
		BBPS->SetIsSpectator(false);
	}

	ForceNetUpdate();
	return true;
}

void ABreachborneGameState::ClearLobbyPlayer(APlayerState* PlayerState)
{
	if (!HasAuthority() || !PlayerState)
	{
		return;
	}

	for (FBBLobbyTeam& Team : LobbyTeams)
	{
		for (FBBLobbySlot& Slot : Team.Slots)
		{
			if (Slot.PlayerState == PlayerState)
			{
				Slot.PlayerState = nullptr;
				Slot.SlotType = EBBLobbyPlayerSlotType::Open;
			}
		}
	}

	SpectatorSlots.RemoveAll([PlayerState](const FBBLobbySlot& Slot)
	{
		return Slot.PlayerState == PlayerState;
	});
}

void ABreachborneGameState::AddSpectator(APlayerState* PlayerState)
{
	if (!HasAuthority() || !PlayerState || !LobbySettings.bAllowSpectators)
	{
		return;
	}

	ClearLobbyPlayer(PlayerState);

	FBBLobbySlot Slot;
	Slot.TeamID = -1;
	Slot.SlotIndex = SpectatorSlots.Num();
	Slot.PlayerState = PlayerState;
	Slot.SlotType = EBBLobbyPlayerSlotType::Spectator;
	SpectatorSlots.Add(Slot);

	if (ABreachbornePlayerState* BBPS = Cast<ABreachbornePlayerState>(PlayerState))
	{
		BBPS->SetTeamID(-1);
		BBPS->SetLobbySlotIndex(Slot.SlotIndex);
		BBPS->SetIsSpectator(true);
	}

	ForceNetUpdate();
}

void ABreachborneGameState::SetLobbyOwner(APlayerState* PlayerState)
{
	if (HasAuthority())
	{
		LobbyOwnerPlayerState = PlayerState;
		ForceNetUpdate();
	}
}

void ABreachborneGameState::SetLobbySettings(const FBBLobbySettings& NewSettings)
{
	if (HasAuthority())
	{
		LobbySettings = NewSettings;
		LobbySettings.TeamSize = FMath::Clamp(LobbySettings.TeamSize, 1, 4);
		LobbySettings.MaxPlayers = FMath::Clamp(LobbySettings.MaxPlayers, 1, 40);
		LobbySettings.MaxTeams = FMath::Max(1, FMath::CeilToInt(static_cast<float>(LobbySettings.MaxPlayers) / static_cast<float>(LobbySettings.TeamSize)));
		ForceNetUpdate();
	}
}

int32 ABreachborneGameState::GetLobbyActivePlayerCount() const
{
	int32 Count = 0;
	for (const FBBLobbyTeam& Team : LobbyTeams)
	{
		for (const FBBLobbySlot& Slot : Team.Slots)
		{
			if (Slot.SlotType == EBBLobbyPlayerSlotType::Player && Slot.PlayerState)
			{
				++Count;
			}
		}
	}
	return Count;
}

void ABreachborneGameState::UpdateTeamAliveCount(int32 TeamID, int32 Delta)
{
	if (!HasAuthority())
	{
		return;
	}

	if (Teams.IsValidIndex(TeamID))
	{
		const int32 OldAlive = Teams[TeamID].AliveCount;
		Teams[TeamID].AliveCount = FMath::Max(0, Teams[TeamID].AliveCount + Delta);

		// Recalculate alive team count
		AliveTeamCount = 0;
		for (const FTeamInfo& Team : Teams)
		{
			if (Team.AliveCount > 0)
			{
				AliveTeamCount++;
			}
		}

		UE_LOG(LogBreachborne, Verbose, TEXT("Team %d alive count: %d -> %d (Total alive teams: %d)"),
			TeamID, OldAlive, Teams[TeamID].AliveCount, AliveTeamCount);
	}
}

void ABreachborneGameState::OnRep_MatchPhase()
{
	UE_LOG(LogBreachborne, Log, TEXT("Match phase replicated: %d"), static_cast<int32>(MatchPhase));

	if (UWorld* World = GetWorld())
	{
		if (UBBGameInstance* GI = World->GetGameInstance<UBBGameInstance>())
		{
			GI->SyncFrontendToMatchPhase(MatchPhase);
		}
	}
}
