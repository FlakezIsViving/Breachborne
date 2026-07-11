#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WispIndicatorWidget.generated.h"

class UProgressBar;
class UVerticalBox;
class ABBWispPawn;

/**
 * World-space widget displayed above wisp pawns.
 * Shows two bars:
 *   - Yellow decay bar: wisp HP / max HP (drains when not being revived)
 *   - Green revive bar: revive progress 0-1 (fills when ally is near/healing)
 */
UCLASS()
class BREACHBORNE_API UWispIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void SetTargetWisp(ABBWispPawn* Wisp);

private:
	UPROPERTY()
	TObjectPtr<UProgressBar> DecayBar;

	UPROPERTY()
	TObjectPtr<UProgressBar> ReviveBar;

	UPROPERTY()
	TObjectPtr<UVerticalBox> VBox;

	TWeakObjectPtr<ABBWispPawn> TargetWisp;
};
