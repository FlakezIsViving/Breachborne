#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BBVoidSwappableComponent.generated.h"

/** Marker for props that Void Swap is allowed to teleport during playtests. */
UCLASS(ClassGroup=(Breachborne), meta=(BlueprintSpawnableComponent))
class BREACHBORNE_API UBBVoidSwappableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBBVoidSwappableComponent();

protected:
	virtual void BeginPlay() override;
};
