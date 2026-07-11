#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BBMapDataAsset.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FBBMinimapIconDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Minimap")
	FName IconID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Minimap")
	TObjectPtr<UTexture2D> IconTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Minimap")
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Minimap")
	float Size = 12.0f;
};

UCLASS(BlueprintType)
class BREACHBORNE_API UBBMapDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Minimap")
	TObjectPtr<UTexture2D> MapTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Minimap")
	FVector2D WorldMin = FVector2D(-10000.0f, -10000.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Minimap")
	FVector2D WorldMax = FVector2D(10000.0f, 10000.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Minimap")
	TArray<FBBMinimapIconDefinition> Icons;

	UFUNCTION(BlueprintPure, Category = "Breachborne|Minimap")
	FVector2D WorldToMapUV(const FVector& WorldLocation) const;
};
