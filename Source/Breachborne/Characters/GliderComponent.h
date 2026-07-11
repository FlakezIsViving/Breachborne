#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Breachborne/Core/BreachborneTypes.h"
#include "GliderComponent.generated.h"

class ACharacter;
class UBBCharacterMovementComponent;
class UAbilitySystemComponent;

/**
 * Manages the glider state machine, heat buildup, and spiking mechanic.
 * Server-authoritative: all state changes happen on the server and replicate to clients.
 * Clients request open/close via Server RPCs; the server validates and changes state.
 *
 * State Machine:
 *   Closed <-> Open -> Spiked -> Closed
 *
 * Heat builds while Open. Taking damage while Open triggers Spiked.
 * Spiked = stunned + fast fall for SpikeDuration, then auto-recovers to Closed.
 * Heat decays when Closed and grounded.
 */
UCLASS(ClassGroup = (Breachborne), meta = (BlueprintSpawnableComponent))
class BREACHBORNE_API UGliderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGliderComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Public Accessors ---

	UFUNCTION(BlueprintPure, Category = "Breachborne|Glider")
	EGliderState GetGliderState() const { return GliderState; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Glider")
	float GetFuelLevel() const { return FuelLevel; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Glider")
	float GetHeatLevel() const { return FuelLevel; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|Glider")
	bool IsGliderOpen() const { return GliderState == EGliderState::Open; }

	/** Get the actor that caused the last spike (for abyss kill credit) */
	AActor* GetLastDamageInstigator() const { return LastDamageInstigator.Get(); }

	// --- Server-Only Direct Control (for bots/AI that have no owning connection) ---

	/** Open glider directly, bypassing RPC + altitude validation. Server only. */
	void ForceOpenGlider();

	/** Open glider for match-start drops without consuming fuel until it closes. Server only. */
	void ForceOpenFreeDropGlider();

	/** Close glider directly, bypassing RPC. Server only. */
	void ForceCloseGlider();

	/** Stop glider/spike state before entering mantle recovery. Server only. */
	void CancelForMantle();

	/** Broadcast after spike recovery completes (server only). */
	DECLARE_MULTICAST_DELEGATE(FOnSpikeRecovered);
	FOnSpikeRecovered OnSpikeRecovered;

	// --- Server RPCs (called by PlayerController) ---

	/** Client requests to open the glider. Server validates and opens if valid. */
	UFUNCTION(Server, Reliable)
	void ServerRequestOpenGlider();

	/** Client requests to close the glider. */
	UFUNCTION(Server, Reliable)
	void ServerRequestCloseGlider();

	// --- Server-Only Logic ---

	/**
	 * Trigger a spike. Called from BBHealthSet when damage is received while gliding.
	 * Server only.
	 * @param Instigator The actor that dealt the damage causing the spike.
	 */
	void TriggerSpike(AActor* Instigator);

protected:
	virtual void BeginPlay() override;

private:
	// --- State Transitions (server only) ---
	void OpenGlider();
	void CloseGlider();
	void RecoverFromSpike();

	/** Bound to ACharacter::LandedDelegate — force close glider and reset heat on landing */
	UFUNCTION()
	void OnCharacterLanded(const FHitResult& Hit);

	// --- OnRep Callbacks ---

	UFUNCTION()
	void OnRep_GliderState();

	/** Show/hide the glider indicator mesh based on current state */
	void UpdateGliderVisual() const;

	// --- Cached References ---
	ACharacter* GetOwnerCharacter() const;
	UBBCharacterMovementComponent* GetBBMovement() const;
	UAbilitySystemComponent* GetASC() const;

	// --- Replicated State ---

	/** Current glider state. Server authoritative. */
	UPROPERTY(ReplicatedUsing = OnRep_GliderState)
	EGliderState GliderState = EGliderState::Closed;

	/** Fuel level 0.0-1.0. Drains while gliding, refills on solid ground. */
	UPROPERTY(Replicated)
	float FuelLevel = 1.0f;

	// --- Server-Only State ---

	/** Countdown timer for spike stun duration */
	float SpikeTimer = 0.0f;

	/** Who spiked us — used for deathbox placement if spiked over abyss */
	TWeakObjectPtr<AActor> LastDamageInstigator;

	bool bFreeFuelUntilClosed = false;

	// --- Tuning ---

	/** Rate at which heat builds per second while gliding */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Glider")
	float FuelDrainRate = 0.18f;

	/** Rate at which heat decays per second when grounded */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Glider")
	float FuelRefillRate = 0.85f;

	/** Minimum fuel required to deploy the glider. */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Glider")
	float MinFuelToOpen = 0.03f;

	/** Duration of the spiked stun state in seconds */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Glider")
	float SpikeDuration = 1.5f;

	/** Minimum Z distance above ground to allow opening the glider */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Glider")
	float MinAltitudeForGlide = 50.0f;
};
