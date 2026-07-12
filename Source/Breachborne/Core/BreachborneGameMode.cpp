#include "BreachborneGameMode.h"
#include "BreachborneGameState.h"
#include "BBNetworkDevSettings.h"
#include "BBTeamDropSpawn.h"
#include "BreachbornePlayerState.h"
#include "BreachbornePlayerController.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Characters/GliderComponent.h"
#include "Breachborne/UI/BBDebugHUD.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBHunterDefinition.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Abilities/BBCombatSet.h"
#include "Breachborne/Abilities/BBMovementSet.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/Hunters/Ghost/GA_Ghost_LMB.h"
#include "Breachborne/Abilities/Hunters/Ghost/GA_Ghost_Passive.h"
#include "Breachborne/Abilities/Hunters/Ghost/GA_Ghost_Q.h"
#include "Breachborne/Abilities/Hunters/Ghost/GA_Ghost_R.h"
#include "Breachborne/Abilities/Hunters/Ghost/GA_Ghost_RMB.h"
#include "Breachborne/Abilities/Hunters/Ghost/GA_Ghost_Shift.h"
#include "Breachborne/Abilities/Hunters/Eluna/GA_Eluna_LMB.h"
#include "Breachborne/Abilities/Hunters/Eluna/GA_Eluna_Passive.h"
#include "Breachborne/Abilities/Hunters/Eluna/GA_Eluna_Q.h"
#include "Breachborne/Abilities/Hunters/Eluna/GA_Eluna_R.h"
#include "Breachborne/Abilities/Hunters/Eluna/GA_Eluna_RMB.h"
#include "Breachborne/Abilities/Hunters/Eluna/GA_Eluna_Shift.h"
#include "Breachborne/Abilities/Hunters/Kingpin/GA_Kingpin_LMB.h"
#include "Breachborne/Abilities/Hunters/Kingpin/GA_Kingpin_Passive.h"
#include "Breachborne/Abilities/Hunters/Kingpin/GA_Kingpin_Q.h"
#include "Breachborne/Abilities/Hunters/Kingpin/GA_Kingpin_R.h"
#include "Breachborne/Abilities/Hunters/Kingpin/GA_Kingpin_RMB.h"
#include "Breachborne/Abilities/Hunters/Kingpin/GA_Kingpin_Shift.h"
#include "Breachborne/Abilities/Hunters/Hudson/GA_Hudson_LMB_Minigun.h"
#include "Breachborne/Abilities/Hunters/Hudson/GA_Hudson_Passive_Salvager.h"
#include "Breachborne/Abilities/Hunters/Hudson/GA_Hudson_Q_BarbedWire.h"
#include "Breachborne/Abilities/Hunters/Hudson/GA_Hudson_R_SalvageHook.h"
#include "Breachborne/Abilities/Hunters/Hudson/GA_Hudson_RMB_SpinBarrels.h"
#include "Breachborne/Abilities/Hunters/Hudson/GA_Hudson_Shift_HoverJets.h"
#include "Breachborne/Abilities/Hunters/Crysta/GA_Crysta_LMB.h"
#include "Breachborne/Abilities/Hunters/Crysta/GA_Crysta_Passive.h"
#include "Breachborne/Abilities/Hunters/Crysta/GA_Crysta_Q.h"
#include "Breachborne/Abilities/Hunters/Crysta/GA_Crysta_R.h"
#include "Breachborne/Abilities/Hunters/Crysta/GA_Crysta_RMB.h"
#include "Breachborne/Abilities/Hunters/Crysta/GA_Crysta_Shift.h"
#include "Breachborne/Abilities/Hunters/Void/GA_Void_LMB.h"
#include "Breachborne/Abilities/Hunters/Void/GA_Void_Passive.h"
#include "Breachborne/Abilities/Hunters/Void/GA_Void_Q.h"
#include "Breachborne/Abilities/Hunters/Void/GA_Void_R.h"
#include "Breachborne/Abilities/Hunters/Void/GA_Void_RMB.h"
#include "Breachborne/Abilities/Hunters/Void/GA_Void_Shift.h"
#include "Breachborne/Storm/BBStormManager.h"
#include "Breachborne/Items/BBInventoryManager.h"
#include "Breachborne/Items/BBWorldItemSpawner.h"
#include "Breachborne/Wisp/BBWispPawn.h"
#include "Breachborne/Wisp/BBDeathboxActor.h"
#include "Breachborne/Breachborne.h"
#include "Engine/Blueprint.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

namespace
{
	bool IsHudsonVisualSetReadyForContentDefinition()
	{
		return FPaths::FileExists(FPaths::ProjectContentDir() / TEXT("Hunters/Hudson/VIS_Hudson.uasset"));
	}

	UBBHunterDefinition* LoadHunterDefinitionAsset(const TCHAR* AssetPath)
	{
		UObject* LoadedAsset = StaticLoadObject(UObject::StaticClass(), nullptr, AssetPath);
		if (UBBHunterDefinition* HunterDefinition = Cast<UBBHunterDefinition>(LoadedAsset))
		{
			return HunterDefinition;
		}

		if (const UBlueprint* Blueprint = Cast<UBlueprint>(LoadedAsset))
		{
			return Blueprint->GeneratedClass ? Cast<UBBHunterDefinition>(Blueprint->GeneratedClass->GetDefaultObject()) : nullptr;
		}

		return nullptr;
	}
}

ABreachborneGameMode::ABreachborneGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	GameStateClass = ABreachborneGameState::StaticClass();
	PlayerStateClass = ABreachbornePlayerState::StaticClass();
	PlayerControllerClass = ABreachbornePlayerController::StaticClass();
	DefaultPawnClass = AHunterCharacter::StaticClass();
	HUDClass = ABBDebugHUD::StaticClass();
	WispPawnClass = ABBWispPawn::StaticClass();
	DeathboxClass = ABBDeathboxActor::StaticClass();
}

void ABreachborneGameMode::InitGameState()
{
	Super::InitGameState();

	ApplyConfiguredMatchSettings();

	BBGameState = GetGameState<ABreachborneGameState>();
	if (BBGameState)
	{
		// Initialize team slots
		BBGameState->InitTeams(MaxTeams);
		InitializeLobbyState();
		BBGameState->ResetMatchClock();
		BBGameState->SetMatchPhase(IsEditorQuickPlay() ? EMatchPhase::Playing : EMatchPhase::WaitingForPlayers);
	}

	BuildNativeHunterDefinitions();

	// Spawn the storm manager
	if (UWorld* World = GetWorld())
	{
		StormManager = World->SpawnActor<ABBStormManager>();
		UE_LOG(LogBreachborne, Log, TEXT("GameMode: Spawned StormManager"));
	}
}

void ABreachborneGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	const bool bQuickPlay = IsEditorQuickPlay();
	const int32 AssignedTeamID = bQuickPlay ? 0 : -1;
	NextPlayerIndex++;

	ABreachbornePlayerState* BBPS = NewPlayer->GetPlayerState<ABreachbornePlayerState>();
	if (BBPS)
	{
		BBPS->SetTeamID(AssignedTeamID);
		BBPS->SetHunterID(DefaultHunterID);
		BBPS->SetReadyForMatch(false);
		BBPS->SetHunterLocked(bQuickPlay);
		BBPS->SetIsAlive(bQuickPlay);

		UE_LOG(LogBreachborne, Log, TEXT("Player %s assigned to Team %d"), *BBPS->GetPlayerName(), AssignedTeamID);
	}

	if (!bQuickPlay)
	{
		const EMatchPhase CurrentPhase = BBGameState ? BBGameState->GetMatchPhase() : EMatchPhase::WaitingForPlayers;
		if (CurrentPhase == EMatchPhase::WaitingForPlayers)
		{
			AssignLobbyOwnerIfNeeded();
		}

		if (BBPS)
		{
			if (CurrentPhase == EMatchPhase::WaitingForPlayers)
			{
				TryAutoFillLobbySlot(BBPS);
			}
			else
			{
				BBPS->SetTeamID(-1);
				BBPS->SetLobbySlotIndex(-1);
				BBPS->SetIsSpectator(true);
				BBPS->SetReadyForMatch(false);
				BBPS->SetHunterLocked(false);
				BBPS->SetIsAlive(false);
				UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|LateJoinSpectator player=%s phase=%d"),
					*BBPS->GetPlayerName(),
					static_cast<int32>(CurrentPhase));
			}
		}
		if (ABreachbornePlayerController* BBPC = Cast<ABreachbornePlayerController>(NewPlayer))
		{
			BBPC->ClientEnterFrontendPhase(CurrentPhase == EMatchPhase::PostMatch ? EMatchPhase::PostMatch : EMatchPhase::WaitingForPlayers);
		}
		return;
	}

	// Update alive count on GameState
	if (BBGameState)
	{
		BBGameState->UpdateTeamAliveCount(AssignedTeamID, 1);
	}

	// Grant hunter abilities based on HunterDefinition
	const UBBHunterDefinition* HunterDef = ResolveHunterDefinition(DefaultHunterID);

	if (HunterDef)
	{
		GrantHunterAbilities(NewPlayer, HunterDef);
	}
	else
	{
		UE_LOG(LogBreachborne, Warning, TEXT("No HunterDefinition found for HunterID %d — no abilities granted"), DefaultHunterID);
	}
}

