#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BBPowerDestructibleInterface.h"
#include "BBPowerDestructibleComponent.generated.h"

UCLASS(ClassGroup=(Breachborne), meta=(BlueprintSpawnableComponent))
class BREACHBORNE_API UBBPowerDestructibleComponent : public UActorComponent, public IBBPowerDestructibleInterface
{
	GENERATED_BODY()

public:
	UBBPowerDestructibleComponent();

	virtual void ApplyPowerDestruction_Implementation(const FBBPowerDestructionContext& Context) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Powers")
	bool bDestroyOwner = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Powers")
	bool bHideOwner = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Powers")
	bool bDisableOwnerCollision = true;
};
