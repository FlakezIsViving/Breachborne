#include "BBWorldItem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "BBInventoryManager.h"
#include "BBEquipmentDefinition.h"
#include "BBItemRegistry.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Breachborne.h"

ABBWorldItem::ABBWorldItem()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Interaction sphere — used for overlap detection
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetSphereRadius(InteractionRange);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionSphere->SetGenerateOverlapEvents(true);
	RootComponent = InteractionSphere;

	// Visual mesh
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	ItemMesh->SetupAttachment(InteractionSphere);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABBWorldItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABBWorldItem, PickupSlotType);
	DOREPLIFETIME(ABBWorldItem, PickupItemID);
	DOREPLIFETIME(ABBWorldItem, PickupAmount);
	DOREPLIFETIME(ABBWorldItem, bIsGoldPickup);
	DOREPLIFETIME(ABBWorldItem, bIsShardPickup);
	DOREPLIFETIME(ABBWorldItem, bIsPowerPickup);
	DOREPLIFETIME(ABBWorldItem, bIsConsumablePickup);
	DOREPLIFETIME(ABBWorldItem, bIsPickedUp);
}

void ABBWorldItem::InitAsEquipment(FName InItemID, EEquipmentSlotType InSlotType)
{
	PickupItemID = InItemID;
	PickupSlotType = InSlotType;
}

void ABBWorldItem::InitAsGold(int32 InAmount)
{
	bIsGoldPickup = true;
	PickupAmount = InAmount;
}

void ABBWorldItem::InitAsShards(int32 InAmount)
{
	bIsShardPickup = true;
	PickupAmount = InAmount;
}

void ABBWorldItem::InitAsConsumable(FName InConsumableID, int32 InCount)
{
	bIsConsumablePickup = true;
	PickupItemID = InConsumableID;
	PickupAmount = InCount;
}

void ABBWorldItem::InitAsPower(FName InPowerID)
{
	bIsPowerPickup = true;
	PickupItemID = InPowerID;
}

void ABBWorldItem::ServerRequestPickup_Implementation(APlayerController* RequestingPlayer)
{
	if (!HasAuthority() || bIsPickedUp || !RequestingPlayer)
	{
		return;
	}

	APawn* PlayerPawn = RequestingPlayer->GetPawn();
	if (!PlayerPawn)
	{
		return;
	}

	// Validate proximity
	const float DistSq = FVector::DistSquared(PlayerPawn->GetActorLocation(), GetActorLocation());
	if (DistSq > FMath::Square(InteractionRange))
	{
		UE_LOG(LogBreachborne, Warning, TEXT("WorldItem: Pickup denied — too far (%.0f > %.0f)"),
			FMath::Sqrt(DistSq), InteractionRange);
		return;
	}

	ABreachbornePlayerState* PS = RequestingPlayer->GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		return;
	}

	bool bPickupSuccess = false;

	if (bIsGoldPickup)
	{
		FBBInventoryManager::AddGold(PS, PickupAmount);
		bPickupSuccess = true;
	}
	else if (bIsShardPickup)
	{
		FBBInventoryManager::AddShards(PS, PickupAmount);
		bPickupSuccess = true;
	}
	else if (PickupSlotType != EEquipmentSlotType::None)
	{
		// Equipment pickup — create a temp definition for now (proper lookup via Registry in 5H)
		UBBEquipmentDefinition* TempDef = NewObject<UBBEquipmentDefinition>();
		TempDef->ItemID = PickupItemID;
		TempDef->SlotType = PickupSlotType;
		// Base stats based on slot type
		if (PickupSlotType == EEquipmentSlotType::Weapon)
		{
			TempDef->BaseStats.AttackPower = 10.0f;
		}
		else if (PickupSlotType == EEquipmentSlotType::Helmet)
		{
			TempDef->BaseStats.MaxHealth = 50.0f;
		}
		else if (PickupSlotType == EEquipmentSlotType::Boots)
		{
			TempDef->BaseStats.MoveSpeed = 25.0f;
		}

		bPickupSuccess = FBBInventoryManager::EquipItem(PS, TempDef);
	}
	else if (bIsConsumablePickup)
	{
		bPickupSuccess = FBBInventoryManager::AddConsumableStack(PS, PickupItemID, FMath::Max(1, PickupAmount));
	}
	else if (bIsPowerPickup)
	{
		bPickupSuccess = FBBInventoryManager::EquipPowerByID(PS, PickupItemID);
		if (bPickupSuccess)
		{
			UE_LOG(LogBreachborne, Log, TEXT("PowerPickup: %s picked up power %s"),
				*PS->GetPlayerName(), *PickupItemID.ToString());
		}
	}

	if (bPickupSuccess)
	{
		bIsPickedUp = true;
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);

		UE_LOG(LogBreachborne, Log, TEXT("WorldItem: %s picked up %s"),
			*PS->GetPlayerName(), *PickupItemID.ToString());
	}
}

void ABBWorldItem::OnRep_IsPickedUp()
{
	if (bIsPickedUp)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}
}
