#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBPowerDeployableActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class BREACHBORNE_API ABBPowerDeployableActor : public AActor
{
	GENERATED_BODY()

public:
	ABBPowerDeployableActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Powers")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Powers")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
