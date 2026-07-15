#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BreachborneTypes.generated.h"

/** Hunter role — determines playstyle and team composition */
UENUM(BlueprintType)
enum class EHunterRole : uint8
{
	Fighter			UMETA(DisplayName = "Fighter"),
	Initiator		UMETA(DisplayName = "Initiator"),
	Frontliner		UMETA(DisplayName = "Frontliner"),
	Protector		UMETA(DisplayName = "Protector"),
	Controller		UMETA(DisplayName = "Controller")
};

/** Current phase of the match */
UENUM(BlueprintType)
enum class EMatchPhase : uint8
{
	WaitingForPlayers	UMETA(DisplayName = "Waiting For Players"),
	HunterSelect		UMETA(DisplayName = "Hunter Select"),
	Countdown			UMETA(DisplayName = "Countdown"),
	Dropping			UMETA(DisplayName = "Dropping"),
	Playing				UMETA(DisplayName = "Playing"),
	PostMatch			UMETA(DisplayName = "Post Match"),
	Resetting			UMETA(DisplayName = "Resetting"),

	/** Deprecated alias kept for older Blueprints/data that referenced Ended. */
	Ended				UMETA(Hidden)
};

/** Frontend flow state for native UMG screens and GameInstance routing. */
UENUM(BlueprintType)
enum class EBBFrontendState : uint8
{
	Boot				UMETA(DisplayName = "Boot"),
	MainMenu			UMETA(DisplayName = "Main Menu"),
	Connecting			UMETA(DisplayName = "Connecting"),
	Lobby				UMETA(DisplayName = "Lobby"),
	HunterSelect		UMETA(DisplayName = "Hunter Select"),
	LoadingMatch		UMETA(DisplayName = "Loading Match"),
	InMatch				UMETA(DisplayName = "In Match"),
	FullscreenMap		UMETA(DisplayName = "Fullscreen Map"),
	PostMatch			UMETA(DisplayName = "Post Match"),
	ReturningToLobby	UMETA(DisplayName = "Returning To Lobby")
};

/** Glider state — server-authoritative, replicated to all clients */
UENUM(BlueprintType)
enum class EGliderState : uint8
{
	Closed		UMETA(DisplayName = "Closed"),		// Default — grounded or falling normally
	Open		UMETA(DisplayName = "Open"),		// Gliding — heat building, vulnerable to spiking
	Spiked		UMETA(DisplayName = "Spiked")		// Stunned from damage while gliding — falling fast, no input
};

/** Custom movement mode identifiers for UBBCharacterMovementComponent */
UENUM(BlueprintType)
enum class EBBCustomMovementMode : uint8
{
	None = 0,
	Gliding = 1,
	Spiked = 2,
	Mantling = 3
};

/** Player life state */
UENUM(BlueprintType)
enum class EPlayerLifeState : uint8
{
	Alive = 0,
	Wisp = 1,
	Dead = 2
};

/** Day/night cycle phase */
UENUM(BlueprintType)
enum class EBBDayNightPhase : uint8
{
	Day = 0,
	Night = 1
};

/** Replicated per-team info */
USTRUCT(BlueprintType)
struct FTeamInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Team")
	int32 TeamID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Team")
	int32 AliveCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Team")
	int32 KillCount = 0;
};

UENUM(BlueprintType)
enum class EBBLobbyPlayerSlotType : uint8
{
	Open		UMETA(DisplayName = "Open"),
	Player		UMETA(DisplayName = "Player"),
	Locked		UMETA(DisplayName = "Locked"),
	Bot			UMETA(DisplayName = "Bot"),
	Spectator	UMETA(DisplayName = "Spectator")
};

USTRUCT(BlueprintType)
struct FBBLobbySlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	int32 TeamID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	int32 SlotIndex = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	EBBLobbyPlayerSlotType SlotType = EBBLobbyPlayerSlotType::Open;
};

USTRUCT(BlueprintType)
struct FBBLobbyTeam
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	int32 TeamID = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	TArray<FBBLobbySlot> Slots;
};

USTRUCT(BlueprintType)
struct FBBLobbySettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	FString Description;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	int32 TeamSize = 4;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	int32 MaxPlayers = 40;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	int32 MaxTeams = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	FName StormShiftPreset = TEXT("Default");

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	bool bStormEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Lobby")
	bool bAllowSpectators = true;
};
