#include "BBFrontendStatusWidget.h"

#include "Breachborne/Core/BBGameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

void UBBFrontendStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StatusRoot"));
	WidgetTree->RootWidget = Root;

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StatusText->SetText(FText::FromString(TEXT("Connecting...")));
	Root->AddChildToVerticalBox(StatusText);

	UButton* CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* CancelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CancelText->SetText(FText::FromString(TEXT("Cancel")));
	CancelButton->AddChild(CancelText);
	CancelButton->OnClicked.AddDynamic(this, &UBBFrontendStatusWidget::HandleCancelClicked);
	Root->AddChildToVerticalBox(CancelButton);
}

void UBBFrontendStatusWidget::SetStatusText(const FText& NewText)
{
	if (StatusText)
	{
		StatusText->SetText(NewText);
	}
}

void UBBFrontendStatusWidget::HandleCancelClicked()
{
	if (UBBGameInstance* GI = GetGameInstance<UBBGameInstance>())
	{
		GI->ShowMainMenu();
	}
}
