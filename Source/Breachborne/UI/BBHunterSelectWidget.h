#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"
#include "BBHunterSelectWidget.generated.h"

class UTextBlock;

UCLASS()
class BREACHBORNE_API UBBHunterSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBBHunterSelectWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	TSharedPtr<class STextBlock> StatusSlateText;

	UPROPERTY()
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UFUNCTION() void SelectHunter1();
	UFUNCTION() void SelectHunter2();
	UFUNCTION() void SelectHunter3();
	UFUNCTION() void SelectHunter4();
	UFUNCTION() void SelectHunter5();
	UFUNCTION() void SelectHunter6();
	UFUNCTION() void ToggleReady();

	FReply HandleSelectHunterClicked(int32 HunterID);
	FReply HandleReadyClicked();
	void SelectHunter(int32 HunterID);
	void RefreshStatus();
};
