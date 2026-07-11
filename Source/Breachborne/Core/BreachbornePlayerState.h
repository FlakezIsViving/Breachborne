#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
#include "Breachborne/Items/BBInventoryTypes.h"
#include "Breachborne/Core/BreachborneTypes.h"
#include "BreachbornePlayerState.generated.h"

class UAbilitySystemComponent;
class UBBAbilitySystemComponent;
class UBBHealthSet;
class UBBCombatSet;
class UBBMovementSet;
class UBBWispHealthSet;
struct FOnAttributeChangeData;

/**
 * Proxy structs for attribute replication.
 * AttributeSets are UObject subobjects on PlayerState. UE's FObjectReplicator can misroute
 * subobject property updates between PlayerState instances (CDO-based name collision).
 * Instead, we replicate attribute values as plain floats on the PlayerState actor itself,
 * then push them to the local AttributeSet + ASC aggregator in OnRep.
 */
USTRUCT()
struct FRepHealthData
{
	GENERATED_BODY()

	UPROPERTY()
	float Health = 500.0f;

	UPROPERTY()
	float MaxHealth = 500.0f;

	UPROPERTY()
	float Shield = 0.0f;

	UPROPERTY()
	float MaxShield = 200.0f;

	UPROPERTY()
	float Armor = 0.0f;
};

USTRUCT()
struct FRepCombatData
{
	GENERATED_BODY()

	UPROPERTY()
	float AttackPower = 50.0f;

	UPROPERTY()
	float AbilityPower = 50.0f;

	UPROPERTY()
	float CooldownReduction = 0.0f;

	UPROPERTY()
	float CritChance = 0.0f;

	UPROPERTY()
	float CritMultiplier = 1.5f;
};

USTRUCT()
struct FRepMovementData
{
	GENERATED_BODY()

	UPROPERTY()
	float MoveSpeed = 600.0f;

	UPROPERTY()
	float GlideSpeed = 800.0f;

	UPROPERTY()
	float DashDistance = 500.0f;
};

USTRUCT()
struct FRepWispData
{
	GENERATED_BODY()

	UPROPERTY()
	float WispHP = 100.0f;

	UPROPERTY()
	float MaxWispHP = 100.0f;
};

/**
 * Replicated to all clients. Holds per-player data: team, hunter ID, level, XP, kills, alive status.
 * Owns the AbilitySystemComponent and all AttributeSets (ASC on PlayerState survives pawn death/respawn).
 */
