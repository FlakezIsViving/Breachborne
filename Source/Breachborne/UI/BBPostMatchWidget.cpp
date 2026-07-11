#include "BBPostMatchWidget.h"

#include "Breachborne/Core/BreachborneGameState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

UBBPostMatchWidget::UBBPostMatchWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> UBBPostMatchWidget::RebuildWidget()
{
	SummarySlateText.Reset();

	return SNew(SBorder)
		.Padding(FMargin(24.0f))
		.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.04f, 0.88f))
		[
			SNew(SBox)
			.WidthOverride(520.0f)
			[
				SAssignNew(SummarySlateText, STextBlock)
				.Text(FText::FromString(TEXT("Match Complete")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
			]
		];
}

void UBBPostMatchWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PostMatchRoot"));
	WidgetTree->RootWidget = Root;

	SummaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Root->AddChildToVerticalBox(SummaryText);
	RefreshSummary();
}

void UBBPostMatchWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshSummary();
}

void UBBPostMatchWidget::RefreshSummary()
{
	if (!SummaryText)
	{
		return;
	}

	const ABreachborneGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABreachborneGameState>() : nullptr;
	SummaryText->SetText(FText::FromString(FString::Printf(TEXT("Match Complete\nWinner: Team %d\nReturning to lobby in %.0f"),
		GS ? GS->GetWinningTeamID() : -1,
		GS ? GS->GetPhaseTimeRemaining() : 0.0f)));

	if (SummarySlateText.IsValid())
	{
		SummarySlateText->SetText(FText::FromString(FString::Printf(TEXT("Match Complete\nWinner: Team %d\nReturning to lobby in %.0f"),
			GS ? GS->GetWinningTeamID() : -1,
			GS ? GS->GetPhaseTimeRemaining() : 0.0f)));
	}
}
