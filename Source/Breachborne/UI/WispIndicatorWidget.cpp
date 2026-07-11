#include "WispIndicatorWidget.h"
#include "Components/ProgressBar.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "Breachborne/Wisp/BBWispPawn.h"
#include "Breachborne/Breachborne.h"

void UWispIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogBreachborne, Log, TEXT("[WISP_UI] NativeConstruct | Widget=%s | Owner=%s"),
		*GetName(), GetOwningPlayer() ? *GetOwningPlayer()->GetName() : TEXT("None"));

	// Root container — fixed size so bars don't jitter
	USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSize"));
	RootSize->SetWidthOverride(120.0f);
	RootSize->SetHeightOverride(40.0f);
	WidgetTree->RootWidget = RootSize;

	VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
	RootSize->AddChild(VBox);

	// Yellow decay bar (wisp HP)
	DecayBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("DecayBar"));
	DecayBar->SetFillColorAndOpacity(FLinearColor(1.0f, 0.84f, 0.0f)); // Supervive yellow
	DecayBar->SetPercent(1.0f);
	DecayBar->SetBarFillType(EProgressBarFillType::LeftToRight);

	// Green revive bar
	ReviveBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ReviveBar"));
	ReviveBar->SetFillColorAndOpacity(FLinearColor(0.2f, 1.0f, 0.2f)); // Bright green
	ReviveBar->SetPercent(0.0f);
	ReviveBar->SetBarFillType(EProgressBarFillType::LeftToRight);

	UVerticalBoxSlot* DecaySlot = Cast<UVerticalBoxSlot>(VBox->AddChildToVerticalBox(DecayBar));
	if (DecaySlot)
	{
		DecaySlot->SetPadding(FMargin(0.0f, 1.0f));
		FSlateChildSize DecaySize(ESlateSizeRule::Fill);
		DecaySize.Value = 1.0f;
		DecaySlot->SetSize(DecaySize);
	}

	UVerticalBoxSlot* ReviveSlot = Cast<UVerticalBoxSlot>(VBox->AddChildToVerticalBox(ReviveBar));
	if (ReviveSlot)
	{
		ReviveSlot->SetPadding(FMargin(0.0f, 1.0f));
		FSlateChildSize ReviveSize(ESlateSizeRule::Fill);
		ReviveSize.Value = 1.0f;
		ReviveSlot->SetSize(ReviveSize);
	}
}

void UWispIndicatorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ABBWispPawn* Wisp = TargetWisp.Get();
	if (!Wisp)
	{
		if (DecayBar) DecayBar->SetPercent(0.0f);
		if (ReviveBar) ReviveBar->SetPercent(0.0f);
		return;
	}

	const float MaxHP = Wisp->GetMaxWispHP();
	const float HP = Wisp->GetCurrentWispHP();
	const float Rez = Wisp->GetRezBarProgress();
	const float DecayPercent = MaxHP > 0.0f ? HP / MaxHP : 0.0f;

	if (DecayBar)
	{
		DecayBar->SetPercent(DecayPercent);
	}

	if (ReviveBar)
	{
		ReviveBar->SetPercent(FMath::Clamp(Rez, 0.0f, 1.0f));
	}

	UE_LOG(LogBreachborne, Verbose, TEXT("[WISP_UI] Tick | Wisp=%s | Decay=%.1f%% | Revive=%.1f%%"),
		*Wisp->GetName(), DecayPercent * 100.0f, Rez * 100.0f);
}

void UWispIndicatorWidget::SetTargetWisp(ABBWispPawn* Wisp)
{
	TargetWisp = Wisp;
	UE_LOG(LogBreachborne, Log, TEXT("[WISP_UI] SetTargetWisp | Wisp=%s | Widget=%s"),
		Wisp ? *Wisp->GetName() : TEXT("NULL"), *GetName());
}
