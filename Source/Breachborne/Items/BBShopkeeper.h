#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBItemTypes.h"
#include "BBShopkeeper.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * Fixed-location shopkeeper actor. Players interact to buy items with gold.
 * Server validates proximity, gold balance, and listing validity before completing purchase.
 */
UCLASS()
class BREACHBORNE_API ABBShopkeeper : public AActor
{
	GENERATED_BODY()

public:
	ABBShopkeeper();

	/** Server RPC: client requests to purchase an item by listing index */
	UFUNCTION(Server, Reliable)
	void ServerRequestPurchase(APlayerController* RequestingPlayer, int32 ListingIndex);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Items")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Items")
	TObjectPtr<UStaticMeshComponent> ShopMesh;

	/** Items available for purchase at this shopkeeper */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Items")
	TArray<FShopListing> Listings;

	/** Max interaction distance */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	float InteractionRange = 300.0f;
};
