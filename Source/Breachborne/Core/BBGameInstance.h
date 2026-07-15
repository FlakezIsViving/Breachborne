#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "BreachborneTypes.h"
#include "BBGameInstance.generated.h"

class UBBFrontendStatusWidget;
class UBBCustomLobbyWidget;
class UBBHunterSelectWidget;
class UBBMinimapWidget;
class UBBMainMenuWidget;
class UBBPostMatchWidget;
class UUserWidget;
class APlayerController;

/**
 * Owns the local frontend flow. The dedicated server never creates these widgets;
 * clients use this to move between main menu, connection status, hunter select,
 * in-match HUD, and post-match screens.
 */
UCLASS()
class BREACHBORNE_API UBBGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	UFUNCTION(BlueprintPure, Category = "Breachborne|Frontend")
	EBBFrontendState GetFrontendState() const { return FrontendState; }

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Frontend")
	void SetFrontendState(EBBFrontendState NewState);

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Frontend")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Frontend")
	void ShowConnecting(const FString& Address);

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Frontend")
	void ShowLobby();

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Frontend")
	void ShowHunterSelect();

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Frontend")
	void ShowPostMatch();

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Frontend")
	void ToggleFullscreenMap();

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Frontend")
	void ClearFrontendWidget();

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Frontend")
	void ConnectDirectIP(const FString& Address);

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Frontend")
	void StartLocalQuickPlay();

	/** Called by local PlayerController and GameState when replicated match state changes. */
	void SyncFrontendToMatchPhase(EMatchPhase MatchPhase);

	void HandleLocalPlayerControllerReady(APlayerController* PlayerController);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Frontend")
	TSubclassOf<UBBMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Frontend")
	TSubclassOf<UBBFrontendStatusWidget> StatusWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Frontend")
	TSubclassOf<UBBCustomLobbyWidget> LobbyWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Frontend")
	TSubclassOf<UBBHunterSelectWidget> HunterSelectWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Frontend")
	TSubclassOf<UBBPostMatchWidget> PostMatchWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Frontend")
	TSubclassOf<UBBMinimapWidget> FullscreenMapWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Frontend")
	FName QuickPlayMapName = TEXT("/Game/Maps/TestMap");

private:
	/** Stable for this client process so a direct-IP reconnect can reclaim its active match slot. */
	FString ReconnectToken;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveFrontendWidget;

	UPROPERTY()
	TObjectPtr<APlayerController> CachedLocalPlayerController;

	EBBFrontendState FrontendState = EBBFrontendState::Boot;

	template <typename WidgetT>
	WidgetT* ShowWidget(TSubclassOf<WidgetT> ConfiguredClass, TSubclassOf<WidgetT> NativeFallbackClass);

	bool ShouldSkipFrontendForEditorQuickPlay(const UWorld* World) const;
};