void ABreachborneGameMode::Logout(AController* Exiting)
{
	if (Exiting)
	{
		ABreachbornePlayerState* BBPS = Exiting->GetPlayerState<ABreachbornePlayerState>();
		if (BBGameState && BBPS)
		{
			BBGameState->ClearLobbyPlayer(BBPS);
			if (BBGameState->GetLobbyOwnerPlayerState() == BBPS)
			{
				BBGameState->SetLobbyOwner(nullptr);
			}
		}
		if (BBPS && BBPS->GetIsAlive())
		{
			BBPS->SetIsAlive(false);
			if (BBGameState)
			{
				BBGameState->UpdateTeamAliveCount(BBPS->GetTeamID(), -1);
			}
		}

		if (BBPS)
		{
			const TWeakObjectPtr<ABreachbornePlayerState> PlayerKey(BBPS);
			if (const TWeakObjectPtr<AHunterCharacter>* DownedHunter = DownedHunters.Find(PlayerKey))
			{
				if (DownedHunter->IsValid())
				{
					DownedHunter->Get()->Destroy();
				}
			}
			DownedHunters.Remove(PlayerKey);
		}
	}

	Super::Logout(Exiting);
	AssignLobbyOwnerIfNeeded();
}

void ABreachborneGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!BBGameState)
	{
		return;
	}

	if (BBGameState->GetMatchPhase() == EMatchPhase::Countdown)
	{
		if (!AreAllConnectedPlayersReady())
		{
			StartHunterSelect();
			return;
		}

		MatchCountdownRemaining = FMath::Max(0.0f, MatchCountdownRemaining - DeltaSeconds);
		BBGameState->SetPhaseTimeRemaining(MatchCountdownRemaining);
		if (MatchCountdownRemaining <= 0.0f)
		{
			StartGameplayMatch();
		}
	}
	else if (BBGameState->GetMatchPhase() == EMatchPhase::PostMatch)
	{
		PostMatchRemaining = FMath::Max(0.0f, PostMatchRemaining - DeltaSeconds);
		BBGameState->SetPhaseTimeRemaining(PostMatchRemaining);
		if (PostMatchRemaining <= 0.0f)
		{
			ResetToHunterSelect();
		}
	}
}

void ABreachborneGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (IsEditorQuickPlay())
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	}
}

void ABreachborneGameMode::SetMatchPhase(EMatchPhase NewPhase)
{
	if (BBGameState)
	{
		BBGameState->SetMatchPhase(NewPhase);
	}
}

void ABreachborneGameMode::RequestHunterSelection(APlayerController* PlayerController, int32 HunterID)
{
	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|RequestHunterSelection pc=%s hunter=%d phase=%d"),
		*GetNameSafe(PlayerController),
		HunterID,
		BBGameState ? static_cast<int32>(BBGameState->GetMatchPhase()) : -1);

	if (!PlayerController || !BBGameState)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|RequestHunterSelectionRejected reason=missing_pc_or_gs"));
		return;
	}

	const EMatchPhase Phase = BBGameState->GetMatchPhase();
	if (Phase != EMatchPhase::HunterSelect && Phase != EMatchPhase::Countdown)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|RequestHunterSelectionRejected reason=phase phase=%d"), static_cast<int32>(Phase));
		return;
	}

	ABreachbornePlayerState* BBPS = PlayerController->GetPlayerState<ABreachbornePlayerState>();
	if (BBPS && BBPS->IsLobbySpectator())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|RequestHunterSelectionRejected reason=spectator player=%s"), *BBPS->GetPlayerName());
		return;
	}
	const bool bHunterSelectable = IsHunterSelectable(HunterID);
	if (!BBPS || BBPS->IsReadyForMatch() || !bHunterSelectable)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|RequestHunterSelectionRejected reason=ps_ready_or_invalid ps=%s ready=%d hunterSelectable=%d"),
			*GetNameSafe(BBPS),
			BBPS && BBPS->IsReadyForMatch() ? 1 : 0,
			bHunterSelectable ? 1 : 0);
		return;
	}

	BBPS->SetHunterID(HunterID);
	BBPS->SetHunterLocked(false);
	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|HunterSelected player=%s hunter=%d"), *BBPS->GetPlayerName(), HunterID);
}

void ABreachborneGameMode::SetPlayerReady(APlayerController* PlayerController, bool bReady)
{
	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|SetPlayerReady pc=%s ready=%d phase=%d"),
		*GetNameSafe(PlayerController),
		bReady ? 1 : 0,
		BBGameState ? static_cast<int32>(BBGameState->GetMatchPhase()) : -1);

	if (!PlayerController || !BBGameState)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|SetPlayerReadyRejected reason=missing_pc_or_gs"));
		return;
	}

	const EMatchPhase Phase = BBGameState->GetMatchPhase();
	if (Phase != EMatchPhase::HunterSelect && Phase != EMatchPhase::Countdown)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|SetPlayerReadyRejected reason=phase phase=%d"), static_cast<int32>(Phase));
		return;
	}

	ABreachbornePlayerState* BBPS = PlayerController->GetPlayerState<ABreachbornePlayerState>();
	if (!BBPS)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|SetPlayerReadyRejected reason=missing_ps"));
		return;
	}
	if (BBPS->IsLobbySpectator())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|SetPlayerReadyRejected reason=spectator player=%s"), *BBPS->GetPlayerName());
		return;
	}

	if (bReady && !IsHunterSelectable(BBPS->GetHunterID()))
	{
		BBPS->SetHunterID(DefaultHunterID);
	}

	BBPS->SetReadyForMatch(bReady);
	BBPS->SetHunterLocked(bReady);
	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|ReadyState player=%s hunter=%d ready=%d selectable=%d allReady=%d"),
		*BBPS->GetPlayerName(),
		BBPS->GetHunterID(),
		bReady ? 1 : 0,
		IsHunterSelectable(BBPS->GetHunterID()) ? 1 : 0,
		AreAllConnectedPlayersReady() ? 1 : 0);

	if (bReady && AreAllConnectedPlayersReady())
	{
		BeginMatchCountdown();
	}
	else if (!bReady && Phase == EMatchPhase::Countdown)
	{
		StartHunterSelect();
	}
}

void ABreachborneGameMode::RequestJoinLobbySlot(APlayerController* PlayerController, int32 TeamID, int32 SlotIndex)
{
	ABreachbornePlayerState* PS = PlayerController ? PlayerController->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	const EMatchPhase Phase = BBGameState ? BBGameState->GetMatchPhase() : EMatchPhase::Ended;
	UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|JoinSlotRequest player=%s team=%d slot=%d phase=%d"),
		PS ? *PS->GetPlayerName() : TEXT("None"),
		TeamID,
		SlotIndex,
		static_cast<int32>(Phase));

	if (!BBGameState || !PS || Phase != EMatchPhase::WaitingForPlayers)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|JoinSlotRejected reason=invalid_state"));
		return;
	}

	if (BBGameState->SetLobbySlot(TeamID, SlotIndex, PS))
	{
		PS->SetReadyForMatch(false);
		PS->SetHunterLocked(false);
		UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|JoinSlotAccepted player=%s team=%d slot=%d"),
			*PS->GetPlayerName(),
			TeamID,
			SlotIndex);
	}
	else
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|JoinSlotRejected reason=slot_unavailable player=%s team=%d slot=%d"),
			*PS->GetPlayerName(),
			TeamID,
			SlotIndex);
	}
}

void ABreachborneGameMode::RequestJoinSpectators(APlayerController* PlayerController)
{
	ABreachbornePlayerState* PS = PlayerController ? PlayerController->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	if (!BBGameState || !PS || BBGameState->GetMatchPhase() != EMatchPhase::WaitingForPlayers)
	{
		return;
	}

	BBGameState->AddSpectator(PS);
	PS->SetReadyForMatch(false);
	PS->SetHunterLocked(false);
	if (BBGameState->GetLobbyOwnerPlayerState() == PS)
	{
		BBGameState->SetLobbyOwner(nullptr);
		AssignLobbyOwnerIfNeeded();
	}
	UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|JoinSpectators player=%s"), *PS->GetPlayerName());
}

void ABreachborneGameMode::RequestAutoFillLobbySlot(APlayerController* PlayerController)
{
	ABreachbornePlayerState* PS = PlayerController ? PlayerController->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	if (!BBGameState || !PS || BBGameState->GetMatchPhase() != EMatchPhase::WaitingForPlayers)
	{
		return;
	}

	TryAutoFillLobbySlot(PS);
}

