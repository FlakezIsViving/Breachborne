#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BBFrontendStatusWidget.generated.h"

class UTextBlock;

UCLASS()
class BREACHBORNE_API UBBFrontendStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetStatusText(const FText& NewText);

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UFUNCTION()
	void HandleCancelClicked();
};
