#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BBMainMenuWidget.generated.h"

class UEditableTextBox;

/** Minimal native main menu for direct-IP testing before polished Blueprint UI exists. */
UCLASS()
class BREACHBORNE_API UBBMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	TObjectPtr<UEditableTextBox> AddressBox;

	UFUNCTION()
	void HandleDirectIPClicked();

	UFUNCTION()
	void HandleLocalQuickPlayClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
