#include "BBShopkeeper.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "BBInventoryManager.h"
#include "BBEquipmentDefinition.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Breachborne.h"

ABBShopkeeper::ABBShopkeeper()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetSphereRadius(InteractionRange);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionSphere->SetGenerateOverlapEvents(true);
	RootComponent = InteractionSphere;

	ShopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShopMesh"));
	ShopMesh->SetupAttachment(InteractionSphere);
	ShopMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABBShopkeeper::ServerRequestPurchase_Implementation(APlayerController* RequestingPlayer, int32 ListingIndex)
{
	if (!HasAuthority() || !RequestingPlayer)
	{
		return;
	}

	// Validate listing index
	if (ListingIndex < 0 || ListingIndex >= Listings.Num())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Shopkeeper: Invalid listing index %d"), ListingIndex);
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
		UE_LOG(LogBreachborne, Warning, TEXT("Shopkeeper: Purchase denied — too far"));
		return;
	}

	ABreachbornePlayerState* PS = RequestingPlayer->GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		return;
	}

	const FShopListing& Listing = Listings[ListingIndex];

	// Validate gold balance
	const FRepInventoryData& Inv = PS->GetInventoryData();
	if (Inv.Gold < Listing.GoldCost)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Shopkeeper: %s cannot afford %s (need %d, have %d)"),
			*PS->GetPlayerName(), *Listing.ItemID.ToString(), Listing.GoldCost, Inv.Gold);
		return;
	}

	// Create temporary equipment definition (proper lookup via Registry in 5H)
	UBBEquipmentDefinition* TempDef = NewObject<UBBEquipmentDefinition>();
	TempDef->ItemID = Listing.ItemID;
	TempDef->SlotType = Listing.SlotType;
	if (Listing.SlotType == EEquipmentSlotType::Weapon)
	{
		TempDef->BaseStats.AttackPower = 10.0f;
	}
	else if (Listing.SlotType == EEquipmentSlotType::Helmet)
	{
		TempDef->BaseStats.MaxHealth = 50.0f;
	}
	else if (Listing.SlotType == EEquipmentSlotType::Boots)
	{
		TempDef->BaseStats.MoveSpeed = 25.0f;
	}

	// Equip and deduct gold
	if (FBBInventoryManager::EquipItem(PS, TempDef))
	{
		FBBInventoryManager::AddGold(PS, -Listing.GoldCost);

		UE_LOG(LogBreachborne, Log, TEXT("Shopkeeper: %s purchased %s for %d gold"),
			*PS->GetPlayerName(), *Listing.ItemID.ToString(), Listing.GoldCost);
	}
}
