#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BBMantleSurfaceComponent.generated.h"

/**
 * Marker component for island terrain that may trigger abyss mantle recovery.
 */
UCLASS(ClassGroup = "Breachborne", meta = (BlueprintSpawnableComponent))
class BREACHBORNE_API UBBMantleSurfaceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBBMantleSurfaceComponent();
};
