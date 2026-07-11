#include "BBMainMenuWidget.h"

#include "Breachborne/Core/BBGameInstance.h"
#include "Breachborne/Core/BBNetworkDevSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	UTextBlock* MakeText(UWidgetTree* Tree, const FString& Label, float Size = 22.0f)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Label));
		Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), Size));
		return Text;
	}

	UButton* MakeButton(UWidgetTree* Tree, const FString& Label)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->AddChild(MakeText(Tree, Label, 18.0f));
		return Button;
	}
}

void UBBMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainMenuRoot"));
	WidgetTree->RootWidget = Root;

	Root->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Breachborne"), 34.0f));
	Root->AddChildToVerticalBox(MakeText(WidgetTree, TEXT("Direct IP"), 18.0f));

	AddressBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	const UBBNetworkDevSettings* Settings = GetDefault<UBBNetworkDevSettings>();
	AddressBox->SetText(FText::FromString(Settings ? Settings->DefaultDirectIPAddress : FString(TEXT("127.0.0.1:7777"))));
	Root->AddChildToVerticalBox(AddressBox);

	UButton* DirectIPButton = MakeButton(WidgetTree, TEXT("Connect"));
	DirectIPButton->OnClicked.AddDynamic(this, &UBBMainMenuWidget::HandleDirectIPClicked);
	Root->AddChildToVerticalBox(DirectIPButton);

	UButton* QuickPlayButton = MakeButton(WidgetTree, TEXT("Local Quick Play"));
	QuickPlayButton->OnClicked.AddDynamic(this, &UBBMainMenuWidget::HandleLocalQuickPlayClicked);
	Root->AddChildToVerticalBox(QuickPlayButton);

	UButton* QuitButton = MakeButton(WidgetTree, TEXT("Quit"));
	QuitButton->OnClicked.AddDynamic(this, &UBBMainMenuWidget::HandleQuitClicked);
	Root->AddChildToVerticalBox(QuitButton);
}

void UBBMainMenuWidget::HandleDirectIPClicked()
{
	if (UBBGameInstance* GI = GetGameInstance<UBBGameInstance>())
	{
		GI->ConnectDirectIP(AddressBox ? AddressBox->GetText().ToString() : FString());
	}
}

void UBBMainMenuWidget::HandleLocalQuickPlayClicked()
{
	if (UBBGameInstance* GI = GetGameInstance<UBBGameInstance>())
	{
		GI->StartLocalQuickPlay();
	}
}

void UBBMainMenuWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