void ABreachborneGameMode::RequestSetLobbyTeamSize(APlayerController* PlayerController, int32 TeamSize)
{
	if (!BBGameState || !IsLobbyOwner(PlayerController) || BBGameState->GetMatchPhase() != EMatchPhase::WaitingForPlayers)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|SetTeamSizeRejected pc=%s size=%d"), *GetNameSafe(PlayerController), TeamSize);
		return;
	}

	FBBLobbySettings Settings = BBGameState->GetLobbySettings();
	Settings.TeamSize = FMath::Clamp(TeamSize, 1, 4);
	Settings.MaxTeams = FMath::Max(1, FMath::CeilToInt(static_cast<float>(Settings.MaxPlayers) / static_cast<float>(Settings.TeamSize)));
	BBGameState->SetLobbySettings(Settings);

	SquadSize = Settings.TeamSize;
	MaxTeams = Settings.MaxTeams;
	BBGameState->InitTeams(MaxTeams);
	BBGameState->RebuildLobbyPreservingPlayers(GetOrderedLobbyPlayers());
	AssignLobbyOwnerIfNeeded();
	UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|SetTeamSize size=%d maxTeams=%d"), SquadSize, MaxTeams);
}

void ABreachborneGameMode::RequestSetLobbyDescription(APlayerController* PlayerController, const FString& Description)
{
	if (!BBGameState || !IsLobbyOwner(PlayerController) || BBGameState->GetMatchPhase() != EMatchPhase::WaitingForPlayers)
	{
		return;
	}

	FBBLobbySettings Settings = BBGameState->GetLobbySettings();
	Settings.Description = Description.Left(140);
	BBGameState->SetLobbySettings(Settings);
	UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|SetDescription owner=%s text=%s"), *GetNameSafe(PlayerController), *Settings.Description);
}

void ABreachborneGameMode::RequestSetStormShiftPreset(APlayerController* PlayerController, FName PresetID)
{
	if (!BBGameState || !IsLobbyOwner(PlayerController) || BBGameState->GetMatchPhase() != EMatchPhase::WaitingForPlayers)
	{
		return;
	}

	FBBLobbySettings Settings = BBGameState->GetLobbySettings();
	Settings.StormShiftPreset = PresetID.IsNone() ? FName(TEXT("Default")) : PresetID;
	BBGameState->SetLobbySettings(Settings);
	UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|SetStormShift owner=%s preset=%s"),
		*GetNameSafe(PlayerController),
		*Settings.StormShiftPreset.ToString());
}

void ABreachborneGameMode::RequestStartLobbyMatch(APlayerController* PlayerController)
{
	FString Reason;
	const bool bAbilitySmoke = FParse::Param(FCommandLine::Get(), TEXT("BBAbilitySmoke"));
	if (!BBGameState)
	{
		Reason = TEXT("missing_gamestate");
	}
	else if (BBGameState->GetMatchPhase() != EMatchPhase::WaitingForPlayers)
	{
		Reason = TEXT("wrong_phase");
	}
	else if (!bAbilitySmoke && !IsLobbyOwner(PlayerController))
	{
		Reason = TEXT("not_lobby_owner");
	}
	else if (!CanStartLobbyMatch(Reason))
	{
		// CanStartLobbyMatch supplies the rejection reason.
	}

	if (!Reason.IsEmpty() && Reason != TEXT("ok"))
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|StartRejected pc=%s reason=%s"),
			*GetNameSafe(PlayerController),
			*Reason);
		return;
	}

	UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|StartAccepted owner=%s activePlayers=%d"),
		*GetNameSafe(PlayerController),
		BBGameState->GetLobbyActivePlayerCount());
	StartHunterSelect();
}

bool ABreachborneGameMode::IsEditorQuickPlay() const
{
	const UWorld* World = GetWorld();
	const UBBNetworkDevSettings* Settings = GetDefault<UBBNetworkDevSettings>();

#if WITH_EDITOR
	return World
		&& World->WorldType == EWorldType::PIE
		&& World->GetNetMode() == NM_Standalone
		&& Settings
		&& !Settings->bUseFrontendFlowInEditor
		&& Settings->bAutoStartSinglePlayerPIE;
#else
	return false;
#endif
}

bool ABreachborneGameMode::ShouldUseDevAutoRespawn() const
{
	const UBBNetworkDevSettings* Settings = GetDefault<UBBNetworkDevSettings>();
	return bDevAutoRespawn && IsEditorQuickPlay() && Settings && Settings->bAllowDevRespawnInEditorOnly;
}

bool ABreachborneGameMode::IsFrontendMatchFlow() const
{
	return !IsEditorQuickPlay();
}

void ABreachborneGameMode::ApplyConfiguredMatchSettings()
{
	const UBBNetworkDevSettings* Settings = GetDefault<UBBNetworkDevSettings>();
	if (Settings)
	{
		SquadSize = FMath::Clamp(Settings->SquadSize, 1, 4);
		MaxTeams = FMath::Max(1, FMath::CeilToInt(static_cast<float>(FMath::Clamp(Settings->MaxPlayers, 1, 40)) / static_cast<float>(SquadSize)));
	}
}

void ABreachborneGameMode::InitializeLobbyState()
{
	if (!BBGameState)
	{
		return;
	}

	const UBBNetworkDevSettings* Settings = GetDefault<UBBNetworkDevSettings>();
	FBBLobbySettings LobbySettings;
	LobbySettings.TeamSize = FMath::Clamp(Settings ? Settings->SquadSize : SquadSize, 1, 4);
	LobbySettings.MaxPlayers = FMath::Clamp(Settings ? Settings->MaxPlayers : 40, 1, 40);
	LobbySettings.MaxTeams = FMath::Max(1, FMath::CeilToInt(static_cast<float>(LobbySettings.MaxPlayers) / static_cast<float>(LobbySettings.TeamSize)));
	LobbySettings.Description = TEXT("Custom Breach lobby");
	LobbySettings.StormShiftPreset = TEXT("Default");
	LobbySettings.bAllowSpectators = true;

	SquadSize = LobbySettings.TeamSize;
	MaxTeams = LobbySettings.MaxTeams;
	BBGameState->InitLobby(LobbySettings);
}

bool ABreachborneGameMode::IsLobbyOwner(const APlayerController* PlayerController) const
{
	if (!BBGameState || !PlayerController)
	{
		return false;
	}

	return BBGameState->GetLobbyOwnerPlayerState() == PlayerController->PlayerState;
}

void ABreachborneGameMode::AssignLobbyOwnerIfNeeded()
{
	if (!BBGameState || BBGameState->GetLobbyOwnerPlayerState())
	{
		return;
	}

	for (APlayerState* BasePS : BBGameState->PlayerArray)
	{
		ABreachbornePlayerState* PS = Cast<ABreachbornePlayerState>(BasePS);
		if (PS && !PS->IsLobbySpectator())
		{
			BBGameState->SetLobbyOwner(PS);
			UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|OwnerAssigned player=%s"), *PS->GetPlayerName());
			return;
		}
	}
}

bool ABreachborneGameMode::TryAutoFillLobbySlot(ABreachbornePlayerState* PlayerState)
{
	if (!BBGameState || !PlayerState)
	{
		return false;
	}

	const TArray<FBBLobbyTeam>& LobbyTeams = BBGameState->GetLobbyTeams();
	for (const FBBLobbyTeam& Team : LobbyTeams)
	{
		for (const FBBLobbySlot& Slot : Team.Slots)
		{
			if (Slot.SlotType == EBBLobbyPlayerSlotType::Open)
			{
				const bool bAssigned = BBGameState->SetLobbySlot(Team.TeamID, Slot.SlotIndex, PlayerState);
				UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|AutoFill player=%s team=%d slot=%d success=%d"),
					*PlayerState->GetPlayerName(),
					Team.TeamID,
					Slot.SlotIndex,
					bAssigned ? 1 : 0);
				return bAssigned;
			}
		}
	}

	if (BBGameState->GetLobbySettings().bAllowSpectators)
	{
		BBGameState->AddSpectator(PlayerState);
		UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|SERVER|AutoFillSpectator player=%s"), *PlayerState->GetPlayerName());
		return true;
	}

	PlayerState->SetTeamID(-1);
	PlayerState->SetLobbySlotIndex(-1);
	PlayerState->SetIsSpectator(false);
	return false;
}

TArray<ABreachbornePlayerState*> ABreachborneGameMode::GetOrderedLobbyPlayers() const
{
	TArray<ABreachbornePlayerState*> OrderedPlayers;
	if (!BBGameState)
	{
		return OrderedPlayers;
	}

	for (const FBBLobbyTeam& Team : BBGameState->GetLobbyTeams())
	{
		for (const FBBLobbySlot& Slot : Team.Slots)
		{
			if (ABreachbornePlayerState* PS = Cast<ABreachbornePlayerState>(Slot.PlayerState))
			{
				OrderedPlayers.Add(PS);
			}
		}
	}
	for (APlayerState* BasePS : BBGameState->PlayerArray)
	{
		ABreachbornePlayerState* PS = Cast<ABreachbornePlayerState>(BasePS);
		if (PS && !OrderedPlayers.Contains(PS) && !PS->IsLobbySpectator())
		{
			OrderedPlayers.Add(PS);
		}
	}
	return OrderedPlayers;
}

