#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Breachborne/Items/BBItemTypes.h"
#include "Breachborne/Core/BreachborneTypes.h"
#include "BreachbornePlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UBBAbilitySystemComponent;
class UBBGameplayAbility;
class ABBBasecampActor;
class ABBWispPawn;
class ABBDeathboxActor;
class UUserWidget;
struct FInputActionValue;
struct FOnAttributeChangeData;

/**
 * Top-down player controller. Handles WASD movement, cursor-to-world aiming,
 * Enhanced Input binding, and ability input routing to the GAS ASC.
 * Exists on server (all) and owning client only.
 */
UCLASS()
class BREACHBORNE_API ABreachbornePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABreachbornePlayerController();

	virtual void PlayerTick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Breachborne|Basecamp")
	bool IsBasecampRecallActive() const { return bBasecampRecallActive; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Basecamp")
	float GetBasecampRecallProgress() const { return BasecampRecallProgress; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Basecamp")
	ABBBasecampActor* GetActiveRecallBasecamp() const { return ActiveRecallBasecamp; }

	bool ConsumePendingTargetedPowerActivation(const FGameplayTag& InputTag, FVector& OutTargetLocation);

	/** Local HUD accessors for owner-only ability range previews. */
	const UBBGameplayAbility* GetActiveRangePreviewAbility() const;
	bool GetRangePreviewCursorLocation(FVector& OutLocation) const;

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Match")
	void RequestHunterSelection(int32 HunterID);

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Match")
	void RequestReadyState(bool bReady);

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Lobby")
	void RequestJoinLobbySlot(int32 TeamID, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Lobby")
	void RequestJoinSpectators();

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Lobby")
	void RequestAutoFillLobbySlot();

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Lobby")
	void RequestSetLobbyTeamSize(int32 TeamSize);

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Lobby")
	void RequestSetLobbyDescription(const FString& Description);

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Lobby")
	void RequestSetStormShiftPreset(FName PresetID);

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Lobby")
	void RequestStartLobbyMatch();

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Map")
	void ToggleFullscreenMap();

	UFUNCTION(Exec)
	void NetFlowDump();

	UFUNCTION(Exec)
	void NetFlowSelectHunter(int32 HunterID);

	UFUNCTION(Exec)
	void NetFlowReady(bool bReady = true);

	UFUNCTION(Exec)
	void NetFlowValidateHunter(int32 HunterID);

	UFUNCTION(Exec)
	void LobbyDump();

	UFUNCTION(Exec)
	void LobbyJoinSlot(int32 TeamID, int32 SlotIndex);

	UFUNCTION(Exec)
	void LobbySetTeamSize(int32 TeamSize);

	UFUNCTION(Exec)
	void LobbyStart();

	UFUNCTION(Client, Reliable)
	void ClientEnterFrontendPhase(EMatchPhase Phase);

	UFUNCTION(Client, Reliable)
	void ClientEnterGameplayPhase();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Enhanced Input mapping context */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Move input action (Axis2D — WASD) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input")
	TObjectPtr<UInputAction> MoveAction;

	/** Jump input action (Digital — Space) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input")
	TObjectPtr<UInputAction> JumpAction;

	// --- Ability Input Actions ---

	/** Primary fire (LMB) — held input */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input|Abilities")
	TObjectPtr<UInputAction> PrimaryFireAction;

	/** Secondary fire (RMB) — discrete */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input|Abilities")
	TObjectPtr<UInputAction> SecondaryFireAction;

	/** Dash/Dodge (Shift) — discrete */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input|Abilities")
	TObjectPtr<UInputAction> DashAction;

	/** Ability Q — discrete */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input|Abilities")
	TObjectPtr<UInputAction> AbilityQAction;

	/** Ultimate R — discrete */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input|Abilities")
	TObjectPtr<UInputAction> UltimateAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input|Powers")
	TObjectPtr<UInputAction> Power1Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input|Powers")
	TObjectPtr<UInputAction> Power2Action;

	// --- Interaction Input ---

	/** Interact (E key) — pick up items, interact with shopkeepers, etc. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input")
	TObjectPtr<UInputAction> InteractAction;

	/** Recall to the team's active basecamp (B key default). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input")
	TObjectPtr<UInputAction> RecallAction;

	/** Consumable hotkeys (1-4 keys) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input")
	TObjectPtr<UInputAction> Consumable1Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input")
	TObjectPtr<UInputAction> Consumable2Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input")
	TObjectPtr<UInputAction> Consumable3Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input")
	TObjectPtr<UInputAction> Consumable4Action;

private:
	/** Handle WASD movement input */
	void HandleMoveInput(const FInputActionValue& Value);

	/** Handle jump pressed */
	void HandleJumpStarted(const FInputActionValue& Value);

	/** Handle jump released */
	void HandleJumpStopped(const FInputActionValue& Value);

	// --- Ability Input Handlers ---
	void HandlePrimaryFireStarted(const FInputActionValue& Value);
	void HandlePrimaryFireCompleted(const FInputActionValue& Value);
	void HandleSecondaryFireStarted(const FInputActionValue& Value);
	void HandleSecondaryFireCompleted(const FInputActionValue& Value);
	void HandleDashStarted(const FInputActionValue& Value);
	void HandleAbilityQStarted(const FInputActionValue& Value);
	void HandleUltimateStarted(const FInputActionValue& Value);
	void HandlePower1Started(const FInputActionValue& Value);
	void HandlePower2Started(const FInputActionValue& Value);
	void HandlePower1KeyPressed();
	void HandlePower2KeyPressed();
	void HandleInteractStarted(const FInputActionValue& Value);
	void HandleInteractKeyPressed();
	void HandleRecallStarted(const FInputActionValue& Value);
	void HandleRecallKeyPressed();
	void HandleConsumable1(const FInputActionValue& Value);
	void HandleConsumable2(const FInputActionValue& Value);
	void HandleConsumable3(const FInputActionValue& Value);
	void HandleConsumable4(const FInputActionValue& Value);
	void HandleConsumable1KeyPressed();
	void HandleConsumable2KeyPressed();
	void HandleConsumable3KeyPressed();
	void HandleConsumable4KeyPressed();
	void HandleCancelTargetingKeyPressed();
	void HandleMapToggleKeyPressed();
	void ClearClientFrontendWidget();
	void PositionClientFrontendWidget(UUserWidget* Widget);

	/** Resolve the ASC from the owning PlayerState */
	UBBAbilitySystemComponent* GetBBASC() const;

	/**
	 * Trace cursor to ground plane, update controller yaw to face cursor,
	 * and send aim location to server at ~20Hz.
	 */
	void UpdateCursorAim(float DeltaTime);

	/** Deproject cursor screen position to world ray, intersect with ground plane at pawn Z */
	bool GetCursorWorldLocation(FVector& OutLocation) const;
	void BeginAbilityRangePreview(const FGameplayTag& InputTag, bool bUntilRelease);
	void EndAbilityRangePreview(const FGameplayTag& InputTag = FGameplayTag());
	void UpdateAbilityRangePreview();
	const UBBGameplayAbility* FindAbilityForRangePreview(const FGameplayTag& InputTag) const;
	void InitializeAbilitySmoke();
	void UpdateAbilitySmoke(float DeltaTime);
	bool ActivateAbilitySmokeInput(const FGameplayTag& InputTag, const TCHAR* ActionName);

	UFUNCTION(Server, Reliable)
	void ServerPrepareAbilitySmoke(int32 SmokeIndex);

	bool IsTacticalNukeEquippedForInput(const FGameplayTag& InputTag) const;
	void BeginTacticalNukeTargeting(const FGameplayTag& InputTag);
	void ConfirmTacticalNukeTargeting();
	void CancelTacticalNukeTargeting();
	void UpdateTacticalNukeTargeting(float DeltaTime);
	FVector GetClampedTacticalNukeTargetLocation() const;

	/** Check if jump is held and character is airborne — auto-open glider */
	void UpdateGliderFromHeldJump();

	/** Accumulator for throttling aim updates to ~20Hz */
	float AimUpdateTimer = 0.0f;

	/** Interval between aim location RPCs (seconds) */
	static constexpr float AimUpdateInterval = 0.05f; // 20Hz

	/** Whether the jump key is currently held down */
	bool bJumpInputHeld = false;

	// --- Debug Console Commands ---

	/** Debug: launch character upward and open glider */
	UFUNCTION(Exec)
	void DebugForceGlide();

	/** Debug: trigger spike while gliding */
	UFUNCTION(Exec)
	void DebugTriggerSpike();

	/** Server RPCs for debug commands — Exec runs on client, these forward to server */
	UFUNCTION(Server, Reliable)
	void ServerDebugForceGlide();

	UFUNCTION(Server, Reliable)
	void ServerDebugTriggerSpike();

	UFUNCTION(Server, Reliable)
	void ServerRequestHunterSelection(int32 HunterID);

	UFUNCTION(Server, Reliable)
	void ServerRequestReadyState(bool bReady);

	UFUNCTION(Server, Reliable)
	void ServerRequestJoinLobbySlot(int32 TeamID, int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerRequestJoinSpectators();

	UFUNCTION(Server, Reliable)
	void ServerRequestAutoFillLobbySlot();

	UFUNCTION(Server, Reliable)
	void ServerRequestSetLobbyTeamSize(int32 TeamSize);

	UFUNCTION(Server, Reliable)
	void ServerRequestSetLobbyDescription(const FString& Description);

	UFUNCTION(Server, Reliable)
	void ServerRequestSetStormShiftPreset(FName PresetID);

	UFUNCTION(Server, Reliable)
	void ServerRequestStartLobbyMatch();

	// --- Inventory Debug Console Commands ---

	/** Debug: add gold */
	UFUNCTION(Exec)
	void DebugAddGold(int32 Amount);

	/** Debug: add upgrade shards */
	UFUNCTION(Exec)
	void DebugAddShards(int32 Amount);

	/** Debug: equip a test weapon (placeholder item) */
	UFUNCTION(Exec)
	void DebugEquipTestWeapon();

	/** Debug: equip a test helmet */
	UFUNCTION(Exec)
	void DebugEquipTestHelmet();

	/** Debug: equip test boots */
	UFUNCTION(Exec)
	void DebugEquipTestBoots();

	/** Debug: unequip an equipment slot (Weapon/Helmet/Boots) */
	UFUNCTION(Exec)
	void DebugUnequip(const FString& SlotName);

	/** Debug: upgrade equipment in a slot */
	UFUNCTION(Exec)
	void DebugUpgrade(const FString& SlotName);

	/** Debug: evolve equipment in a slot (PathA/PathB/PathC) */
	UFUNCTION(Exec)
	void DebugEvolve(const FString& SlotName, const FString& PathName);

	/** Debug: set armor tier (None/White/Green/Blue/Purple) */
	UFUNCTION(Exec)
	void DebugSetArmor(const FString& TierName);

	/** Debug: add a consumable by name and count (e.g. DebugAddConsumable Food 3) */
	UFUNCTION(Exec)
	void DebugAddConsumable(const FString& ConsumableName, int32 Count);

	/** Debug: add a named key stack, e.g. DebugGiveKey Yellow or DebugGiveKey Red. */
	UFUNCTION(Exec)
	void DebugGiveKey(const FString& KeyName, int32 Count = 1);

	/** Debug: spawn a named key pickup near the player, e.g. DebugSpawnKeyPickup Yellow. */
	UFUNCTION(Exec)
	void DebugSpawnKeyPickup(const FString& KeyName);

	/** Debug: print inventory to log */
	UFUNCTION(Exec)
	void DebugPrintInventory();

	UFUNCTION(Exec)
	void DebugGivePower(const FString& PowerName, int32 SlotIndex = 0);

	UFUNCTION(Exec)
	void DebugDropPower(int32 SlotIndex);

	UFUNCTION(Exec)
	void DebugPrintPowers();

	UFUNCTION(Exec)
	void DebugSpawnPowerPickup(const FString& PowerName);

	/** Basecamp test: add gold for anvil repair. */
	UFUNCTION(Exec)
	void BasecampGiveGold(int32 Amount);

	/** Basecamp test: add Vive Beans for cauldron brewing. */
	UFUNCTION(Exec)
	void BasecampGiveViveBeans(int32 Count);

	/** Basecamp test: set current shield to 0. */
	UFUNCTION(Exec)
	void BasecampBreakShield();

	/** Basecamp test: set armor durability to 0 for anvil repair testing. */
	UFUNCTION(Exec)
	void BasecampBreakArmor();

	/** Basecamp test: reduce health without killing the player. */
	UFUNCTION(Exec)
	void BasecampDamageSelf(float Amount);

	/** Server RPCs for inventory debug commands */
	UFUNCTION(Server, Reliable)
	void ServerDebugAddGold(int32 Amount);

	UFUNCTION(Server, Reliable)
	void ServerDebugAddShards(int32 Amount);

	UFUNCTION(Server, Reliable)
	void ServerDebugEquipTestItem(EEquipmentSlotType SlotType);

	UFUNCTION(Server, Reliable)
	void ServerDebugUnequip(EEquipmentSlotType SlotType);

	UFUNCTION(Server, Reliable)
	void ServerDebugUpgrade(EEquipmentSlotType SlotType);

	UFUNCTION(Server, Reliable)
	void ServerDebugEvolve(EEquipmentSlotType SlotType, EEvolutionPath Path);

	UFUNCTION(Server, Reliable)
	void ServerDebugSetArmor(EArmorTier Tier);

	UFUNCTION(Server, Reliable)
	void ServerDebugAddConsumable(const FString& ConsumableName, int32 Count);

	UFUNCTION(Server, Reliable)
	void ServerDebugGiveKey(const FString& KeyName, int32 Count);

	UFUNCTION(Server, Reliable)
	void ServerDebugSpawnKeyPickup(const FString& KeyName);

	UFUNCTION(Server, Reliable)
	void ServerDebugGivePower(const FString& PowerName, int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerDebugDropPower(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerDebugSpawnPowerPickup(const FString& PowerName);

	UFUNCTION(Server, Reliable)
	void ServerConfirmTargetedPower(FGameplayTag InputTag, FVector_NetQuantize TargetLocation);

	UFUNCTION(Server, Reliable)
	void ServerBasecampBreakShield();

	UFUNCTION(Server, Reliable)
	void ServerBasecampBreakArmor();

	UFUNCTION(Server, Reliable)
	void ServerBasecampDamageSelf(float Amount);

	UFUNCTION(Server, Reliable)
	void ServerUseConsumable(int32 BackpackSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerInteractWithBasecamp(ABBBasecampActor* Basecamp);

	UFUNCTION(Server, Reliable)
	void ServerInteractWithWisp(ABBWispPawn* Wisp);

	UFUNCTION(Server, Reliable)
	void ServerInteractWithDeathbox(ABBDeathboxActor* Deathbox);

	UFUNCTION(Server, Reliable)
	void ServerStartBasecampRecall();

	void RecallTick();
	void CancelBasecampRecall(const TCHAR* Reason);
	void CompleteBasecampRecall();
	void ClearBasecampRecallState();
	void OnRecallHealthChanged(const FOnAttributeChangeData& Data);
	ABBBasecampActor* FindActiveRecallBasecampForTeam(int32 TeamID) const;
	bool TryTeleportPawnNearTransform(APawn* PawnToTeleport, const FTransform& TargetTransform) const;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Basecamp", meta = (AllowPrivateAccess = "true"))
	bool bBasecampRecallActive = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Basecamp", meta = (AllowPrivateAccess = "true"))
	float BasecampRecallProgress = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Basecamp", meta = (AllowPrivateAccess = "true"))
	float BasecampRecallDuration = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Basecamp", meta = (AllowPrivateAccess = "true"))
	float BasecampRecallMoveCancelDistance = 80.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Basecamp", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ABBBasecampActor> ActiveRecallBasecamp;

	FTimerHandle BasecampRecallTimerHandle;
	FDelegateHandle RecallHealthHandle;
	FVector BasecampRecallStartLocation = FVector::ZeroVector;
	float BasecampRecallElapsed = 0.0f;

	bool bTacticalNukeTargetingActive = false;
	FGameplayTag TacticalNukeTargetingInputTag;
	FVector TacticalNukeTargetLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke", meta = (AllowPrivateAccess = "true"))
	float TacticalNukeMaxTargetRange = 5200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke", meta = (AllowPrivateAccess = "true"))
	float TacticalNukePreviewRadius = 850.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke", meta = (AllowPrivateAccess = "true"))
	float TacticalNukeAerialViewArmLength = 4200.0f;

	FGameplayTag PendingTargetedPowerInputTag;
	FVector PendingTargetedPowerLocation = FVector::ZeroVector;

	/** Owner-only preview state; never replicated and never affects gameplay. */
	FGameplayTag ActiveRangePreviewInputTag;
	float ActiveRangePreviewEndTime = -1.0f;
	bool bRangePreviewUntilRelease = false;

	bool bAbilitySmokeEnabled = false;
	bool bAbilitySmokePrepared = false;
	bool bAbilitySmokeComplete = false;
	int32 AbilitySmokeIndex = 0;
	int32 AbilitySmokeHunterID = 0;
	int32 AbilitySmokeStep = 0;
	float AbilitySmokeElapsed = 0.0f;
	float AbilitySmokePhaseElapsed = 0.0f;
	float AbilitySmokeActionDelay = 0.0f;
	float AbilitySmokeAimAccumulator = 0.0f;
	float AbilitySmokeLobbyRetry = 0.0f;
	EMatchPhase AbilitySmokeLastPhase = EMatchPhase::Ended;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|UI|RangeIndicator")
	float DiscreteRangePreviewDuration = 0.85f;

	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveClientFrontendWidget;
};
