#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BBPowerDestructibleInterface.generated.h"

class AActor;

USTRUCT(BlueprintType)
struct FBBPowerDestructionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Powers")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Powers")
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Powers")
	float Radius = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Powers")
	FName PowerID = NAME_None;
};

UINTERFACE(BlueprintType)
class UBBPowerDestructibleInterface : public UInterface
{
	GENERATED_BODY()
};

class BREACHBORNE_API IBBPowerDestructibleInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Breachborne|Powers")
	void ApplyPowerDestruction(const FBBPowerDestructionContext& Context);
};
