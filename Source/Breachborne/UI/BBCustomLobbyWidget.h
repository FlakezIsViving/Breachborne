#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "Types/SlateEnums.h"
#include "Widgets/SWidget.h"
#include "BBCustomLobbyWidget.generated.h"

UCLASS()
class BREACHBORNE_API UBBCustomLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UBBCustomLobbyWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	TSharedPtr<class STextBlock> HeaderText;
	TSharedPtr<class STextBlock> StatusText;
	TSharedPtr<class SUniformGridPanel> TeamGrid;
	TSharedPtr<class SVerticalBox> SpectatorBox;
	FString LastLobbyFingerprint;

	FReply HandleJoinSlotClicked(int32 TeamID, int32 SlotIndex);
	FReply HandleSpectateClicked();
	FReply HandleAutoFillClicked();
	FReply HandleTeamSizeClicked(int32 TeamSize);
	FReply HandleStartClicked();
	void HandleStormEnabledChanged(ECheckBoxState NewState);
	void HandleDescriptionCommitted(const FText& Text, ETextCommit::Type CommitType);

	void RefreshLobby(bool bForce = false);
	FString BuildLobbyFingerprint() const;
	bool IsLocalPlayerLobbyOwner() const;
};
