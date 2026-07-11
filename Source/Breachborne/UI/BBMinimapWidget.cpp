#include "BBMinimapWidget.h"

#include "Breachborne/World/BBMapDataAsset.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

void UBBMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MinimapRoot"));
	WidgetTree->RootWidget = Root;

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(FText::FromString(TEXT("MINIMAP")));
	Root->AddChildToVerticalBox(Label);
}

FVector2D UBBMinimapWidget::WorldToMapUV(const FVector& WorldLocation) const
{
	if (!MapData)
	{
		return FVector2D::ZeroVector;
	}

	return MapData->WorldToMapUV(WorldLocation);
}