bool ABreachborneGameMode::CanStartLobbyMatch(FString& OutReason) const
{
	if (!BBGameState)
	{
		OutReason = TEXT("missing_gamestate");
		return false;
	}

	int32 NonEmptyTeams = 0;
	for (const FBBLobbyTeam& Team : BBGameState->GetLobbyTeams())
	{
		bool bHasPlayer = false;
		for (const FBBLobbySlot& Slot : Team.Slots)
		{
			if (Slot.SlotType == EBBLobbyPlayerSlotType::Player && Slot.PlayerState)
			{
				bHasPlayer = true;
				break;
			}
		}
		if (bHasPlayer)
		{
			++NonEmptyTeams;
		}
	}

	if (NonEmptyTeams < 2)
	{
		OutReason = FString::Printf(TEXT("need_at_least_2_teams current=%d"), NonEmptyTeams);
		return false;
	}

	for (APlayerState* BasePS : BBGameState->PlayerArray)
	{
		const ABreachbornePlayerState* PS = Cast<ABreachbornePlayerState>(BasePS);
		if (!PS || PS->IsLobbySpectator())
		{
			continue;
		}
		if (PS->GetTeamID() < 0 || PS->GetLobbySlotIndex() < 0)
		{
			OutReason = FString::Printf(TEXT("unassigned_player %s"), PS ? *PS->GetPlayerName() : TEXT("unknown"));
			return false;
		}
	}

	OutReason = TEXT("ok");
	return true;
}

int32 ABreachborneGameMode::AssignTeamForNewPlayer() const
{
	TArray<int32> TeamCounts;
	TeamCounts.Init(0, FMath::Max(1, MaxTeams));

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ABreachbornePlayerState> It(World); It; ++It)
		{
			const ABreachbornePlayerState* PS = *It;
			if (PS && TeamCounts.IsValidIndex(PS->GetTeamID()))
			{
				TeamCounts[PS->GetTeamID()]++;
			}
		}
	}

	for (int32 TeamID = 0; TeamID < TeamCounts.Num(); ++TeamID)
	{
		if (TeamCounts[TeamID] < SquadSize)
		{
			return TeamID;
		}
	}

	return FMath::Clamp(NextPlayerIndex / FMath::Max(1, SquadSize), 0, MaxTeams - 1);
}

bool ABreachborneGameMode::IsHunterSelectable(int32 HunterID) const
{
	return HunterID > 0 && ResolveHunterDefinition(HunterID) != nullptr;
}

bool ABreachborneGameMode::DebugValidateHunterDefinition(int32 HunterID) const
{
	const UBBHunterDefinition* HunterDef = ResolveHunterDefinition(HunterID);
	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|DEBUG|HunterDefinition hunter=%d valid=%d def=%s"),
		HunterID,
		HunterDef ? 1 : 0,
		*GetNameSafe(HunterDef));
	return HunterDef != nullptr;
}

const UBBHunterDefinition* ABreachborneGameMode::ResolveHunterDefinition(int32 HunterID) const
{
	const bool bAllowContentDefinition = HunterID != 4 || IsHudsonVisualSetReadyForContentDefinition();
	if (!bAllowContentDefinition)
	{
		static bool bLoggedHudsonNativeFallback = false;
		if (!bLoggedHudsonNativeFallback)
		{
			UE_LOG(LogBreachborne, Warning, TEXT("Hudson VIS_Hudson asset is missing; using native Hudson definition to avoid loading unrepaired DA_Hudson content."));
			bLoggedHudsonNativeFallback = true;
		}
	}

	if (bAllowContentDefinition)
	{
		switch (HunterID)
		{
		case 1:
			if (const UBBHunterDefinition* GhostDef = LoadHunterDefinitionAsset(TEXT("/Game/DA_Ghost.DA_Ghost")))
			{
				return GhostDef;
			}
			break;
		case 2:
			if (const UBBHunterDefinition* KingpinDef = LoadHunterDefinitionAsset(TEXT("/Game/Hunters/Kingpin/DA_Kingpin.DA_Kingpin")))
			{
				return KingpinDef;
			}
			break;
		case 3:
			if (const UBBHunterDefinition* ElunaDef = LoadHunterDefinitionAsset(TEXT("/Game/Hunters/Eluna/DA_Eluna.DA_Eluna")))
			{
				return ElunaDef;
			}
			break;
		case 4:
			if (const UBBHunterDefinition* HudsonDef = LoadHunterDefinitionAsset(TEXT("/Game/Hunters/Hudson/DA_Hudson.DA_Hudson")))
			{
				return HudsonDef;
			}
			break;
		case 5:
			if (const UBBHunterDefinition* CrystaDef = LoadHunterDefinitionAsset(TEXT("/Game/Hunters/Crysta/DA_Crysta.DA_Crysta")))
			{
				return CrystaDef;
			}
			break;
		case 6:
			if (const UBBHunterDefinition* VoidDef = LoadHunterDefinitionAsset(TEXT("/Game/Hunters/Void/DA_Void.DA_Void")))
			{
				return VoidDef;
			}
			break;
		default:
			break;
		}
	}

	if (const TObjectPtr<UBBHunterDefinition>* FoundNativeDef = NativeHunterDefinitions.Find(HunterID))
	{
		if (const UBBHunterDefinition* NativeDef = FoundNativeDef->Get())
		{
			return NativeDef;
		}
	}

	return nullptr;
}

void ABreachborneGameMode::BuildNativeHunterDefinitions()
{
	if (!NativeHunterDefinitions.Contains(1))
	{
		UBBHunterDefinition* GhostDef = NewObject<UBBHunterDefinition>(this, TEXT("Native_DA_Ghost"));
		GhostDef->HunterName = FText::FromString(TEXT("Ghost"));
		GhostDef->Role = EHunterRole::Fighter;
		GhostDef->BaseHealth = 500.0f;
		GhostDef->BaseAttackPower = 55.0f;
		GhostDef->BaseAbilityPower = 45.0f;
		GhostDef->BaseMoveSpeed = 600.0f;
		GhostDef->AbilitiesToGrant = {
			UGA_Ghost_LMB::StaticClass(),
			UGA_Ghost_RMB::StaticClass(),
			UGA_Ghost_Shift::StaticClass(),
			UGA_Ghost_Q::StaticClass(),
			UGA_Ghost_R::StaticClass(),
			UGA_Ghost_Passive::StaticClass()
		};

		NativeHunterDefinitions.Add(1, GhostDef);
	}

	if (!NativeHunterDefinitions.Contains(3))
	{
		UBBHunterDefinition* ElunaDef = NewObject<UBBHunterDefinition>(this, TEXT("Native_DA_Eluna"));
		ElunaDef->HunterName = FText::FromString(TEXT("Eluna"));
		ElunaDef->Role = EHunterRole::Protector;
		ElunaDef->BaseHealth = 525.0f;
		ElunaDef->BaseAttackPower = 45.0f;
		ElunaDef->BaseAbilityPower = 65.0f;
		ElunaDef->BaseMoveSpeed = 590.0f;
		ElunaDef->AbilitiesToGrant = {
			UGA_Eluna_LMB::StaticClass(),
			UGA_Eluna_RMB::StaticClass(),
			UGA_Eluna_AerialDash::StaticClass(),
			UGA_Eluna_GroundDash::StaticClass(),
			UGA_Eluna_Q::StaticClass(),
			UGA_Eluna_R::StaticClass(),
			UGA_Eluna_Passive::StaticClass()
		};

		NativeHunterDefinitions.Add(3, ElunaDef);
	}

	if (!NativeHunterDefinitions.Contains(2))
	{
		UBBHunterDefinition* KingpinDef = NewObject<UBBHunterDefinition>(this, TEXT("Native_DA_Kingpin"));
		KingpinDef->HunterName = FText::FromString(TEXT("Kingpin"));
		KingpinDef->Role = EHunterRole::Fighter;
		KingpinDef->BaseHealth = 650.0f;
		KingpinDef->BaseAttackPower = 55.0f;
		KingpinDef->BaseAbilityPower = 45.0f;
		KingpinDef->BaseMoveSpeed = 560.0f;
		KingpinDef->AbilitiesToGrant = {
			UGA_Kingpin_LMB::StaticClass(),
			UGA_Kingpin_RMB::StaticClass(),
			UGA_Kingpin_Shift::StaticClass(),
			UGA_Kingpin_Q::StaticClass(),
			UGA_Kingpin_R::StaticClass(),
			UGA_Kingpin_Passive::StaticClass()
		};

		NativeHunterDefinitions.Add(2, KingpinDef);
	}

	if (!NativeHunterDefinitions.Contains(4))
	{
		UBBHunterDefinition* HudsonDef = NewObject<UBBHunterDefinition>(this, TEXT("Native_DA_Hudson"));
		HudsonDef->HunterName = FText::FromString(TEXT("Hudson"));
		HudsonDef->Role = EHunterRole::Controller;
		HudsonDef->BaseHealth = 600.0f;
		HudsonDef->BaseAttackPower = 50.0f;
		HudsonDef->BaseAbilityPower = 50.0f;
		HudsonDef->BaseMoveSpeed = 575.0f;
		HudsonDef->AbilitiesToGrant = {
			UGA_Hudson_LMB_Minigun::StaticClass(),
			UGA_Hudson_RMB_SpinBarrels::StaticClass(),
			UGA_Hudson_Shift_HoverJets::StaticClass(),
			UGA_Hudson_Q_BarbedWire::StaticClass(),
			UGA_Hudson_R_SalvageHook::StaticClass(),
			UGA_Hudson_Passive_Salvager::StaticClass()
		};

		NativeHunterDefinitions.Add(4, HudsonDef);
	}

	if (!NativeHunterDefinitions.Contains(5))
	{
		UBBHunterDefinition* CrystaDef = NewObject<UBBHunterDefinition>(this, TEXT("Native_DA_Crysta"));
		CrystaDef->HunterName = FText::FromString(TEXT("Crysta"));
		CrystaDef->Role = EHunterRole::Fighter;
		CrystaDef->BaseHealth = 525.0f;
		CrystaDef->BaseAttackPower = 50.0f;
		CrystaDef->BaseAbilityPower = 60.0f;
		CrystaDef->BaseMoveSpeed = 600.0f;
		CrystaDef->AbilitiesToGrant = {
			UGA_Crysta_LMB::StaticClass(),
			UGA_Crysta_RMB::StaticClass(),
			UGA_Crysta_Shift_Primary::StaticClass(),
			UGA_Crysta_Shift_Secondary::StaticClass(),
			UGA_Crysta_Q::StaticClass(),
			UGA_Crysta_R::StaticClass(),
			UGA_Crysta_Passive::StaticClass()
		};

		NativeHunterDefinitions.Add(5, CrystaDef);
	}

	if (!NativeHunterDefinitions.Contains(6))
	{
		UBBHunterDefinition* VoidDef = NewObject<UBBHunterDefinition>(this, TEXT("Native_DA_Void"));
		VoidDef->HunterName = FText::FromString(TEXT("Void"));
		VoidDef->Role = EHunterRole::Controller;
		VoidDef->BaseHealth = 500.0f;
		VoidDef->BaseAttackPower = 45.0f;
		VoidDef->BaseAbilityPower = 65.0f;
		VoidDef->BaseMoveSpeed = 590.0f;
		VoidDef->AbilitiesToGrant = {
			UGA_Void_LMB::StaticClass(),
			UGA_Void_RMB::StaticClass(),
			UGA_Void_Shift::StaticClass(),
			UGA_Void_Q::StaticClass(),
			UGA_Void_R::StaticClass(),
			UGA_Void_Passive::StaticClass()
		};

		NativeHunterDefinitions.Add(6, VoidDef);
	}
}

