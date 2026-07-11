#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Breachborne/World/BBPowerDestructibleInterface.h"
#include "BBVaultInteractable.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class ABBWorldItem;
class ABreachbornePlayerState;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVaultHacked,
	ABBVaultInteractable*, Vault,
	ABreachbornePlayerState*, HackerPS);

/**
 * Vault — a channeled hack interaction.
 *
 * - Player must stand in InteractRadius and hold Interact for HackDuration seconds
 * - Progress cancels if the player moves more than CancelMoveThreshold cm, takes damage,
 *   or leaves the interaction radius
 * - On successful hack: spawns ItemDropClass at vault location, marks vault as looted
 * - Vault does NOT respawn within a match
 */
UCLASS()
class BREACHBORNE_API ABBVaultInteractable : public AActor, public IBBPowerDestructibleInterface
{
	GENERATED_BODY()

public:
	ABBVaultInteractable();

	/** Fired on server when the hack channel completes */
	UPROPERTY(BlueprintAssignable, Category = "Breachborne|PvE")
	FOnVaultHacked OnVaultHacked;

	/** Begin the hack channel for the given player. Called by PlayerController on interact. */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|PvE")
	void ServerBeginHack(ABreachbornePlayerState* HackerPS);

	/** Cancel an in-progress hack (called on damage, movement, or leaving radius). */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|PvE")
	void ServerCancelHack();

	/** Returns true if this vault has already been looted this match */
	UFUNCTION(BlueprintPure, Category = "Breachborne|PvE")
	bool IsLooted() const { return bLooted || bDestroyed; }

	virtual void ApplyPowerDestruction_Implementation(const FBBPowerDestructionContext& Context) override;

	/** Progress 0-1. Replicated so clients can show a progress bar. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|PvE")
	float HackProgress = 0.0f;

	/** True while a hack channel is active */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|PvE")
	bool bHacking = false;

	UPROPERTY(ReplicatedUsing = OnRep_Destroyed, BlueprintReadOnly, Category = "Breachborne|PvE")
	bool bDestroyed = false;

	/** Interaction radius the player must stay inside */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	float InteractRadius = 250.0f;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Seconds to complete the hack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	float HackDuration = 4.0f;

	/** If the hacker moves more than this (cm) from the hack start position, cancel */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	float CancelMoveThreshold = 80.0f;

	/** Item class to spawn on successful hack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	TSubclassOf<ABBWorldItem> ItemDropClass;

	/** Item ID for the vault reward */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|PvE")
	FName VaultItemID = FName(TEXT("Vault_Equipment"));

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE")
	TObjectPtr<UStaticMeshComponent> VaultMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PvE")
	TObjectPtr<USphereComponent> InteractSphere;

private:
	/** 10Hz progress tick */
	void HackTick();

	FTimerHandle HackTickHandle;
	FDelegateHandle HackerHealthHandle;

	UPROPERTY()
	TObjectPtr<ABreachbornePlayerState> CurrentHacker;

	FVector HackStartLocation = FVector::ZeroVector;
	float   ChannelElapsed    = 0.0f;
	bool    bLooted           = false;

	void OnHackerTookDamage(const FOnAttributeChangeData& Data);
	void SpawnReward();
	void ApplyDestroyedPresentation();

	UFUNCTION()
	void OnRep_Destroyed();
};
