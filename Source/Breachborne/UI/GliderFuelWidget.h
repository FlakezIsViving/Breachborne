#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GliderFuelWidget.generated.h"

class AHunterCharacter;

UCLASS()
class BREACHBORNE_API UGliderFuelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void SetTargetHunter(AHunterCharacter* Hunter);

private:
	FLinearColor GetFuelColor(float Fuel) const;
	bool ShouldShow(float Fuel, bool bGliding, bool bGrounded) const;

	TWeakObjectPtr<AHunterCharacter> TargetHunter;
};