bool ABreachborneGameMode::AreAllConnectedPlayersReady() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	int32 PlayerCount = 0;
	for (TActorIterator<ABreachbornePlayerState> It(World); It; ++It)
	{
		const ABreachbornePlayerState* PS = *It;
		if (!PS)
		{
			continue;
		}
		if (PS->IsLobbySpectator())
		{
			continue;
		}

		PlayerCount++;
		if (!PS->IsReadyForMatch() || !IsHunterSelectable(PS->GetHunterID()))
		{
			return false;
		}
	}

	return PlayerCount > 0;
}

void ABreachborneGameMode::StartHunterSelect()
{
	MatchCountdownRemaining = 0.0f;
	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|StartHunterSelect players=%d"), GetNumPlayers());
	if (BBGameState)
	{
		BBGameState->SetPhaseTimeRemaining(0.0f);
		BBGameState->SetMatchPhase(EMatchPhase::HunterSelect);
	}

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (ABreachbornePlayerController* BBPC = Cast<ABreachbornePlayerController>(It->Get()))
			{
				const ABreachbornePlayerState* PS = BBPC->GetPlayerState<ABreachbornePlayerState>();
				BBPC->ClientEnterFrontendPhase((PS && PS->IsLobbySpectator()) ? EMatchPhase::WaitingForPlayers : EMatchPhase::HunterSelect);
			}
		}
	}
}

void ABreachborneGameMode::BeginMatchCountdown()
{
	const UBBNetworkDevSettings* Settings = GetDefault<UBBNetworkDevSettings>();
	MatchCountdownRemaining = Settings ? Settings->HunterSelectCountdownSeconds : 5.0f;
	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|BeginMatchCountdown seconds=%.1f"), MatchCountdownRemaining);

	if (BBGameState)
	{
		BBGameState->SetPhaseTimeRemaining(MatchCountdownRemaining);
		BBGameState->SetMatchPhase(EMatchPhase::Countdown);
	}

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (ABreachbornePlayerController* BBPC = Cast<ABreachbornePlayerController>(It->Get()))
			{
				const ABreachbornePlayerState* PS = BBPC->GetPlayerState<ABreachbornePlayerState>();
				BBPC->ClientEnterFrontendPhase((PS && PS->IsLobbySpectator()) ? EMatchPhase::WaitingForPlayers : EMatchPhase::Countdown);
			}
		}
	}
}

void ABreachborneGameMode::StartGameplayMatch()
{
	if (!BBGameState || !GetWorld())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|StartGameplayMatchRejected reason=missing_gs_or_world"));
		return;
	}

	FString LobbyReason;
	if (!CanStartLobbyMatch(LobbyReason))
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|StartGameplayMatchRejected reason=%s"), *LobbyReason);
		StartHunterSelect();
		return;
	}

	if (!AreAllConnectedPlayersReady())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|StartGameplayMatchRejected reason=players_not_ready_or_invalid_hunter"));
		StartHunterSelect();
		return;
	}

	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|StartGameplayMatch players=%d"), GetNumPlayers());
	BBGameState->SetMatchPhase(EMatchPhase::Dropping);
	BBGameState->ResetMatchClock();
	BBGameState->InitTeams(MaxTeams);

	if (StormManager)
	{
		StormManager->ResetStorm();
		StormManager->StartStorm();
	}

	// Clear stale downed-state actors before spawning the new match's hunters.
	for (const TPair<TWeakObjectPtr<ABreachbornePlayerState>, TWeakObjectPtr<AHunterCharacter>>& Entry : DownedHunters)
	{
		if (Entry.Value.IsValid())
		{
			Entry.Value->Destroy();
		}
	}
	DownedHunters.Reset();

	for (TActorIterator<ABBWispPawn> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
	for (TActorIterator<ABBDeathboxActor> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (const ABreachbornePlayerState* PS = PC->GetPlayerState<ABreachbornePlayerState>(); PS && PS->IsLobbySpectator())
			{
				continue;
			}
			SpawnPlayerForMatch(PC);
			if (ABreachbornePlayerController* BBPC = Cast<ABreachbornePlayerController>(PC))
			{
				BBPC->ClientEnterGameplayPhase();
			}
		}
	}

	MatchStartAliveTeamCount = BBGameState->GetAliveTeamCount();
	UE_LOG(LogBreachborne, Warning, TEXT("BB_MATCH|SERVER|MatchStartAliveTeams count=%d players=%d squadSize=%d"),
		MatchStartAliveTeamCount,
		GetNumPlayers(),
		SquadSize);

	BBGameState->SetMatchPhase(EMatchPhase::Playing);
}

bool ABreachborneGameMode::ShouldEvaluateWinCondition() const
{
	return IsFrontendMatchFlow()
		&& BBGameState
		&& BBGameState->GetMatchPhase() == EMatchPhase::Playing
		&& MatchStartAliveTeamCount >= 2;
}

void ABreachborneGameMode::EnterPostMatch(int32 WinningTeamID)
{
	if (!BBGameState || BBGameState->GetMatchPhase() == EMatchPhase::PostMatch)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|EnterPostMatchRejected winner=%d"), WinningTeamID);
		return;
	}

	const UBBNetworkDevSettings* Settings = GetDefault<UBBNetworkDevSettings>();
	PostMatchRemaining = Settings ? Settings->PostMatchReturnSeconds : 10.0f;
	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|EnterPostMatch winner=%d seconds=%.1f"), WinningTeamID, PostMatchRemaining);
	BBGameState->SetWinningTeamID(WinningTeamID);
	BBGameState->SetPhaseTimeRemaining(PostMatchRemaining);
	BBGameState->SetMatchPhase(EMatchPhase::PostMatch);

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (ABreachbornePlayerController* BBPC = Cast<ABreachbornePlayerController>(It->Get()))
			{
				BBPC->ClientEnterFrontendPhase(EMatchPhase::PostMatch);
			}
		}
	}
}

