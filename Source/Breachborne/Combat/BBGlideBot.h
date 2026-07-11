#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "BBGlideBot.generated.h"

class UAbilitySystemComponent;
class UBBHealthSet;
class UGliderComponent;

/**
 * Placeable gliding test target for validating the spike and abyss-spike-kill systems.
 * Patrols between two editor-configurable waypoints at altitude with the glider open.
 *
 * Place over ground to test spike-then-recover.
 * Place over the abyss to test spike-into-abyss-kill.
 *
 * Has its own ASC + HealthSet + GliderComponent + BBCharacterMovementComponent.
 * Uses the same damage/spike pipeline as AHunterCharacter.
 */
UCLASS()
class BREACHBORNE_API ABBGlideBot : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABBGlideBot(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Breachborne|GlideBot")
	int32 GetTeamID() const { return TeamID; }

protected:
	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|GlideBot")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UBBHealthSet> HealthSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|GlideBot")
	TObjectPtr<UGliderComponent> GliderComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|GlideBot")
	TObjectPtr<UStaticMeshComponent> BotMesh;

	// --- Editor-Configurable ---

	/** First patrol waypoint (relative to actor placement, draggable in viewport) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|GlideBot", meta = (MakeEditWidget = true))
	FVector WaypointA = FVector(-1000.0f, 0.0f, 0.0f);

	/** Second patrol waypoint (relative to actor placement, draggable in viewport) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|GlideBot", meta = (MakeEditWidget = true))
	FVector WaypointB = FVector(1000.0f, 0.0f, 0.0f);

	/** Horizontal patrol speed while gliding */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|GlideBot")
	float PatrolSpeed = 600.0f;

	/** Team ID for damage checks. Use a value different from any player team. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|GlideBot")
	int32 TeamID = 99;

	/** If true, respawns after being killed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|GlideBot")
	bool bAutoRespawn = true;

	/** Seconds to wait before respawning after death. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|GlideBot", meta = (EditCondition = "bAutoRespawn"))
	float RespawnDelay = 3.0f;

private:
	/** Launch into the air and open the glider after a short delay */
	void LaunchAndGlide();

	/** Deferred call to open glider after launch (needs a tick in Falling mode first) */
	void DeferredOpenGlider();

	/** Patrol between waypoints while gliding */
	void UpdatePatrol(float DeltaTime);

	UFUNCTION()
	void OnHealthDepleted(UAbilitySystemComponent* VictimASC, UAbilitySystemComponent* KillerASC);

	void OnSpikeRecovered();

	void Respawn();

	// --- Runtime State ---

	/** World-space waypoints (converted from relative in BeginPlay) */
	FVector WorldWaypointA;
	FVector WorldWaypointB;

	/** Which waypoint we're currently heading toward (true = B, false = A) */
	bool bHeadingToB = true;

	/** Spawn location for respawning */
	FVector SpawnLocation;

	/** Whether the bot is currently dead */
	bool bIsDead = false;

	FTimerHandle RespawnTimerHandle;
	FTimerHandle GlideOpenTimerHandle;
};