UCLASS()
class BREACHBORNE_API ABreachbornePlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABreachbornePlayerState();

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Typed getter for the Breachborne ASC */
	UFUNCTION(BlueprintPure, Category = "Breachborne|GAS")
	UBBAbilitySystemComponent* GetBBAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** Typed getters for attribute sets */
	UFUNCTION(BlueprintPure, Category = "Breachborne|GAS")
	UBBHealthSet* GetHealthSet() const { return HealthSet; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|GAS")
	UBBCombatSet* GetCombatSet() const { return CombatSet; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|GAS")
	UBBMovementSet* GetMovementSet() const { return MovementSet; }

	// --- Getters ---
	UFUNCTION(BlueprintPure, Category = "Breachborne|Player")
	int32 GetTeamID() const { return TeamID; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Player")
	int32 GetHunterID() const { return HunterID; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Player")
	int32 GetHunterLevel() const { return Level; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Player")
	int32 GetXP() const { return XP; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Player")
	int32 GetKills() const { return Kills; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Player")
	bool GetIsAlive() const { return bIsAlive; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Match")
	bool IsReadyForMatch() const { return bReadyForMatch; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Match")
	bool IsHunterLocked() const { return bHunterLocked; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Lobby")
	int32 GetLobbySlotIndex() const { return LobbySlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Lobby")
	bool IsLobbySpectator() const { return bIsLobbySpectator; }

	// --- Setters (server only) ---
	void SetTeamID(int32 NewTeamID);
	void SetHunterID(int32 NewHunterID);
	void SetReadyForMatch(bool bReady);
	void SetHunterLocked(bool bLocked);
	void SetLobbySlotIndex(int32 NewSlotIndex);
	void SetIsSpectator(bool bSpectator);
	void SetLevel(int32 NewLevel);
	void AddXP(int32 Amount);
	void AddKill();
	void SetIsAlive(bool bAlive);

	/**
	 * Server only: called from HealthSet::PostGameplayEffectExecute to explicitly
	 * copy current health attribute values into the replicated proxy struct.
	 * This bypasses GAS delegate machinery which may not fire reliably from within
	 * the GameplayEffect execution pipeline.
	 */
	void UpdateHealthProxy();

	// --- Inventory ---

	/** Read-only access to replicated inventory data */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Inventory")
	const FRepInventoryData& GetInventoryData() const { return Rep_InventoryData; }

	/** Server only: mutable access to inventory data for the InventoryManager */
	FRepInventoryData& GetInventoryDataMutable();

	// --- Equipment GE Handle Tracking (server only, NOT replicated) ---

	/** Active GE handle for current weapon bonuses */
	FActiveGameplayEffectHandle ActiveWeaponGEHandle;

	/** Active GE handle for current helmet bonuses */
	FActiveGameplayEffectHandle ActiveHelmetGEHandle;

	/** Active GE handle for current boots bonuses */
	FActiveGameplayEffectHandle ActiveBootsGEHandle;

	/** Active GE handle for current armor bonuses */
	FActiveGameplayEffectHandle ActiveArmorGEHandle;

	/** Active GE handle for power 1 stat bonuses */
	FActiveGameplayEffectHandle ActivePower1GEHandle;

	/** Active GE handle for power 2 stat bonuses */
	FActiveGameplayEffectHandle ActivePower2GEHandle;

	/** Ability spec handle for power 1 granted ability */
	FGameplayAbilitySpecHandle GrantedPower1AbilityHandle;

	/** Ability spec handle for power 2 granted ability */
	FGameplayAbilitySpecHandle GrantedPower2AbilityHandle;

protected:
	// --- GAS Components (created as subobjects in constructor) ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|GAS")
	TObjectPtr<UBBAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|GAS")
	TObjectPtr<UBBHealthSet> HealthSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|GAS")
	TObjectPtr<UBBCombatSet> CombatSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|GAS")
	TObjectPtr<UBBMovementSet> MovementSet;

	// --- Replicated Player Data ---

	UPROPERTY(ReplicatedUsing = OnRep_TeamID, BlueprintReadOnly, Category = "Breachborne|Player")
	int32 TeamID = -1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Player")
	int32 HunterID = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Player")
	int32 Level = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Player")
	int32 XP = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Player")
	int32 Kills = 0;

	UPROPERTY(ReplicatedUsing = OnRep_IsAlive, BlueprintReadOnly, Category = "Breachborne|Player")
	bool bIsAlive = true;

	UPROPERTY(ReplicatedUsing = OnRep_ReadyForMatch, BlueprintReadOnly, Category = "Breachborne|Match")
	bool bReadyForMatch = false;

	UPROPERTY(ReplicatedUsing = OnRep_HunterLocked, BlueprintReadOnly, Category = "Breachborne|Match")
	bool bHunterLocked = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Lobby")
	int32 LobbySlotIndex = -1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Lobby")
	bool bIsLobbySpectator = false;

	UFUNCTION()
	void OnRep_TeamID();

	UFUNCTION()
	void OnRep_IsAlive();

	UFUNCTION()
	void OnRep_ReadyForMatch();

	UFUNCTION()
	void OnRep_HunterLocked();

	// --- Proxy Attribute Replication ---
	// Replicated through PlayerState actor properties (NOT through AttributeSet subobjects)
	// to avoid UE's FObjectReplicator misrouting between PlayerState instances.

	UPROPERTY(ReplicatedUsing = OnRep_HealthData)
	FRepHealthData Rep_HealthData;

	UPROPERTY(ReplicatedUsing = OnRep_CombatData)
	FRepCombatData Rep_CombatData;

	UPROPERTY(ReplicatedUsing = OnRep_MovementData)
	FRepMovementData Rep_MovementData;

	// --- Replicated Inventory Data ---

	UPROPERTY(ReplicatedUsing = OnRep_InventoryData)
	FRepInventoryData Rep_InventoryData;

	UFUNCTION()
	void OnRep_InventoryData();

	UFUNCTION()
	void OnRep_HealthData();

	UFUNCTION()
	void OnRep_CombatData();

	UFUNCTION()
	void OnRep_MovementData();

	/** Server: bind to ASC attribute change delegates to keep proxy structs in sync */
	void BindAttributeChangeCallbacks();

	/** Server: generic handler for any attribute change — copies new value to the matching proxy field */
	void OnAnyAttributeChanged(const FOnAttributeChangeData& Data);

public:
	// --- Stubs for in-progress features ---
	EPlayerLifeState GetLifeState() const { return bIsAlive ? EPlayerLifeState::Alive : EPlayerLifeState::Dead; }
	const FRepHealthData& GetHealthData() const { return Rep_HealthData; }
	const FRepCombatData& GetCombatData() const { return Rep_CombatData; }
	const FRepMovementData& GetMovementData() const { return Rep_MovementData; }
	const FRepWispData& GetWispData() const { static FRepWispData Dummy; return Dummy; }
	int32 GetKnocks() const { return 0; }
	UBBWispHealthSet* GetWispHealthSet() const { return nullptr; }
	void UpdateWispHealthProxy() {}
};
