#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBRespawnBeacon.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class ABreachbornePlayerState;
class AHunterCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBeaconActivated, ABBRespawnBeacon*, Beacon, ABreachbornePlayerState*, ActivatorPS);

/**
 * Respawn Beacon — a one-time-use world object that revives all dead squadmates of the
 * player who activates it.
 *
 * Placement: spawned by GameMode at designated beacon sites on the map.
 * Interaction: any alive hunter walks within InteractRadius and presses E (interact key).
 * Effect: all dead players on the activator's team are revived at the beacon location.
 * The beacon is then consumed (destroyed).
 *
 * Server-authoritative. Activation via ABreachbornePlayerController server RPC.
 */
UCLASS()
class BREACHBORNE_API ABBRespawnBeacon : public AActor
{
	GENERATED_BODY()

public:
	ABBRespawnBeacon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Server: attempt to activate the beacon by an allied hunter.
	 * Validates proximity, checks that at least one squadmate is dead, then fires OnBeaconActivated.
	 * Returns true if activation succeeded.
	 */
	bool ServerActivateBeacon(AHunterCharacter* Activator);

	UFUNCTION(BlueprintPure, Category = "Breachborne|Beacon")
	bool IsAvailable() const { return bAvailable; }

	float GetInteractRadius() const { return InteractRadius; }

	/** Server only — bound by GameMode to handle squad revival */
	FOnBeaconActivated OnBeaconActivated;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Beacon")
	TObjectPtr<USphereComponent> InteractSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Beacon")
	TObjectPtr<UStaticMeshComponent> BeaconMesh;

	/** Interaction radius — how close a hunter must be to activate */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Beacon")
	float InteractRadius = 250.0f;

private:
	/** Whether the beacon has already been used */
	UPROPERTY(ReplicatedUsing = OnRep_Available)
	bool bAvailable = true;

	UFUNCTION()
	void OnRep_Available();
};
