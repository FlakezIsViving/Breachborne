#include "BBHunterSelectWidget.h"

#include "Breachborne/Core/BreachborneGameState.h"
#include "Breachborne/Core/BreachbornePlayerController.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	UButton* MakeHunterButton(UWidgetTree* Tree, const FString& Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Label));
		Button->AddChild(Text);
		return Button;
	}
}

UBBHunterSelectWidget::UBBHunterSelectWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> UBBHunterSelectWidget::RebuildWidget()
{
	StatusSlateText.Reset();

	TSharedRef<SUniformGridPanel> HunterGrid = SNew(SUniformGridPanel)
		.SlotPadding(FMargin(6.0f));

	auto AddHunterButton = [this, HunterGrid](int32 HunterID, const TCHAR* Label, int32 Row, int32 Column)
	{
		HunterGrid->AddSlot(Column, Row)
		[
			SNew(SButton)
			.ContentPadding(FMargin(16.0f, 10.0f))
			.OnClicked_UObject(this, &UBBHunterSelectWidget::HandleSelectHunterClicked, HunterID)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Label))
			]
		];
	};

	AddHunterButton(1, TEXT("1 Ghost"), 0, 0);
	AddHunterButton(2, TEXT("2 Kingpin"), 0, 1);
	AddHunterButton(3, TEXT("3 Hunter"), 1, 0);
	AddHunterButton(4, TEXT("4 Hudson"), 1, 1);
	AddHunterButton(5, TEXT("5 Hunter"), 2, 0);
	AddHunterButton(6, TEXT("6 Hunter"), 2, 1);

	return SNew(SBorder)
		.Padding(FMargin(24.0f))
		.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.04f, 0.88f))
		[
			SNew(SBox)
			.WidthOverride(560.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 14.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Hunter Select")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 28))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					HunterGrid
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 16.0f, 0.0f, 8.0f)
				[
					SNew(SButton)
					.ContentPadding(FMargin(18.0f, 12.0f))
					.OnClicked_UObject(this, &UBBHunterSelectWidget::HandleReadyClicked)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Ready / Unready")))
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(StatusSlateText, STextBlock)
					.Text(FText::FromString(TEXT("Waiting for replicated state...")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 12.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Console debug: NetFlowDump, NetFlowSelectHunter 1, NetFlowReady 1")))
					.ColorAndOpacity(FLinearColor(0.65f, 0.75f, 1.0f, 1.0f))
				]
			]
		];
}

void UBBHunterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HunterSelectRoot"));
	WidgetTree->RootWidget = Root;

	HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	HeaderText->SetText(FText::FromString(TEXT("Hunter Select")));
	Root->AddChildToVerticalBox(HeaderText);

	UUniformGridPanel* Grid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	Root->AddChildToVerticalBox(Grid);

	UButton* Hunter1 = MakeHunterButton(WidgetTree, TEXT("1 Ghost"));
	Hunter1->OnClicked.AddDynamic(this, &UBBHunterSelectWidget::SelectHunter1);
	Grid->AddChildToUniformGrid(Hunter1, 0, 0);

	UButton* Hunter2 = MakeHunterButton(WidgetTree, TEXT("2 Kingpin"));
	Hunter2->OnClicked.AddDynamic(this, &UBBHunterSelectWidget::SelectHunter2);
	Grid->AddChildToUniformGrid(Hunter2, 0, 1);

	UButton* Hunter3 = MakeHunterButton(WidgetTree, TEXT("3 Hunter"));
	Hunter3->OnClicked.AddDynamic(this, &UBBHunterSelectWidget::SelectHunter3);
	Grid->AddChildToUniformGrid(Hunter3, 1, 0);

	UButton* Hunter4 = MakeHunterButton(WidgetTree, TEXT("4 Hudson"));
	Hunter4->OnClicked.AddDynamic(this, &UBBHunterSelectWidget::SelectHunter4);
	Grid->AddChildToUniformGrid(Hunter4, 1, 1);

	UButton* Hunter5 = MakeHunterButton(WidgetTree, TEXT("5 Hunter"));
	Hunter5->OnClicked.AddDynamic(this, &UBBHunterSelectWidget::SelectHunter5);
	Grid->AddChildToUniformGrid(Hunter5, 2, 0);

	UButton* Hunter6 = MakeHunterButton(WidgetTree, TEXT("6 Hunter"));
	Hunter6->OnClicked.AddDynamic(this, &UBBHunterSelectWidget::SelectHunter6);
	Grid->AddChildToUniformGrid(Hunter6, 2, 1);

	UButton* ReadyButton = MakeHunterButton(WidgetTree, TEXT("Ready / Unready"));
	ReadyButton->OnClicked.AddDynamic(this, &UBBHunterSelectWidget::ToggleReady);
	Root->AddChildToVerticalBox(ReadyButton);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Root->AddChildToVerticalBox(StatusText);

	RefreshStatus();
}

void UBBHunterSelectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshStatus();
}

void UBBHunterSelectWidget::SelectHunter1() { SelectHunter(1); }
void UBBHunterSelectWidget::SelectHunter2() { SelectHunter(2); }
void UBBHunterSelectWidget::SelectHunter3() { SelectHunter(3); }
void UBBHunterSelectWidget::SelectHunter4() { SelectHunter(4); }
void UBBHunterSelectWidget::SelectHunter5() { SelectHunter(5); }
void UBBHunterSelectWidget::SelectHunter6() { SelectHunter(6); }

void UBBHunterSelectWidget::ToggleReady()
{
	if (ABreachbornePlayerController* PC = Cast<ABreachbornePlayerController>(GetOwningPlayer()))
	{
		const ABreachbornePlayerState* PS = PC->GetPlayerState<ABreachbornePlayerState>();
		PC->RequestReadyState(PS ? !PS->IsReadyForMatch() : true);
	}
}

FReply UBBHunterSelectWidget::HandleSelectHunterClicked(int32 HunterID)
{
	SelectHunter(HunterID);
	return FReply::Handled();
}

FReply UBBHunterSelectWidget::HandleReadyClicked()
{
	ToggleReady();
	return FReply::Handled();
}

void UBBHunterSelectWidget::SelectHunter(int32 HunterID)
{
	if (ABreachbornePlayerController* PC = Cast<ABreachbornePlayerController>(GetOwningPlayer()))
	{
		PC->RequestHunterSelection(HunterID);
	}
}

void UBBHunterSelectWidget::RefreshStatus()
{
	if (!StatusText)
	{
		return;
	}

	const ABreachbornePlayerController* PC = Cast<ABreachbornePlayerController>(GetOwningPlayer());
	const ABreachbornePlayerState* PS = PC ? PC->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	const ABreachborneGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABreachborneGameState>() : nullptr;

	const FString Status = FString::Printf(TEXT("Team %d | Hunter %d | %s | Phase %d | Countdown %.0f"),
		PS ? PS->GetTeamID() : -1,
		PS ? PS->GetHunterID() : 0,
		(PS && PS->IsReadyForMatch()) ? TEXT("Ready") : TEXT("Not Ready"),
		GS ? static_cast<int32>(GS->GetMatchPhase()) : -1,
		GS ? GS->GetPhaseTimeRemaining() : 0.0f);
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Status));
	}
	if (StatusSlateText.IsValid())
	{
		StatusSlateText->SetText(FText::FromString(Status));
	}
}
