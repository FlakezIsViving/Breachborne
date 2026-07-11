#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BBNetworkDevSettings.generated.h"

/**
 * Developer/networking switches that keep single-player editor iteration fast
 * while packaged and dedicated builds use the real server flow.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Breachborne Networking"))
class BREACHBORNE_API UBBNetworkDevSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** If true, PIE also opens frontend screens instead of using quick-play shortcuts. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Editor")
	bool bUseFrontendFlowInEditor = false;

	/** Single-player standalone PIE spawns directly into gameplay for fast iteration. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Editor")
	bool bAutoStartSinglePlayerPIE = true;

	/** Single-player standalone PIE auto-selects DefaultHunterID. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Editor")
	bool bAutoAssignDefaultHunterInEditor = true;

	/** Single-player standalone PIE uses normal PlayerStart behavior. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Editor")
	bool bUsePlayerStartsForEditorQuickPlay = true;

	/** Dev respawn is allowed only for editor quick-play, never packaged/dedicated. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Editor")
	bool bAllowDevRespawnInEditorOnly = true;

	/** Allows debug exec/RPC cheats in packaged or dedicated builds. Keep false for playtests. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bAllowPackagedDebugCheats = false;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1", ClampMax = "4"))
	int32 SquadSize = 4;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxTeams = 10;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1", ClampMax = "40"))
	int32 MaxPlayers = 40;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1.0"))
	float HunterSelectCountdownSeconds = 5.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "1.0"))
	float PostMatchReturnSeconds = 10.0f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Network")
	FString DefaultDirectIPAddress = TEXT("127.0.0.1:7777");
};
