#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBItemTypes.h"
#include "BBWorldItem.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * A physical item in the world that can be picked up by pressing E.
 * Server authoritative: validates proximity and slot availability.
 * Replicates pickup state so item disappears for all clients.
 */
UCLASS()
class BREACHBORNE_API ABBWorldItem : public AActor
{
	GENERATED_BODY()

public:
	ABBWorldItem();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Initialize this pickup with an equipment item */
	void InitAsEquipment(FName InItemID, EEquipmentSlotType InSlotType);

	/** Initialize this pickup as gold */
	void InitAsGold(int32 InAmount);

	/** Initialize this pickup as upgrade shards */
	void InitAsShards(int32 InAmount);

	/** Initialize this pickup as a consumable */
	void InitAsConsumable(FName InConsumableID, int32 InCount);

	/** Initialize this pickup as a power */
	void InitAsPower(FName InPowerID);

	/** Server RPC: client requests to pick up this item */
	UFUNCTION(Server, Reliable)
	void ServerRequestPickup(APlayerController* RequestingPlayer);

	/** Whether this item has been picked up */
	bool IsPickedUp() const { return bIsPickedUp; }

	bool IsPowerPickup() const { return bIsPowerPickup; }
	FName GetPickupItemID() const { return PickupItemID; }

	/** Set the item ID for this pickup (used by spawn logic) */
	void SetPickupItemID(const FName& InID) { PickupItemID = InID; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Items")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Items")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	/** What kind of pickup this is */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Items")
	EEquipmentSlotType PickupSlotType = EEquipmentSlotType::None;

	/** Item ID for equipment, consumable, or power pickups */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Items")
	FName PickupItemID;

	/** Amount for gold/shard/consumable pickups */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Items")
	int32 PickupAmount = 0;

	/** Whether this is a gold pickup vs equipment/consumable/power */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Items")
	bool bIsGoldPickup = false;

	/** Whether this is a shard pickup */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Items")
	bool bIsShardPickup = false;

	/** Whether this is a power pickup */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Items")
	bool bIsPowerPickup = false;

	/** Whether this is a consumable pickup */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Items")
	bool bIsConsumablePickup = false;

	/** Has this item been picked up? */
	UPROPERTY(ReplicatedUsing = OnRep_IsPickedUp, BlueprintReadOnly, Category = "Breachborne|Items")
	bool bIsPickedUp = false;

	UFUNCTION()
	void OnRep_IsPickedUp();

	/** Max distance for pickup interaction */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Items")
	float InteractionRange = 200.0f;
};
