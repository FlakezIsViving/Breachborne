#include "WispIndicatorWidget.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Wisp/BBWispPawn.h"
#include "Breachborne/Breachborne.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

void UWispIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	UE_LOG(LogBreachborne, Log, TEXT("[WISP_UI] NativeConstruct | Widget=%s | Owner=%s"),
		*GetName(), GetOwningPlayer() ? *GetOwningPlayer()->GetName() : TEXT("None"));
}

void UWispIndicatorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UWispIndicatorWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
		OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const ABBWispPawn* Wisp = TargetWisp.Get();
	if (!Wisp)
	{
		return BaseLayer;
	}

	const FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	if (WidgetSize.X <= 1.0f || WidgetSize.Y <= 1.0f)
	{
		return BaseLayer;
	}
	if (!bLoggedFirstPaint)
	{
		bLoggedFirstPaint = true;
		UE_LOG(LogBreachborne, Log, TEXT("[WISP_UI] FirstPaint | Wisp=%s | Size=%s"),
			*Wisp->GetName(), *WidgetSize.ToString());
	}

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const float MaxHP = Wisp->GetMaxWispHP();
	const float DecayPercent = MaxHP > 0.0f
		? FMath::Clamp(Wisp->GetCurrentWispHP() / MaxHP, 0.0f, 1.0f)
		: 0.0f;
	const float RevivePercent = FMath::Clamp(Wisp->GetRezBarProgress(), 0.0f, 1.0f);

	const ABreachbornePlayerState* OwnerPS = Wisp->GetOwningPlayerState();
	const FString OwnerName = OwnerPS ? OwnerPS->GetPlayerName() : FString(TEXT("Wisp"));
	const FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10);
	const FVector2D TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(OwnerName, NameFont);
	const FVector2D TextPosition(FMath::Max(0.0f, (WidgetSize.X - TextSize.X) * 0.5f), 0.0f);
	FSlateDrawElement::MakeText(OutDrawElements, BaseLayer + 1,
		AllottedGeometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(TextPosition)),
		OwnerName, NameFont, ESlateDrawEffect::None, FLinearColor::White);

	const FLinearColor BorderColor(0.01f, 0.015f, 0.02f, 0.96f);
	const FLinearColor EmptyColor(0.10f, 0.11f, 0.12f, 0.92f);
	const FLinearColor ReviveColor(0.15f, 0.95f, 0.45f, 1.0f);
	const FLinearColor DecayColor(0.92f, 0.85f, 0.18f, 1.0f);

	const FVector2D ReviveOuterPos(4.0f, 17.0f);
	const FVector2D ReviveOuterSize(FMath::Max(1.0f, WidgetSize.X - 8.0f), 8.0f);
	const FVector2D ReviveInnerPos = ReviveOuterPos + FVector2D(1.0f, 1.0f);
	const FVector2D ReviveInnerSize(ReviveOuterSize.X - 2.0f, ReviveOuterSize.Y - 2.0f);
	FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 1,
		AllottedGeometry.ToPaintGeometry(ReviveOuterSize, FSlateLayoutTransform(ReviveOuterPos)),
		WhiteBrush, ESlateDrawEffect::None, BorderColor);
	FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 2,
		AllottedGeometry.ToPaintGeometry(ReviveInnerSize, FSlateLayoutTransform(ReviveInnerPos)),
		WhiteBrush, ESlateDrawEffect::None, EmptyColor);
	if (RevivePercent > 0.0f)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 3,
			AllottedGeometry.ToPaintGeometry(FVector2D(ReviveInnerSize.X * RevivePercent, ReviveInnerSize.Y),
				FSlateLayoutTransform(ReviveInnerPos)), WhiteBrush, ESlateDrawEffect::None, ReviveColor);
	}

	const FVector2D DecayOuterPos(0.0f, 29.0f);
	const FVector2D DecayOuterSize(WidgetSize.X, 16.0f);
	const FVector2D DecayInnerPos = DecayOuterPos + FVector2D(2.0f, 2.0f);
	const FVector2D DecayInnerSize(DecayOuterSize.X - 4.0f, DecayOuterSize.Y - 4.0f);
	FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 1,
		AllottedGeometry.ToPaintGeometry(DecayOuterSize, FSlateLayoutTransform(DecayOuterPos)),
		WhiteBrush, ESlateDrawEffect::None, BorderColor);
	FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 2,
		AllottedGeometry.ToPaintGeometry(DecayInnerSize, FSlateLayoutTransform(DecayInnerPos)),
		WhiteBrush, ESlateDrawEffect::None, EmptyColor);
	if (DecayPercent > 0.0f)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, BaseLayer + 3,
			AllottedGeometry.ToPaintGeometry(FVector2D(DecayInnerSize.X * DecayPercent, DecayInnerSize.Y),
				FSlateLayoutTransform(DecayInnerPos)), WhiteBrush, ESlateDrawEffect::None, DecayColor);
	}

	return BaseLayer + 3;
}

void UWispIndicatorWidget::SetTargetWisp(ABBWispPawn* Wisp)
{
	TargetWisp = Wisp;
	UE_LOG(LogBreachborne, Log, TEXT("[WISP_UI] SetTargetWisp | Wisp=%s | Widget=%s"),
		Wisp ? *Wisp->GetName() : TEXT("NULL"), *GetName());
}
