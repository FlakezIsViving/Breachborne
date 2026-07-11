#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBChestLootTable.h"
#include "BBChestActor.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class ABreachbornePlayerState;

UENUM(BlueprintType)
enum class EBBChestState : uint8
{
	Closed	UMETA(DisplayName = "Closed"),
	Opening	UMETA(DisplayName = "Opening"),
	Opened	UMETA(DisplayName = "Opened"),
	Expired	UMETA(DisplayName = "Expired")
};

UENUM(BlueprintType)
enum class EBBChestType : uint8
{
	Free		UMETA(DisplayName = "Free"),
	YellowKey	UMETA(DisplayName = "Yellow Key"),
	RedKey		UMETA(DisplayName = "Red Key"),
	Armor		UMETA(DisplayName = "Armor"),
	SupplyDrop	UMETA(DisplayName = "Supply Drop"),
	CustomKey	UMETA(DisplayName = "Custom Key")
};

UCLASS()
class BREACHBORNE_API ABBChestActor : public AActor
{
	GENERATED_BODY()

public:
	ABBChestActor();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Breachborne|Chest")
	EBBChestState GetChestState() const { return ChestState; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Chest")
	float GetOpenProgress() const { return OpenProgress; }

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Chest")
	void ConfigureChest(EBBChestType InChestType, EBBChestTier InChestTier, float InOpenDuration, UBBChestLootTable* InLootTable, float InInteractRadius);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest")
	TObjectPtr<USphereComponent> InteractZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest")
	TObjectPtr<USceneComponent> ClosedVisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest")
	TObjectPtr<UStaticMeshComponent> ClosedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest")
	TObjectPtr<USceneComponent> OpenVisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest")
	TObjectPtr<UStaticMeshComponent> OpenMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest")
	TObjectPtr<UStaticMeshComponent> ProgressVisual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest")
	TObjectPtr<USceneComponent> LootSpawnRoot;

	UPROPERTY(ReplicatedUsing = OnRep_ChestState, BlueprintReadOnly, Category = "Breachborne|Chest")
	EBBChestState ChestState = EBBChestState::Closed;

	UPROPERTY(ReplicatedUsing = OnRep_OpenProgress, BlueprintReadOnly, Category = "Breachborne|Chest")
	float OpenProgress = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest")
	EBBChestType ChestType = EBBChestType::Free;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest")
	EBBChestTier ChestTier = EBBChestTier::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest|Access", meta = (EditCondition = "ChestType == EBBChestType::CustomKey"))
	FName RequiredKeyItemID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest|Access", meta = (ClampMin = "1", EditCondition = "ChestType == EBBChestType::YellowKey || ChestType == EBBChestType::RedKey || ChestType == EBBChestType::CustomKey"))
	int32 RequiredKeyCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest|Access", meta = (EditCondition = "ChestType == EBBChestType::YellowKey || ChestType == EBBChestType::RedKey || ChestType == EBBChestType::CustomKey"))
	bool bConsumeKeyOnOpen = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest", meta = (ClampMin = "0.1"))
	float OpenDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest", meta = (ClampMin = "0.1"))
	float OpenDespawnDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest", meta = (ClampMin = "50.0"))
	float InteractRadius = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breachborne|Chest")
	TObjectPtr<UBBChestLootTable> LootTable;

private:
	UFUNCTION()
	void OnRep_ChestState();

	UFUNCTION()
	void OnRep_OpenProgress();

	void OpeningTick();
	void OpenChest();
	void ExpireOpenChest();
	void UpdateVisualState();
	void UpdateProgressVisual();
	ABreachbornePlayerState* FindValidOpener() const;
	FName ResolveRequiredKeyItemID() const;
	bool DoesChestRequireKey() const;
	int32 SpawnLoot();

	FTimerHandle OpeningTickHandle;
	FTimerHandle ExpireHandle;
	TWeakObjectPtr<ABreachbornePlayerState> OpeningPlayerState;
};
