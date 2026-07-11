#include "GliderFuelWidget.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Characters/GliderComponent.h"
#include "Breachborne/Characters/BBCharacterMovementComponent.h"
#include "Rendering/DrawElements.h"

void UGliderFuelWidget::SetTargetHunter(AHunterCharacter* Hunter)
{
	TargetHunter = Hunter;
}

void UGliderFuelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const AHunterCharacter* Hunter = TargetHunter.Get();
	const UGliderComponent* Glider = Hunter ? Hunter->GetGliderComponent() : nullptr;
	const UBBCharacterMovementComponent* Movement = Hunter ? Hunter->GetBBMovementComponent() : nullptr;
	if (!Glider || !Movement)
	{
		SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

int32 UGliderFuelWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const AHunterCharacter* Hunter = TargetHunter.Get();
	const UGliderComponent* Glider = Hunter ? Hunter->GetGliderComponent() : nullptr;
	const UBBCharacterMovementComponent* Movement = Hunter ? Hunter->GetBBMovementComponent() : nullptr;
	if (!Glider || !Movement || !ShouldShow(Glider->GetFuelLevel(), Glider->IsGliderOpen(), Movement->IsMovingOnGround()))
	{
		return BaseLayer;
	}

	const float Fuel = Glider ? FMath::Clamp(Glider->GetFuelLevel(), 0.0f, 1.0f) : 0.0f;

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FVector2D Center = Size * 0.5f;
	const float Radius = FMath::Max(1.0f, FMath::Min(Size.X, Size.Y) * 0.42f);
	constexpr int32 SegmentCount = 48;

	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		const float Angle = (static_cast<float>(Index) / SegmentCount) * 2.0f * PI - (PI * 0.5f);
		const FVector2D End = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
		TArray<FVector2D> Line;
		Line.Add(Center);
		Line.Add(End);
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 1, AllottedGeometry.ToPaintGeometry(),
			Line, ESlateDrawEffect::None, FLinearColor::Black, true, 2.5f);
	}

	const int32 FuelSegments = FMath::RoundToInt(Fuel * SegmentCount);
	const FLinearColor FuelColor = GetFuelColor(Fuel);
	for (int32 Index = 0; Index < FuelSegments; ++Index)
	{
		const float Angle = (static_cast<float>(Index) / SegmentCount) * 2.0f * PI - (PI * 0.5f);
		const FVector2D End = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
		TArray<FVector2D> Line;
		Line.Add(Center);
		Line.Add(End);
		FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 2, AllottedGeometry.ToPaintGeometry(),
			Line, ESlateDrawEffect::None, FuelColor, true, 2.5f);
	}

	TArray<FVector2D> Outline;
	for (int32 Index = 0; Index <= SegmentCount; ++Index)
	{
		const float Angle = (static_cast<float>(Index) / SegmentCount) * 2.0f * PI;
		Outline.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
	}
	FSlateDrawElement::MakeLines(OutDrawElements, BaseLayer + 3, AllottedGeometry.ToPaintGeometry(),
		Outline, ESlateDrawEffect::None, FLinearColor(0.02f, 0.02f, 0.02f, 1.0f), true, 2.0f);

	return BaseLayer + 3;
}

FLinearColor UGliderFuelWidget::GetFuelColor(float Fuel) const
{
	if (Fuel > 0.5f)
	{
		const float T = (Fuel - 0.5f) / 0.5f;
		return FMath::Lerp(FLinearColor(1.0f, 0.72f, 0.05f, 1.0f), FLinearColor(0.35f, 1.0f, 0.1f, 1.0f), T);
	}

	const float T = Fuel / 0.5f;
	return FMath::Lerp(FLinearColor(1.0f, 0.05f, 0.02f, 1.0f), FLinearColor(1.0f, 0.72f, 0.05f, 1.0f), T);
}

bool UGliderFuelWidget::ShouldShow(float Fuel, bool bGliding, bool bGrounded) const
{
	return bGliding || !bGrounded || Fuel < 0.995f;
}
