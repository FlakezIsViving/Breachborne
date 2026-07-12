#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBMoonlightZone.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class AHunterCharacter;
class UGameplayEffect;
class UBBAbilitySystemComponent;

/**
 * Moonlight Blessing healing zone — spawned by Eluna's Q ability.
 *
 * Heals allies within radius at regular intervals.
 * Ends with a burst heal. Less effective on self. Can heal Wisps at 500% rate.
 */
UCLASS()
class BREACHBORNE_API ABBMoonlightZone : public AActor
{
	GENERATED_BODY()

public:
	ABBMoonlightZone();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Initialize the zone with heal parameters. Call immediately after spawn.
	 */
	void InitZone(AHunterCharacter* InSourceHunter, float InHealPerTick, float InTickInterval, float InHealRadius, float InDuration, float InSelfHealFraction, float InWispHealMultiplier, float InBurstHeal = 0.0f);

	/** Toss the zone to a new location (boosts healing by 50%). */
	void TossToLocation(const FVector& TargetLocation);

	/** Whether this zone has been tossed (healing boosted). */
	bool WasTossed() const { return bTossed; }

	/** Whether this zone is currently traveling to a toss target. */
	bool IsTraveling() const { return bIsTraveling; }

	/** The ally this zone is attached to (nullptr if not attached). */
	AHunterCharacter* GetAttachedAlly() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|MoonlightZone")
	TObjectPtr<USphereComponent> HealSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|MoonlightZone")
	TObjectPtr<UStaticMeshComponent> ZoneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|MoonlightZone")
	TObjectPtr<UStaticMeshComponent> GroundRing;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|MoonlightZone")
	TSubclassOf<UGameplayEffect> HealEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|MoonlightZone")
	float TossSpeed = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|MoonlightZone")
	float AttachSearchRadius = 150.0f;

private:
	void TickHeal();
	void ApplyBurstHeal();
	void DestroyZone();
	void UpdateTravel(float DeltaTime);
	void TryAttachToAlly();
	bool TryAttachAlongPath(const FVector& From, const FVector& To);
	void RefreshVisualState();
	void SpawnPulse(bool bFinalBurst);

	UFUNCTION()
	void OnRep_VisualState();

	TWeakObjectPtr<AHunterCharacter> SourceHunter;
	TWeakObjectPtr<AActor> AttachedAlly;
	float HealPerTick = 12.0f;
	float TickInterval = 0.5f;
	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	float HealRadius = 400.0f;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	float ZoneDuration = 2.5f;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	bool bVisualInitialized = false;

	float SelfHealFraction = 0.5f;
	float WispHealMultiplier = 5.0f;
	float BurstHeal = 90.0f;
	float ElapsedTime = 0.0f;
	float TickAccumulator = 0.0f;
	bool bInitialized = false;
	UPROPERTY(Replicated)
	bool bTossed = false;

	UPROPERTY(Replicated)
	bool bIsTraveling = false;

	FVector ThrowStartLocation;
	FVector ThrowTargetLocation;
};
