#include "BBGameInstance.h"

#include "BBNetworkDevSettings.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/UI/BBCustomLobbyWidget.h"
#include "Breachborne/UI/BBFrontendStatusWidget.h"
#include "Breachborne/UI/BBHunterSelectWidget.h"
#include "Breachborne/UI/BBMinimapWidget.h"
#include "Breachborne/UI/BBMainMenuWidget.h"
#include "Breachborne/UI/BBPostMatchWidget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"

namespace
{
	const TCHAR* ReconnectConfigSection = TEXT("Breachborne.Reconnect");
	const TCHAR* ReconnectConfigKey = TEXT("ClientToken");
}

void UBBGameInstance::Init()
{
	Super::Init();
	FrontendState = EBBFrontendState::Boot;

	const bool bCommandLineToken = FParse::Value(
		FCommandLine::Get(), TEXT("BBReconnectToken="), ReconnectToken);
	if (!bCommandLineToken && GConfig)
	{
		GConfig->GetString(ReconnectConfigSection, ReconnectConfigKey, ReconnectToken, GGameUserSettingsIni);
	}
	if (ReconnectToken.IsEmpty())
	{
		ReconnectToken = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
		if (GConfig)
		{
			GConfig->SetString(ReconnectConfigSection, ReconnectConfigKey, *ReconnectToken, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
	}
}

void UBBGameInstance::SetFrontendState(EBBFrontendState NewState)
{
	if (FrontendState == NewState)
	{
		return;
	}

	FrontendState = NewState;
	UE_LOG(LogBreachborne, Log, TEXT("Frontend state changed to %d"), static_cast<int32>(FrontendState));
}

void UBBGameInstance::ShowMainMenu()
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (ShouldSkipFrontendForEditorQuickPlay(GetWorld()))
	{
		return;
	}

	SetFrontendState(EBBFrontendState::MainMenu);
	ShowWidget<UBBMainMenuWidget>(MainMenuWidgetClass, UBBMainMenuWidget::StaticClass());
}

void UBBGameInstance::ShowConnecting(const FString& Address)
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	SetFrontendState(EBBFrontendState::Connecting);
	if (UBBFrontendStatusWidget* StatusWidget = ShowWidget<UBBFrontendStatusWidget>(StatusWidgetClass, UBBFrontendStatusWidget::StaticClass()))
	{
		StatusWidget->SetStatusText(FText::FromString(FString::Printf(TEXT("Connecting to %s"), *Address)));
	}
}

void UBBGameInstance::ShowLobby()
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	SetFrontendState(EBBFrontendState::Lobby);
	ShowWidget<UBBCustomLobbyWidget>(LobbyWidgetClass, UBBCustomLobbyWidget::StaticClass());
}

void UBBGameInstance::ShowHunterSelect()
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	SetFrontendState(EBBFrontendState::HunterSelect);
	ShowWidget<UBBHunterSelectWidget>(HunterSelectWidgetClass, UBBHunterSelectWidget::StaticClass());
}

void UBBGameInstance::ShowPostMatch()
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	SetFrontendState(EBBFrontendState::PostMatch);
	ShowWidget<UBBPostMatchWidget>(PostMatchWidgetClass, UBBPostMatchWidget::StaticClass());
}

void UBBGameInstance::ToggleFullscreenMap()
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (FrontendState == EBBFrontendState::FullscreenMap)
	{
		ClearFrontendWidget();
		SetFrontendState(EBBFrontendState::InMatch);
		return;
	}

	if (FrontendState == EBBFrontendState::InMatch)
	{
		SetFrontendState(EBBFrontendState::FullscreenMap);
		ShowWidget<UBBMinimapWidget>(FullscreenMapWidgetClass, UBBMinimapWidget::StaticClass());
	}
}

void UBBGameInstance::ClearFrontendWidget()
{
	if (ActiveFrontendWidget)
	{
		ActiveFrontendWidget->RemoveFromParent();
		ActiveFrontendWidget = nullptr;
	}
}