void ABreachborneGameMode::ResetToHunterSelect()
{
	if (!BBGameState || !GetWorld())
	{
		return;
	}

	BBGameState->SetMatchPhase(EMatchPhase::Resetting);
	BBGameState->InitTeams(MaxTeams);
	BBGameState->ResetMatchClock();
	MatchStartAliveTeamCount = 0;

	if (StormManager)
	{
		StormManager->ResetStorm();
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		if (APawn* Pawn = PC->GetPawn())
		{
			PC->UnPossess();
			Pawn->Destroy();
		}

		ClearHunterAbilities(PC);

		if (ABreachbornePlayerState* PS = PC->GetPlayerState<ABreachbornePlayerState>())
		{
			PS->SetIsAlive(false);
			PS->SetReadyForMatch(false);
			PS->SetHunterLocked(false);
		}
	}

	for (const TPair<TWeakObjectPtr<ABreachbornePlayerState>, TWeakObjectPtr<AHunterCharacter>>& Entry : DownedHunters)
	{
		if (Entry.Value.IsValid())
		{
			Entry.Value->Destroy();
		}
	}
	DownedHunters.Reset();
	for (TActorIterator<ABBWispPawn> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
	for (TActorIterator<ABBDeathboxActor> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

	BBGameState->SetMatchPhase(EMatchPhase::WaitingForPlayers);
	AssignLobbyOwnerIfNeeded();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ABreachbornePlayerController* BBPC = Cast<ABreachbornePlayerController>(It->Get()))
		{
			BBPC->ClientEnterFrontendPhase(EMatchPhase::WaitingForPlayers);
		}
	}
}

void ABreachborneGameMode::SpawnPlayerForMatch(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|SpawnPlayerForMatchRejected reason=missing_pc"));
		return;
	}

	ABreachbornePlayerState* PS = PlayerController->GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|SpawnPlayerForMatchRejected reason=missing_ps pc=%s"), *GetNameSafe(PlayerController));
		return;
	}

	if (PS->IsLobbySpectator() || PS->GetTeamID() < 0 || PS->GetLobbySlotIndex() < 0)
	{
		PS->SetIsAlive(false);
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|SpawnPlayerForMatchRejected player=%s reason=invalid_team_or_spectator team=%d slot=%d spectator=%d"),
			*PS->GetPlayerName(),
			PS->GetTeamID(),
			PS->GetLobbySlotIndex(),
			PS->IsLobbySpectator() ? 1 : 0);
		return;
	}

	if (!PS->IsReadyForMatch() || !IsHunterSelectable(PS->GetHunterID()))
	{
		PS->SetIsAlive(false);
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|SpawnPlayerForMatchRejected player=%s reason=not_ready_or_invalid_hunter ready=%d hunter=%d"),
			*PS->GetPlayerName(),
			PS->IsReadyForMatch() ? 1 : 0,
			PS->GetHunterID());
		return;
	}

	const UBBHunterDefinition* HunterDef = ResolveHunterDefinition(PS->GetHunterID());
	if (!HunterDef)
	{
		PS->SetIsAlive(false);
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|SpawnPlayerForMatchRejected player=%s reason=missing_hunter_definition hunter=%d"),
			*PS->GetPlayerName(),
			PS->GetHunterID());
		return;
	}

	ClearHunterAbilities(PlayerController);
	const FTransform DropTransform = GetDropTransformForPlayer(PS);
	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|SpawnPlayerForMatch pc=%s player=%s team=%d hunter=%d loc=%s"),
		*GetNameSafe(PlayerController),
		*PS->GetPlayerName(),
		PS->GetTeamID(),
		PS->GetHunterID(),
		*DropTransform.GetLocation().ToCompactString());
	UE_LOG(LogBreachborne, Warning, TEXT("BB_DROP|SERVER|SpawnPlayer pc=%s player=%s team=%d hunter=%d loc=%s rot=%s"),
		*GetNameSafe(PlayerController),
		*PS->GetPlayerName(),
		PS->GetTeamID(),
		PS->GetHunterID(),
		*DropTransform.GetLocation().ToCompactString(),
		*DropTransform.GetRotation().Rotator().ToCompactString());
	RestartPlayerAtTransform(PlayerController, DropTransform);

	if (!PlayerController->GetPawn())
	{
		PS->SetIsAlive(false);
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|SERVER|SpawnPlayerForMatchRejected player=%s reason=spawn_failed"), *PS->GetPlayerName());
		return;
	}

	if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(PlayerController->GetPawn()))
	{
		Hunter->ApplyHunterDefinitionVisuals(HunterDef);
	}

	GrantHunterAbilities(PlayerController, HunterDef);

	PS->SetIsAlive(true);
	PS->SetHunterLocked(true);
	PS->SetReadyForMatch(false);

	if (BBGameState)
	{
		BBGameState->UpdateTeamAliveCount(PS->GetTeamID(), 1);
	}

	if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(PlayerController->GetPawn()))
	{
		if (UGliderComponent* Glider = Hunter->GetGliderComponent())
		{
			Glider->ForceOpenFreeDropGlider();
		}
	}
}

FTransform ABreachborneGameMode::GetDropTransformForPlayer(const ABreachbornePlayerState* PlayerState) const
{
	const int32 TeamID = PlayerState ? FMath::Max(0, PlayerState->GetTeamID()) : 0;
	const int32 SlotIndex = PlayerState ? FMath::Max(0, PlayerState->GetLobbySlotIndex()) : 0;

	if (UWorld* World = GetWorld())
	{
		TArray<ABBTeamDropSpawn*> DropSpawns;
		for (TActorIterator<ABBTeamDropSpawn> It(World); It; ++It)
		{
			if (*It)
			{
				DropSpawns.Add(*It);
			}
		}
		DropSpawns.Sort([](const ABBTeamDropSpawn& A, const ABBTeamDropSpawn& B)
		{
			return A.GetName() < B.GetName();
		});

		if (DropSpawns.IsValidIndex(TeamID))
		{
			return DropSpawns[TeamID]->GetSpawnTransformForSlot(SlotIndex, SquadSize);
		}
	}

	const FVector Center = StormManager ? StormManager->GetActorLocation() : FVector::ZeroVector;
	const float Angle = (static_cast<float>(TeamID) / FMath::Max(1.0f, static_cast<float>(MaxTeams))) * 2.0f * PI;
	const FVector RingOffset(FMath::Cos(Angle) * 7000.0f, FMath::Sin(Angle) * 7000.0f, 2200.0f);
	const FVector SlotOffset(-FMath::Sin(Angle) * SlotIndex * 180.0f, FMath::Cos(Angle) * SlotIndex * 180.0f, 0.0f);
	const FVector SpawnLocation = Center + RingOffset + SlotOffset;
	const FRotator SpawnRotation = (Center - SpawnLocation).Rotation();
	return FTransform(SpawnRotation, SpawnLocation);
}

void ABreachborneGameMode::ClearHunterAbilities(APlayerController* PlayerController)
{
	ABreachbornePlayerState* PS = PlayerController ? PlayerController->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	UBBAbilitySystemComponent* ASC = PS ? PS->GetBBAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> HandlesToClear;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.SourceObject == PS)
		{
			HandlesToClear.Add(Spec.Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& Handle : HandlesToClear)
	{
		ASC->ClearAbility(Handle);
	}
}

void ABreachborneGameMode::GrantHunterAbilities(APlayerController* PlayerController, const UBBHunterDefinition* HunterDef)
{
	if (!PlayerController || !HunterDef)
	{
		return;
	}

	ABreachbornePlayerState* BBPS = PlayerController->GetPlayerState<ABreachbornePlayerState>();
	if (!BBPS)
	{
		return;
	}

	UBBAbilitySystemComponent* ASC = BBPS->GetBBAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(PlayerController->GetPawn()))
	{
		Hunter->ApplyHunterDefinitionVisuals(HunterDef);
	}

	// Apply base stats from HunterDefinition to attribute sets
	if (UBBHealthSet* HealthSet = BBPS->GetHealthSet())
	{
		HealthSet->InitMaxHealth(HunterDef->BaseHealth);
		HealthSet->InitHealth(HunterDef->BaseHealth);
	}

	if (UBBCombatSet* CombatSet = BBPS->GetCombatSet())
	{
		CombatSet->InitAttackPower(HunterDef->BaseAttackPower);
		CombatSet->InitAbilityPower(HunterDef->BaseAbilityPower);
	}

	if (UBBMovementSet* MovementSet = BBPS->GetMovementSet())
	{
		MovementSet->InitMoveSpeed(HunterDef->BaseMoveSpeed);
	}

	// Grant each ability from the definition
	for (const TSubclassOf<UBBGameplayAbility>& AbilityClass : HunterDef->AbilitiesToGrant)
	{
		if (AbilityClass)
		{
			FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, BBPS);
			FGameplayAbilitySpecHandle GrantedHandle = ASC->GiveAbility(Spec);

			// Auto-activate ServerOnly abilities (passives) immediately after grant
			const UBBGameplayAbility* AbilityCDO = AbilityClass.GetDefaultObject();
			if (AbilityCDO && AbilityCDO->GetBBNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::ServerOnly
				&& !AbilityCDO->GetAbilityInputTag().IsValid())
			{
				ASC->TryActivateAbility(GrantedHandle);
			}

			UE_LOG(LogBreachborne, Log, TEXT("Granted ability %s to %s"), *AbilityClass->GetName(), *BBPS->GetPlayerName());
		}
	}

	// Bind to HealthSet's OnHealthDepleted for kill tracking
	if (UBBHealthSet* HealthSet = BBPS->GetHealthSet())
	{
		HealthSet->OnHealthDepleted.RemoveDynamic(this, &ABreachborneGameMode::OnHunterKilled);
		HealthSet->OnHealthDepleted.AddDynamic(this, &ABreachborneGameMode::OnHunterKilled);
	}

	BBPS->UpdateHealthProxy();
}

