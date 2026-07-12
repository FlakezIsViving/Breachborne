#include "BBVoidSwappableComponent.h"

#include "GameFramework/Actor.h"

UBBVoidSwappableComponent::UBBVoidSwappableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBBVoidSwappableComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		Owner->SetReplicates(true);
		Owner->SetReplicateMovement(true);
	}
}
