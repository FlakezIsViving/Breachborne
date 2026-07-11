#include "BBCustomLobbyWidget.h"

#include "Breachborne/Core/BreachborneGameState.h"
#include "Breachborne/Core/BreachbornePlayerController.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

UBBCustomLobbyWidget::UBBCustomLobbyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> UBBCustomLobbyWidget::RebuildWidget()
{
	LastLobbyFingerprint.Reset();

	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);

	Root->AddSlot()
	.AutoHeight()
	.Padding(18.0f, 14.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SAssignNew(HeaderText, STextBlock)
			.Text(FText::FromString(TEXT("Custom Game")))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 32))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(8.0f, 0.0f)
		[
			SNew(SButton)
			.ContentPadding(FMargin(16.0f, 10.0f))
			.OnClicked_UObject(this, &UBBCustomLobbyWidget::HandleStartClicked)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Start Match")))
			]
		]
	];

	Root->AddSlot()
	.AutoHeight()
	.Padding(18.0f, 0.0f, 18.0f, 12.0f)
	[
		SAssignNew(StatusText, STextBlock)
		.Text(FText::FromString(TEXT("Waiting for lobby state...")))
		.ColorAndOpacity(FLinearColor(0.78f, 0.9f, 1.0f, 1.0f))
	];

	Root->AddSlot()
	.AutoHeight()
	.Padding(18.0f, 0.0f, 18.0f, 12.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			SNew(SButton)
			.OnClicked_UObject(this, &UBBCustomLobbyWidget::HandleAutoFillClicked)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Auto")))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 18.0f, 0.0f)
		[
			SNew(SButton)
			.OnClicked_UObject(this, &UBBCustomLobbyWidget::HandleSpectateClicked)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Spectate")))
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.Padding(0.0f, 0.0f, 12.0f, 0.0f)
		[
			SNew(SEditableTextBox)
			.HintText(FText::FromString(TEXT("Owner: lobby description")))
			.OnTextCommitted_UObject(this, &UBBCustomLobbyWidget::HandleDescriptionCommitted)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(SButton)
			.OnClicked_UObject(this, &UBBCustomLobbyWidget::HandleTeamSizeClicked, 4)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Squads")))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(SButton)
			.OnClicked_UObject(this, &UBBCustomLobbyWidget::HandleTeamSizeClicked, 3)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Trios")))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(SButton)
			.OnClicked_UObject(this, &UBBCustomLobbyWidget::HandleTeamSizeClicked, 2)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Duos")))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4.0f, 0.0f)
		[
			SNew(SButton)
			.OnClicked_UObject(this, &UBBCustomLobbyWidget::HandleTeamSizeClicked, 1)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Solos")))
			]
		]
	];

	Root->AddSlot()
	.FillHeight(1.0f)
	.Padding(18.0f, 0.0f, 18.0f, 18.0f)
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(TeamGrid, SUniformGridPanel)
				.SlotPadding(FMargin(8.0f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				SAssignNew(SpectatorBox, SVerticalBox)
			]
		]
	];

	RefreshLobby(true);

	return SNew(SBorder)
		.Padding(0.0f)
		.BorderBackgroundColor(FLinearColor(0.01f, 0.015f, 0.025f, 0.94f))
		[
			Root
		];
}

void UBBCustomLobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshLobby(false);
}

FReply UBBCustomLobbyWidget::HandleJoinSlotClicked(int32 TeamID, int32 SlotIndex)
{
	if (ABreachbornePlayerController* PC = Cast<ABreachbornePlayerController>(GetOwningPlayer()))
	{
		PC->RequestJoinLobbySlot(TeamID, SlotIndex);
	}
	return FReply::Handled();
}

FReply UBBCustomLobbyWidget::HandleSpectateClicked()
{
	if (ABreachbornePlayerController* PC = Cast<ABreachbornePlayerController>(GetOwningPlayer()))
	{
		PC->RequestJoinSpectators();
	}
	return FReply::Handled();
}

FReply UBBCustomLobbyWidget::HandleAutoFillClicked()
{
	if (ABreachbornePlayerController* PC = Cast<ABreachbornePlayerController>(GetOwningPlayer()))
	{
		PC->RequestAutoFillLobbySlot();
	}
	return FReply::Handled();
}

FReply UBBCustomLobbyWidget::HandleTeamSizeClicked(int32 TeamSize)
{
	if (ABreachbornePlayerController* PC = Cast<ABreachbornePlayerController>(GetOwningPlayer()))
	{
		PC->RequestSetLobbyTeamSize(TeamSize);
	}
	return FReply::Handled();
}

FReply UBBCustomLobbyWidget::HandleStartClicked()
{
	if (ABreachbornePlayerController* PC = Cast<ABreachbornePlayerController>(GetOwningPlayer()))
	{
		PC->RequestStartLobbyMatch();
	}
	return FReply::Handled();
}

void UBBCustomLobbyWidget::HandleDescriptionCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType != ETextCommit::OnEnter && CommitType != ETextCommit::OnUserMovedFocus)
	{
		return;
	}

	if (ABreachbornePlayerController* PC = Cast<ABreachbornePlayerController>(GetOwningPlayer()))
	{
		PC->RequestSetLobbyDescription(Text.ToString());
	}
}

