#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WispIndicatorWidget.generated.h"

class ABBWispPawn;

/**
 * World-space widget displayed above wisp pawns.
 * Shows the owning player's name and two bars:
 *   - Thin green upper bar: ally revive progress
 *   - Larger yellow lower bar: master decay HP; reaching zero kills the wisp
 */
UCLASS()
class BREACHBORNE_API UWispIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	void SetTargetWisp(ABBWispPawn* Wisp);

private:
	TWeakObjectPtr<ABBWispPawn> TargetWisp;
	mutable bool bLoggedFirstPaint = false;
};
