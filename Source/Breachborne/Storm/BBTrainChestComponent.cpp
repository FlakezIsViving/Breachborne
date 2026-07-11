#include "BBTrainChestComponent.h"
#include "Net/UnrealNetwork.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Items/BBChestLootTable.h"

UBBTrainChestComponent::UBBTrainChestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// Default 4-square pattern
	FrontPattern.Init(false, 4);
	SegmentPatterns.Init(false, 4);
}

void UBBTrainChestComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBBTrainChestComponent, FrontPattern);
	DOREPLIFETIME(UBBTrainChestComponent, SegmentPatterns);
	DOREPLIFETIME(UBBTrainChestComponent, bIsLocked);
}

void UBBTrainChestComponent::GeneratePattern()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// Generate random 4-bool front pattern
	FrontPattern.Empty(4);
	for (int32 i = 0; i < 4; ++i)
	{
		FrontPattern.Add(FMath::RandBool());
	}

	// Generate random segment patterns (1 per segment)
	SegmentPatterns.Empty(4);
	for (int32 i = 0; i < 4; ++i)
	{
		SegmentPatterns.Add(FMath::RandBool());
	}

	bIsLocked = true;

	UE_LOG(LogBreachborne, Log, TEXT("TrainChest: Generated new pattern [%d%d%d%d]"),
		FrontPattern[0] ? 1 : 0, FrontPattern[1] ? 1 : 0,
		FrontPattern[2] ? 1 : 0, FrontPattern[3] ? 1 : 0);
}

bool UBBTrainChestComponent::TryUnlock(const TArray<bool>& PlayerAttempt)
{
	if (!GetOwner()->HasAuthority() || !bIsLocked)
	{
		return false;
	}

	// Validate attempt matches front pattern
	if (PlayerAttempt.Num() != FrontPattern.Num())
	{
		UE_LOG(LogBreachborne, Log, TEXT("TrainChest: Unlock attempt failed — wrong length (%d vs %d)"),
			PlayerAttempt.Num(), FrontPattern.Num());
		return false;
	}

	for (int32 i = 0; i < FrontPattern.Num(); ++i)
	{
		if (PlayerAttempt[i] != FrontPattern[i])
		{
			UE_LOG(LogBreachborne, Log, TEXT("TrainChest: Unlock attempt failed — mismatch at index %d"), i);
			return false;
		}
	}

	// Success!
	bIsLocked = false;
	SpawnLoot();
	OnUnlocked.Broadcast();

	UE_LOG(LogBreachborne, Log, TEXT("TrainChest: Unlocked successfully!"));
	return true;
}

void UBBTrainChestComponent::OnRep_FrontPattern()
{
	// Client UI update hook
}

void UBBTrainChestComponent::OnRep_SegmentPatterns()
{
	// Client UI update hook
}

void UBBTrainChestComponent::OnRep_Locked()
{
	// Client UI update hook — play unlock VFX
	if (!bIsLocked)
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("TrainChest: Unlocked (client notification)"));
	}
}

void UBBTrainChestComponent::SpawnLoot()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	const FVector SpawnLocation = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
	const int32 SpawnedCount = LootTable
		? LootTable->SpawnLoot(GetWorld(), SpawnLocation, ChestTier)
		: UBBChestLootTable::SpawnFallbackLoot(GetWorld(), SpawnLocation, ChestTier);
	UE_LOG(LogBreachborne, Log, TEXT("TrainChest: Spawned %d loot pickups at %s"), SpawnedCount, *SpawnLocation.ToString());
}
