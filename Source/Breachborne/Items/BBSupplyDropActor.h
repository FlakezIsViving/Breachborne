#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBChestActor.h"
#include "BBSupplyDropActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UBBChestLootTable;

UCLASS()
class BREACHBORNE_API ABBSupplyDropActor : public AActor
{
	GENERATED_BODY()

public:
	ABBSupplyDropActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Breachborne|Supply Drop")
	bool HasLanded() const { return bHasLanded; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Supply Drop")
	float GetDropProgress() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop")
	TObjectPtr<USceneComponent> DropVisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop")
	TObjectPtr<UStaticMeshComponent> DropMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop")
	TObjectPtr<UStaticMeshComponent> LandingIndicator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop")
	TObjectPtr<USceneComponent> ChestSpawnRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop")
	TSubclassOf<ABBChestActor> ChestActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop", meta = (ClampMin = "100.0"))
	float DropHeight = 2600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop", meta = (ClampMin = "0.1"))
	float FallDuration = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop", meta = (ClampMin = "50.0"))
	float LandingIndicatorRadius = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop|Chest")
	EBBChestTier ChestTier = EBBChestTier::SupplyDrop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop|Chest", meta = (ClampMin = "0.1"))
	float ChestOpenDuration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop|Chest", meta = (ClampMin = "50.0"))
	float ChestInteractRadius = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop|Chest")
	TObjectPtr<UBBChestLootTable> LootTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Supply Drop|Lifecycle", meta = (ClampMin = "0.0"))
	float DespawnDelayAfterLanding = 2.0f;

	UPROPERTY(ReplicatedUsing = OnRep_DropState, BlueprintReadOnly, Category = "Breachborne|Supply Drop")
	bool bHasLanded = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Supply Drop")
	float ReplicatedDropElapsed = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Supply Drop")
	TObjectPtr<ABBChestActor> SpawnedChest;

private:
	UFUNCTION()
	void OnRep_DropState();

	void LandDrop();
	void SpawnSupplyChest();
	void UpdateDropVisual();

	FTimerHandle DespawnHandle;
};