void UBBGameInstance::ConnectDirectIP(const FString& Address)
{
	const FString TrimmedAddress = Address.TrimStartAndEnd();
	if (TrimmedAddress.IsEmpty())
	{
		return;
	}

	ShowConnecting(TrimmedAddress);

	APlayerController* PC = CachedLocalPlayerController.Get();
	if (!PC && GetWorld())
	{
		PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	}

	if (PC)
	{
		FString TravelAddress = TrimmedAddress;
		if (!ReconnectToken.IsEmpty() && !TravelAddress.Contains(TEXT("BBReconnectToken="), ESearchCase::IgnoreCase))
		{
			TravelAddress += FString::Printf(TEXT("?BBReconnectToken=%s"), *ReconnectToken);
		}
		PC->ClientTravel(TravelAddress, TRAVEL_Absolute);
	}
}

void UBBGameInstance::StartLocalQuickPlay()
{
	ClearFrontendWidget();
	SetFrontendState(EBBFrontendState::LoadingMatch);

	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::OpenLevel(World, QuickPlayMapName);
	}
}

void UBBGameInstance::SyncFrontendToMatchPhase(EMatchPhase MatchPhase)
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (ShouldSkipFrontendForEditorQuickPlay(GetWorld()))
	{
		return;
	}

	switch (MatchPhase)
	{
	case EMatchPhase::WaitingForPlayers:
		ShowLobby();
		break;
	case EMatchPhase::HunterSelect:
	case EMatchPhase::Countdown:
		ShowHunterSelect();
		break;
	case EMatchPhase::Dropping:
	case EMatchPhase::Playing:
		SetFrontendState(EBBFrontendState::InMatch);
		ClearFrontendWidget();
		break;
	case EMatchPhase::PostMatch:
	case EMatchPhase::Ended:
		ShowPostMatch();
		break;
	case EMatchPhase::Resetting:
		SetFrontendState(EBBFrontendState::ReturningToLobby);
		break;
	default:
		break;
	}
}

void UBBGameInstance::HandleLocalPlayerControllerReady(APlayerController* PlayerController)
{
	CachedLocalPlayerController = PlayerController;

	if (!PlayerController || ShouldSkipFrontendForEditorQuickPlay(PlayerController->GetWorld()))
	{
		return;
	}

	if (PlayerController->GetNetMode() == NM_Client)
	{
		ShowLobby();
	}
	else if (PlayerController->GetNetMode() == NM_Standalone)
	{
		ShowMainMenu();
	}
}

bool UBBGameInstance::ShouldSkipFrontendForEditorQuickPlay(const UWorld* World) const
{
	if (!World)
	{
		return false;
	}

#if WITH_EDITOR
	const UBBNetworkDevSettings* Settings = GetDefault<UBBNetworkDevSettings>();
	return World->WorldType == EWorldType::PIE
		&& World->GetNetMode() == NM_Standalone
		&& Settings
		&& !Settings->bUseFrontendFlowInEditor
		&& Settings->bAutoStartSinglePlayerPIE;
#else
	return false;
#endif
}

template <typename WidgetT>
WidgetT* UBBGameInstance::ShowWidget(TSubclassOf<WidgetT> ConfiguredClass, TSubclassOf<WidgetT> NativeFallbackClass)
{
	ClearFrontendWidget();

	UWorld* World = GetWorld();
	APlayerController* PC = CachedLocalPlayerController.Get();
	if (!PC && World)
	{
		PC = UGameplayStatics::GetPlayerController(World, 0);
	}
	if (!PC)
	{
		return nullptr;
	}

	TSubclassOf<WidgetT> WidgetClass = ConfiguredClass ? ConfiguredClass : NativeFallbackClass;
	WidgetT* Widget = CreateWidget<WidgetT>(PC, WidgetClass);
	if (!Widget)
	{
		return nullptr;
	}

	Widget->AddToViewport();
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(PC);
	const bool bIsLobbyWidget = Widget->IsA<UBBCustomLobbyWidget>();
	const FVector2D DesiredSize = bIsLobbyWidget
		? FVector2D(FMath::Max(360.0f, ViewportSize.X - 32.0f), FMath::Max(360.0f, ViewportSize.Y - 32.0f))
		: FVector2D(FMath::Min(640.0f, FMath::Max(320.0f, ViewportSize.X - 32.0f)), FMath::Min(420.0f, FMath::Max(280.0f, ViewportSize.Y - 32.0f)));
	Widget->SetDesiredSizeInViewport(DesiredSize);
	Widget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	Widget->SetPositionInViewport(ViewportSize * 0.5f, false);
	ActiveFrontendWidget = Widget;
	return Widget;
}
