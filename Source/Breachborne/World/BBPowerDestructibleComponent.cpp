#include "BBPowerDestructibleComponent.h"

#include "GameFramework/Actor.h"

UBBPowerDestructibleComponent::UBBPowerDestructibleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBBPowerDestructibleComponent::ApplyPowerDestruction_Implementation(const FBBPowerDestructionContext& Context)
{
	AActor* Owner = GetOwner();
	if (!Owner || Owner->IsActorBeingDestroyed())
	{
		return;
	}

	if (bDisableOwnerCollision)
	{
		Owner->SetActorEnableCollision(false);
	}

	if (bHideOwner)
	{
		Owner->SetActorHiddenInGame(true);
	}

	if (bDestroyOwner)
	{
		Owner->Destroy();
	}
}