void ABreachborneGameMode::OnHunterKilled(UAbilitySystemComponent* VictimASC, UAbilitySystemComponent* KillerASC)
{
	if (!VictimASC)
	{
		return;
	}

	// --- Kill tracking (if there's a killer) ---
	ABreachbornePlayerState* KillerPS = KillerASC ? Cast<ABreachbornePlayerState>(KillerASC->GetOwnerActor()) : nullptr;
	ABreachbornePlayerState* VictimPS = Cast<ABreachbornePlayerState>(VictimASC->GetOwnerActor());

	if (KillerPS)
	{
		KillerPS->AddKill();

		// Dispatch Event_Kill gameplay event to the killer's ASC
		// This is the generic event that passives (like Ghost's kill-resets-shift) listen for
		FGameplayEventData EventData;
		EventData.Instigator = KillerASC->GetOwnerActor();
		EventData.Target = VictimASC->GetAvatarActor();
		KillerASC->HandleGameplayEvent(BBGameplayTags::Event_Kill, &EventData);

		// --- PvP Kill Economy: Gold transfer + shard drops ---
		if (VictimPS)
		{
			// Transfer 50% of victim's gold to killer
			const int32 VictimGold = VictimPS->GetInventoryData().Gold;
			const int32 GoldTransfer = VictimGold / 2;
			if (GoldTransfer > 0)
			{
				FBBInventoryManager::AddGold(KillerPS, GoldTransfer);
				FBBInventoryManager::AddGold(VictimPS, -GoldTransfer);
				UE_LOG(LogBreachborne, Log, TEXT("    Gold transfer: %d gold from %s to %s"),
					GoldTransfer, *VictimPS->GetPlayerName(), *KillerPS->GetPlayerName());
			}

			// Spawn shard pickups at death location
			AHunterCharacter* VictimHunterForLoot = Cast<AHunterCharacter>(VictimASC->GetAvatarActor());
			if (VictimHunterForLoot && GetWorld())
			{
				const FVector DeathLocation = VictimHunterForLoot->GetActorLocation();
				const int32 ShardDropCount = 3;
				FBBWorldItemSpawner::SpawnShardPickup(GetWorld(), DeathLocation + FVector(0, 0, 50), ShardDropCount);
			}
		}
	}

	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT("=== HUNTER DEATH: %s killed by %s ==="),
		VictimPS ? *VictimPS->GetPlayerName() : TEXT("Unknown"),
		KillerPS ? *KillerPS->GetPlayerName() : TEXT("Environment"));

	// --- Disable victim pawn ---
	AHunterCharacter* VictimHunter = Cast<AHunterCharacter>(VictimASC->GetAvatarActor());
	if (VictimHunter)
	{
		VictimASC->CancelAllAbilities();
		VictimHunter->SetActorHiddenInGame(true);
		VictimHunter->SetActorEnableCollision(false);
		VictimHunter->GetCharacterMovement()->DisableMovement();
		VictimHunter->GetCharacterMovement()->StopMovementImmediately();
	}

	// --- Update PlayerState ---
	if (VictimPS)
	{
		VictimPS->SetIsAlive(false);

		if (BBGameState)
		{
			BBGameState->UpdateTeamAliveCount(VictimPS->GetTeamID(), -1);
		}
	}

	const bool bUseDevAutoRespawn = ShouldUseDevAutoRespawn();
	if (!bUseDevAutoRespawn && VictimPS && VictimHunter)
	{
		DownedHunters.Add(VictimPS, VictimHunter);

		const bool bEnteredWisp = KillerASC && EnterWispState(VictimPS, VictimHunter, KillerPS);
		if (!bEnteredWisp)
		{
			if (AController* VictimController = VictimHunter->GetController())
			{
				VictimController->UnPossess();
			}
			SpawnDeathbox(VictimPS, VictimHunter->GetActorLocation());
		}
	}

	// --- Dev auto-respawn ---
	if (bUseDevAutoRespawn)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("    Dev respawn in %.1fs..."), DevRespawnDelay);

		FTimerHandle RespawnTimerHandle;
		GetWorldTimerManager().SetTimer(RespawnTimerHandle, [this, WeakVictimASC = TWeakObjectPtr<UAbilitySystemComponent>(VictimASC)]()
		{
			if (UAbilitySystemComponent* ASC = WeakVictimASC.Get())
			{
				DevRespawnHunter(ASC);
			}
		}, DevRespawnDelay, false);
	}
	else if (ShouldEvaluateWinCondition() && BBGameState->GetAliveTeamCount() <= 1)
	{
		int32 WinningTeamID = -1;
		for (const FTeamInfo& Team : BBGameState->GetTeams())
		{
			if (Team.AliveCount > 0)
			{
				WinningTeamID = Team.TeamID;
				break;
			}
		}
		EnterPostMatch(WinningTeamID);
	}
	else if (IsFrontendMatchFlow() && BBGameState && BBGameState->GetMatchPhase() == EMatchPhase::Playing)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_MATCH|SERVER|WinCheckSkipped aliveTeams=%d matchStartAliveTeams=%d"),
			BBGameState->GetAliveTeamCount(),
			MatchStartAliveTeamCount);
	}
}

