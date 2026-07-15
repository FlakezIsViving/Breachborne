#include "BBVoidSwappableComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

UBBVoidSwappableComponent::UBBVoidSwappableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBBVoidSwappableComponent::BeginPlay()
{
	Super::BeginPlay();
	PrepareOwnerForSwap();
}

void UBBVoidSwappableComponent::PrepareOwnerForSwap()
{
	if (AActor* Owner = GetOwner())
	{
		if (USceneComponent* Root = Owner->GetRootComponent())
		{
			Root->SetMobility(EComponentMobility::Movable);
		}
		Owner->SetReplicates(true);
		Owner->SetReplicateMovement(true);
		Owner->ForceNetUpdate();
	}
}
