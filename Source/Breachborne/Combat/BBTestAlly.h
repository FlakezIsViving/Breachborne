#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "BBTestAlly.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UBBAbilitySystemComponent;
class UBBHealthSet;
class AHunterCharacter;
class ABreachbornePlayerState;
class ABBWispPawn;

/**
 * Placeable test ally for validating Eluna Q heal and wisp interactions.
 * Spawns with configurable health that rapidly decays. After LifeSpan seconds
 * (or when health reaches 0) it hides itself and spawns a BBWispPawn. When the
 * wisp's rez bar fills, this actor automatically revives. Can also be manually
 * killed/revived via console commands on BreachbornePlayerController.
 *
 * Drop into the level near where combat happens. Set TeamID to match the player
 * you want to test with (first PIE player is Team 0).
 */
UCLASS()
class BREACHBORNE_API ABBTestAlly : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABBTestAlly();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Breachborne|TestAlly")
	int32 GetTeamID() const { return TeamID; }

	/** Current health from the HealthSet. */
	UFUNCTION(BlueprintPure, Category = "Breachborne|TestAlly")
	float GetCurrentHealth() const;

	/** Whether this ally is currently dead (hidden, wisp spawned). */
	UFUNCTION(BlueprintPure, Category = "Breachborne|TestAlly")
	bool IsDead() const { return bDead; }

	/** Toggle health decay on/off. */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|TestAlly")
	void SetHealthDecayEnabled(bool bEnabled) { bHealthDecayEnabled = bEnabled; }

	/** Instantly kill this ally (spawns wisp). */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|TestAlly")
	void Kill();

	/** Revive this ally (destroys wisp, restores health, becomes visible). */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|TestAlly")
	void Revive();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|TestAlly")
	TObjectPtr<UCapsuleComponent> CapsuleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|TestAlly")
	TObjectPtr<UStaticMeshComponent> AllyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|TestAlly")
	TObjectPtr<UBBAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UBBHealthSet> HealthSet;

	/** Team ID — set this to match the player's team (first PIE player is Team 0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|TestAlly")
	int32 TeamID = 0;

	/** Starting / max health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|TestAlly")
	float MaxHealth = 100.0f;

	/** Health lost per second. Set high to test Q heal under pressure. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|TestAlly")
	float HealthDecayRate = 20.0f;

	/** Seconds until automatic death (and wisp spawn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|TestAlly")
	float LifeSpan = 5.0f;

	/** If true, health decays over time. Toggle off to keep ally alive for ability testing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|TestAlly")
	bool bHealthDecayEnabled = true;

private:
	void InitAbilitySystem();
	void ApplyHealthDecay(float DeltaTime);
	void EnterWispForm();
	ABBWispPawn* SpawnWispForNearestPlayer();

	UFUNCTION()
	void OnHealthDepleted(UAbilitySystemComponent* VictimASC, UAbilitySystemComponent* KillerASC);

	/** Called when our wisp's rez bar fills — auto-revives this ally. */
	UFUNCTION()
	void OnWispReadyToRevive(ABBWispPawn* Wisp);

	/** Called when our wisp dies permanently — just clears our reference. */
	UFUNCTION()
	void OnWispDied(ABBWispPawn* Wisp, bool bWasExecuted);

	float ElapsedLife = 0.0f;
	bool bDead = false;

	UPROPERTY()
	TWeakObjectPtr<ABBWispPawn> SpawnedWisp;
};
