#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/SWidget.h"
#include "BBPostMatchWidget.generated.h"

UCLASS()
class BREACHBORNE_API UBBPostMatchWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBBPostMatchWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	TSharedPtr<class STextBlock> SummarySlateText;

	UPROPERTY()
	TObjectPtr<class UTextBlock> SummaryText;

	void RefreshSummary();
};