bool ABreachborneGameMode::EnterWispState(ABreachbornePlayerState* VictimPS,
	AHunterCharacter* VictimHunter, ABreachbornePlayerState* KnockerPS)
{
	if (!HasAuthority() || !VictimPS || !VictimHunter || !GetWorld() || !WispPawnClass)
	{
		return false;
	}

	AController* VictimController = VictimHunter->GetController();
	if (!VictimController)
	{
		UE_LOG(LogBreachborne, Error, TEXT("WispSpawn: missing controller for victim=%s"),
			*VictimPS->GetPlayerName());
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = VictimController;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector SpawnLocation = VictimHunter->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
	ABBWispPawn* Wisp = GetWorld()->SpawnActor<ABBWispPawn>(
		WispPawnClass, SpawnLocation, VictimHunter->GetActorRotation(), SpawnParams);
	if (!Wisp)
	{
		UE_LOG(LogBreachborne, Error, TEXT("WispSpawn: failed for victim=%s at %s"),
			*VictimPS->GetPlayerName(), *SpawnLocation.ToCompactString());
		return false;
	}

	Wisp->KnockerPlayerState = KnockerPS;
	Wisp->PreWispGroundedLocation = VictimHunter->GetActorLocation();
	Wisp->OnWispReviveReady.AddDynamic(this, &ABreachborneGameMode::HandleWispRevive);
	Wisp->OnWispDied.AddDynamic(this, &ABreachborneGameMode::HandleWispDied);
	Wisp->OnWispExecuted.AddDynamic(this, &ABreachborneGameMode::HandleWispExecuted);
	Wisp->InitWisp(VictimPS);

	if (UBBAbilitySystemComponent* ASC = VictimPS->GetBBAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(BBGameplayTags::State_Wisp);
	}
	VictimController->Possess(Wisp);
	if (VictimController->GetPawn() != Wisp)
	{
		UE_LOG(LogBreachborne, Error, TEXT("WispSpawn: possession failed for victim=%s controller=%s"),
			*VictimPS->GetPlayerName(), *VictimController->GetName());
		Wisp->Destroy();
		return false;
	}

	UE_LOG(LogBreachborne, Warning,
		TEXT("WispSpawn: victim=%s team=%d wisp=%s location=%s controller=%s"),
		*VictimPS->GetPlayerName(), VictimPS->GetTeamID(), *Wisp->GetName(),
		*SpawnLocation.ToCompactString(), *VictimController->GetName());
	return true;
}

bool ABreachborneGameMode::ReviveDownedHunter(ABreachbornePlayerState* VictimPS,
	const FVector& ReviveLocation, float HealthFraction)
{
	if (!HasAuthority() || !VictimPS)
	{
		return false;
	}

	const TWeakObjectPtr<ABreachbornePlayerState> PlayerKey(VictimPS);
	TWeakObjectPtr<AHunterCharacter>* HunterEntry = DownedHunters.Find(PlayerKey);
	AHunterCharacter* Hunter = HunterEntry ? HunterEntry->Get() : nullptr;
	APlayerController* PlayerController = Cast<APlayerController>(VictimPS->GetOwner());
	UBBAbilitySystemComponent* ASC = VictimPS->GetBBAbilitySystemComponent();
	UBBHealthSet* HealthSet = VictimPS->GetHealthSet();
	if (!Hunter || !PlayerController || !ASC || !HealthSet)
	{
		UE_LOG(LogBreachborne, Error,
			TEXT("WispRevive: invalid state victim=%s hunter=%s controller=%s asc=%s health=%s"),
			*VictimPS->GetPlayerName(), *GetNameSafe(Hunter), *GetNameSafe(PlayerController),
			*GetNameSafe(ASC), *GetNameSafe(HealthSet));
		return false;
	}

	const FVector SafeReviveLocation = ReviveLocation + FVector(0.0f, 0.0f, 50.0f);
	Hunter->SetActorLocation(SafeReviveLocation, false, nullptr, ETeleportType::TeleportPhysics);
	Hunter->SetActorHiddenInGame(false);
	Hunter->SetActorEnableCollision(true);
	Hunter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Hunter->GetCharacterMovement()->StopMovementImmediately();

	ASC->RemoveLooseGameplayTag(BBGameplayTags::State_Wisp);
	ASC->RemoveLooseGameplayTag(BBGameplayTags::State_Dead);
	HealthSet->SetHealth(FMath::Max(1.0f, HealthSet->GetMaxHealth() * FMath::Clamp(HealthFraction, 0.01f, 1.0f)));
	HealthSet->SetShield(0.0f);
	VictimPS->SetIsAlive(true);
	VictimPS->UpdateHealthProxy();

	PlayerController->Possess(Hunter);
	if (BBGameState)
	{
		BBGameState->UpdateTeamAliveCount(VictimPS->GetTeamID(), 1);
	}
	DownedHunters.Remove(PlayerKey);

	UE_LOG(LogBreachborne, Warning, TEXT("WispRevive: player=%s health=%.0f location=%s"),
		*VictimPS->GetPlayerName(), HealthSet->GetHealth(), *SafeReviveLocation.ToCompactString());
	return true;
}

ABBDeathboxActor* ABreachborneGameMode::SpawnDeathbox(ABreachbornePlayerState* VictimPS,
	const FVector& Location)
{
	if (!HasAuthority() || !VictimPS || !GetWorld() || !DeathboxClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABBDeathboxActor* Deathbox = GetWorld()->SpawnActor<ABBDeathboxActor>(
		DeathboxClass, Location + FVector(0.0f, 0.0f, 25.0f), FRotator::ZeroRotator, SpawnParams);
	if (!Deathbox)
	{
		UE_LOG(LogBreachborne, Error, TEXT("DeathboxSpawn: failed for victim=%s"),
			*VictimPS->GetPlayerName());
		return nullptr;
	}

	Deathbox->InitDeathbox(VictimPS, VictimPS->GetInventoryData());
	Deathbox->OnReviveComplete.AddDynamic(this, &ABreachborneGameMode::HandleDeathboxReviveComplete);
	UE_LOG(LogBreachborne, Warning, TEXT("DeathboxSpawn: victim=%s deathbox=%s location=%s"),
		*VictimPS->GetPlayerName(), *Deathbox->GetName(), *Deathbox->GetActorLocation().ToCompactString());
	return Deathbox;
}

void ABreachborneGameMode::HandleWispRevive(ABBWispPawn* Wisp)
{
	if (!Wisp || !HasAuthority())
	{
		return;
	}

	ABreachbornePlayerState* VictimPS = Wisp->GetOwningPlayerState();
	if (ReviveDownedHunter(VictimPS, Wisp->GetActorLocation(), WispReviveHealthFraction))
	{
		Wisp->OnWispReviveReady.RemoveDynamic(this, &ABreachborneGameMode::HandleWispRevive);
		Wisp->OnWispDied.RemoveDynamic(this, &ABreachborneGameMode::HandleWispDied);
		Wisp->OnWispExecuted.RemoveDynamic(this, &ABreachborneGameMode::HandleWispExecuted);
		Wisp->Destroy();
	}
}

void ABreachborneGameMode::HandleWispDied(ABBWispPawn* Wisp, bool bWasExecuted)
{
	if (!Wisp || !HasAuthority())
	{
		return;
	}

	ABreachbornePlayerState* VictimPS = Wisp->GetOwningPlayerState();
	const FVector DeathLocation = Wisp->GetActorLocation();
	if (AController* WispController = Wisp->GetController())
	{
		WispController->UnPossess();
	}
	if (VictimPS)
	{
		if (UBBAbilitySystemComponent* ASC = VictimPS->GetBBAbilitySystemComponent())
		{
			ASC->RemoveLooseGameplayTag(BBGameplayTags::State_Wisp);
		}
		SpawnDeathbox(VictimPS, DeathLocation);
	}

	UE_LOG(LogBreachborne, Warning, TEXT("WispDeath: victim=%s executed=%d location=%s"),
		VictimPS ? *VictimPS->GetPlayerName() : TEXT("Unknown"), bWasExecuted ? 1 : 0,
		*DeathLocation.ToCompactString());
	Wisp->Destroy();
}

void ABreachborneGameMode::HandleWispExecuted(ABBWispPawn* Wisp, AHunterCharacter* Executor)
{
	if (!Wisp || !HasAuthority())
	{
		return;
	}

	if (Executor)
	{
		if (ABreachbornePlayerState* ExecutorPS = Executor->GetPlayerState<ABreachbornePlayerState>())
		{
			if (UBBHealthSet* ExecutorHealth = ExecutorPS->GetHealthSet())
			{
				ExecutorHealth->SetHealth(ExecutorHealth->GetMaxHealth());
				ExecutorPS->UpdateHealthProxy();
			}
		}
	}
	HandleWispDied(Wisp, true);
}

void ABreachborneGameMode::HandleDeathboxReviveComplete(ABBDeathboxActor* Deathbox,
	AHunterCharacter* Reviver)
{
	(void)Reviver;
	if (!Deathbox || !HasAuthority())
	{
		return;
	}

	if (ReviveDownedHunter(Deathbox->GetVictimPlayerState(), Deathbox->GetActorLocation(),
		Deathbox->GetReviveHealthFraction()))
	{
		Deathbox->OnReviveComplete.RemoveDynamic(this, &ABreachborneGameMode::HandleDeathboxReviveComplete);
		Deathbox->Destroy();
	}
}

void ABreachborneGameMode::HandleTrainDeath(AHunterCharacter* Hunter, const FVector& Location)
{
	if (!Hunter || !HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* VictimASC = Hunter->GetAbilitySystemComponent();
	ABreachbornePlayerState* VictimPS = Hunter->GetPlayerState<ABreachbornePlayerState>();
	if (!VictimASC || (VictimPS && !VictimPS->GetIsAlive()))
	{
		UE_LOG(LogBreachborne, Warning, TEXT("TrainKillDebug: death ignored hunter=%s asc=%s alive=%d loc=%s"),
			*GetNameSafe(Hunter),
			*GetNameSafe(VictimASC),
			VictimPS ? (VictimPS->GetIsAlive() ? 1 : 0) : -1,
			*Location.ToCompactString());
		return;
	}

	if (UBBHealthSet* HealthSet = VictimPS ? VictimPS->GetHealthSet() : nullptr)
	{
		HealthSet->SetHealth(0.0f);
	}

	VictimASC->AddLooseGameplayTag(BBGameplayTags::State_Dead);
	UE_LOG(LogBreachborne, Warning, TEXT("TrainKillDebug: applying train death hunter=%s loc=%s"),
		*GetNameSafe(Hunter), *Location.ToCompactString());

	OnHunterKilled(VictimASC, nullptr);
}

void ABreachborneGameMode::DevRespawnHunter(UAbilitySystemComponent* VictimASC)
{
	if (!VictimASC)
	{
		return;
	}

	ABreachbornePlayerState* VictimPS = Cast<ABreachbornePlayerState>(VictimASC->GetOwnerActor());
	if (!VictimPS)
	{
		return;
	}

	// Reset health and shield to max
	if (UBBHealthSet* HealthSet = VictimPS->GetHealthSet())
	{
		HealthSet->SetHealth(HealthSet->GetMaxHealth());
		HealthSet->SetShield(HealthSet->GetMaxShield());
	}

	// Remove dead tag
	VictimASC->RemoveLooseGameplayTag(BBGameplayTags::State_Dead);

	// Re-enable pawn
	AHunterCharacter* VictimHunter = Cast<AHunterCharacter>(VictimASC->GetAvatarActor());
	if (VictimHunter)
	{
		VictimHunter->SetActorHiddenInGame(false);
		VictimHunter->SetActorEnableCollision(true);
		VictimHunter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);

		// Teleport to a spawn point
		AActor* SpawnPoint = FindPlayerStart(VictimHunter->GetController());
		if (SpawnPoint)
		{
			VictimHunter->SetActorLocationAndRotation(
				SpawnPoint->GetActorLocation(),
				SpawnPoint->GetActorRotation());
		}
	}

	// Update PlayerState
	VictimPS->SetIsAlive(true);
	if (BBGameState)
	{
		BBGameState->UpdateTeamAliveCount(VictimPS->GetTeamID(), 1);
	}

	// Push new health values to clients
	VictimPS->UpdateHealthProxy();

	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT("=== DEV RESPAWN: %s ==="), *VictimPS->GetPlayerName());
}
