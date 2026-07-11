#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BBMinimapWidget.generated.h"

class UBBMapDataAsset;

/** UMG minimap shell. Blueprints can replace the visuals while keeping these map helpers. */
UCLASS()
class BREACHBORNE_API UBBMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Minimap")
	void SetMapData(UBBMapDataAsset* InMapData) { MapData = InMapData; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Minimap")
	FVector2D WorldToMapUV(const FVector& WorldLocation) const;

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Minimap", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBBMapDataAsset> MapData;
};
