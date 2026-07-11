#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBMostWantedCrown.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class ABreachbornePlayerState;
class AHunterCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrownPickedUp, ABBMostWantedCrown*, Crown, ABreachbornePlayerState*, HolderPS);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCrownHolderKilled, ABBMostWantedCrown*, Crown, ABreachbornePlayerState* , KillerPS);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrownTimerExpired, ABBMostWantedCrown*, Crown);

/**
 * Most Wanted Crown — a floating world pickup that confers risk and reward.
 *
 * Spawned at match start at a configurable world location (default: map center).
 * First hunter to walk through it becomes the "Most Wanted" holder.
 *
 * While held:
 *  - Holder has State.MostWanted tag on their ASC (visible to all enemy vision cones).
 *  - Every 30 seconds, GameMode broadcasts the holder's location to all clients (ping alert).
 *  - A 2-minute countdown runs. If it expires, the holder's dead squadmates are revived at
 *    the holder's location (or at the crown location if holder is dead).
 *
 * On holder death (any cause):
 *  - Crown drops at holder's death location (or pre-wisp location for abyss deaths).
 *  - Killing team gets a "Most Wanted Kill" reward: all their dead squadmates are revived.
 *  - Crown becomes available for pickup again (reset timer).
 *
 * Server-authoritative. Pickup is proximity overlap; no button press required.
 */
UCLASS()
class BREACHBORNE_API ABBMostWantedCrown : public AActor
{
	GENERATED_BODY()

public:
	ABBMostWantedCrown();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** Server: call when the holder dies. Drops crown and fires OnCrownHolderKilled. */
	void ServerOnHolderDied(ABreachbornePlayerState* KillerPS, const FVector& DeathLocation);

	/** Server: reset the crown to an unclaimed state at a new world location */
	void ServerDropCrown(const FVector& NewLocation);

	UFUNCTION(BlueprintPure, Category = "Breachborne|Crown")
	ABreachbornePlayerState* GetCrownHolder() const { return CrownHolder; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Crown")
	bool IsClaimed() const { return bClaimed; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Crown")
	float GetCrownTimer() const { return CrownTimerRemaining; }

	// --- Delegates (server only, bound by GameMode) ---
	FOnCrownPickedUp OnCrownPickedUp;
	FOnCrownHolderKilled OnCrownHolderKilled;
	FOnCrownTimerExpired OnCrownTimerExpired;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Crown")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Crown")
	TObjectPtr<UStaticMeshComponent> CrownMesh;

	/** Total crown countdown in seconds before squad-revive reward triggers */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crown")
	float CrownDuration = 120.0f;

	/** How often (seconds) the holder's location is broadcast to all clients */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crown")
	float BroadcastInterval = 30.0f;

	/** Pickup radius — walking within this range claims the unclaimed crown */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crown")
	float PickupRadius = 200.0f;

private:
	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Ticks the crown countdown and fires periodic broadcasts */
	void CrownCountdownTick();

	/** Called every BroadcastInterval — GameMode should multicast location ping to clients */
	void FireLocationBroadcast();

	// --- Replicated state ---

	/** The player currently holding the crown (null if unclaimed) */
	UPROPERTY(ReplicatedUsing = OnRep_CrownHolder)
	TObjectPtr<ABreachbornePlayerState> CrownHolder;

	UFUNCTION()
	void OnRep_CrownHolder();

	/** Whether the crown is claimed */
	UPROPERTY(ReplicatedUsing = OnRep_Claimed)
	bool bClaimed = false;

	UFUNCTION()
	void OnRep_Claimed();

	/** Remaining seconds on the crown timer (replicated at low frequency for HUD) */
	UPROPERTY(Replicated)
	float CrownTimerRemaining = 0.0f;

	/** Server: elapsed time since last broadcast */
	float TimeSinceLastBroadcast = 0.0f;

	FTimerHandle CountdownTickHandle;
	FTimerHandle BroadcastHandle;
};
