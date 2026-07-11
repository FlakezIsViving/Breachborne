#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBTeamDropSpawn.generated.h"

/**
 * Optional placed squad drop anchor. If the map has fewer anchors than teams,
 * GameMode falls back to generated ring positions.
 */
UCLASS()
class BREACHBORNE_API ABBTeamDropSpawn : public AActor
{
	GENERATED_BODY()

public:
	ABBTeamDropSpawn();

	UFUNCTION(BlueprintPure, Category = "Breachborne|Spawning")
	FTransform GetSpawnTransformForSlot(int32 SlotIndex, int32 SquadSize) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Spawning")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Spawning")
	float FormationSpacing = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Spawning")
	float DropHeight = 2200.0f;
};
