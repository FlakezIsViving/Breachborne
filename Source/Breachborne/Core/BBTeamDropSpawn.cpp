#include "BBTeamDropSpawn.h"

ABBTeamDropSpawn::ABBTeamDropSpawn()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

FTransform ABBTeamDropSpawn::GetSpawnTransformForSlot(int32 SlotIndex, int32 SquadSize) const
{
	const int32 SafeSquadSize = FMath::Max(1, SquadSize);
	const float CenterOffset = (static_cast<float>(SafeSquadSize - 1) * FormationSpacing) * 0.5f;
	const FVector RightOffset = GetActorRightVector() * (static_cast<float>(SlotIndex) * FormationSpacing - CenterOffset);
	const FVector DropLocation = GetActorLocation() + RightOffset + FVector(0.0f, 0.0f, DropHeight);

	return FTransform(GetActorRotation(), DropLocation);
}