void UBBCustomLobbyWidget::RefreshLobby(bool bForce)
{
	const FString Fingerprint = BuildLobbyFingerprint();
	if (!bForce && Fingerprint == LastLobbyFingerprint)
	{
		return;
	}
	LastLobbyFingerprint = Fingerprint;

	const ABreachborneGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABreachborneGameState>() : nullptr;
	const ABreachbornePlayerController* PC = Cast<ABreachbornePlayerController>(GetOwningPlayer());
	const ABreachbornePlayerState* LocalPS = PC ? PC->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	if (!GS || !TeamGrid.IsValid() || !SpectatorBox.IsValid())
	{
		return;
	}

	const FBBLobbySettings& Settings = GS->GetLobbySettings();
	const APlayerState* OwnerPS = GS->GetLobbyOwnerPlayerState();
	const bool bOwner = IsLocalPlayerLobbyOwner();

	if (StatusText.IsValid())
	{
		StatusText->SetText(FText::FromString(FString::Printf(
			TEXT("Owner: %s | Players: %d/%d | Team Size: %d | Teams: %d | Storm: %s | %s"),
			OwnerPS ? *OwnerPS->GetPlayerName() : TEXT("None"),
			GS->GetLobbyActivePlayerCount(),
			Settings.MaxPlayers,
			Settings.TeamSize,
			Settings.MaxTeams,
			*Settings.StormShiftPreset.ToString(),
			bOwner ? TEXT("Owner controls enabled") : TEXT("Only owner can change settings/start"))));
	}

	TeamGrid->ClearChildren();
	SpectatorBox->ClearChildren();

	const FVector2D ViewportSize = PC ? UWidgetLayoutLibrary::GetViewportSize(PC) : FVector2D(1280.0f, 720.0f);
	const int32 Columns = FMath::Clamp(FMath::FloorToInt((ViewportSize.X - 96.0f) / 220.0f), 1, 5);
	for (int32 TeamIndex = 0; TeamIndex < GS->GetLobbyTeams().Num(); ++TeamIndex)
	{
		const FBBLobbyTeam& Team = GS->GetLobbyTeams()[TeamIndex];
		TSharedRef<SVerticalBox> TeamBox = SNew(SVerticalBox);
		TeamBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%s (%d/%d)"), *Team.DisplayName, Team.Slots.FilterByPredicate([](const FBBLobbySlot& LobbySlot)
			{
				return LobbySlot.SlotType == EBBLobbyPlayerSlotType::Player && LobbySlot.PlayerState;
			}).Num(), Team.Slots.Num())))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
		];

		for (const FBBLobbySlot& LobbySlot : Team.Slots)
		{
			const APlayerState* SlotPS = LobbySlot.PlayerState;
			const bool bLocal = LocalPS && SlotPS == LocalPS;
			const FString Label = SlotPS ? SlotPS->GetPlayerName() : FString::Printf(TEXT("Open Slot %d"), LobbySlot.SlotIndex + 1);
			TeamBox->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 3.0f)
			[
				SNew(SButton)
				.ContentPadding(FMargin(8.0f, 6.0f))
				.OnClicked_UObject(this, &UBBCustomLobbyWidget::HandleJoinSlotClicked, Team.TeamID, LobbySlot.SlotIndex)
				[
					SNew(STextBlock)
					.Text(FText::FromString(bLocal ? FString::Printf(TEXT("> %s"), *Label) : Label))
					.ColorAndOpacity(bLocal ? FLinearColor(0.25f, 1.0f, 0.75f, 1.0f) : FLinearColor::White)
				]
			];
		}

		TeamGrid->AddSlot(TeamIndex % Columns, TeamIndex / Columns)
		[
			SNew(SBorder)
			.Padding(10.0f)
			.BorderBackgroundColor(FLinearColor(0.08f, 0.10f, 0.13f, 0.88f))
			[
				TeamBox
			]
		];
	}

	SpectatorBox->AddSlot()
	.AutoHeight()
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Spectators")))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
	];

	if (GS->GetSpectatorSlots().IsEmpty())
	{
		SpectatorBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("None")))
		];
	}
	else
	{
		for (const FBBLobbySlot& LobbySlot : GS->GetSpectatorSlots())
		{
			SpectatorBox->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(LobbySlot.PlayerState ? LobbySlot.PlayerState->GetPlayerName() : TEXT("Unknown")))
			];
		}
	}
}

FString UBBCustomLobbyWidget::BuildLobbyFingerprint() const
{
	const ABreachborneGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABreachborneGameState>() : nullptr;
	if (!GS)
	{
		return TEXT("no-gs");
	}

	const FBBLobbySettings& Settings = GS->GetLobbySettings();
	FString Fingerprint = FString::Printf(TEXT("%d|%d|%d|%s|%s|%s"),
		static_cast<int32>(GS->GetMatchPhase()),
		Settings.TeamSize,
		Settings.MaxTeams,
		*Settings.Description,
		*Settings.StormShiftPreset.ToString(),
		*GetNameSafe(GS->GetLobbyOwnerPlayerState()));

	for (const FBBLobbyTeam& Team : GS->GetLobbyTeams())
	{
		Fingerprint += FString::Printf(TEXT("|T%d"), Team.TeamID);
		for (const FBBLobbySlot& LobbySlot : Team.Slots)
		{
			Fingerprint += FString::Printf(TEXT(":%d:%s:%d"), LobbySlot.SlotIndex, *GetNameSafe(LobbySlot.PlayerState), static_cast<int32>(LobbySlot.SlotType));
		}
	}
	for (const FBBLobbySlot& LobbySlot : GS->GetSpectatorSlots())
	{
		Fingerprint += FString::Printf(TEXT("|S:%s"), *GetNameSafe(LobbySlot.PlayerState));
	}
	return Fingerprint;
}

bool UBBCustomLobbyWidget::IsLocalPlayerLobbyOwner() const
{
	const ABreachborneGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABreachborneGameState>() : nullptr;
	const ABreachbornePlayerController* PC = Cast<ABreachbornePlayerController>(GetOwningPlayer());
	return GS && PC && GS->GetLobbyOwnerPlayerState() == PC->PlayerState;
}
