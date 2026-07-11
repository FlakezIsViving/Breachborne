#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "BBTargetDummy.generated.h"

class UAbilitySystemComponent;
class UBBHealthSet;
class UCapsuleComponent;
class UStaticMeshComponent;

/**
 * Placeable test dummy for validating the damage pipeline.
 * Has its own ASC + HealthSet. Set TeamID to a value different from player teams
 * so team checks pass. Place in a level and shoot/grenade/napalm it to test damage flow.
 *
 * The output log will show every stage of the damage pipeline when this dummy is hit.
 * If bAutoResetHealth is true, the dummy resets health after HealthResetDelay seconds.
 */
UCLASS()
class BREACHBORNE_API ABBTargetDummy : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABBTargetDummy();

	virtual void BeginPlay() override;

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Breachborne|TargetDummy")
	int32 GetTeamID() const { return TeamID; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|TargetDummy")
	TObjectPtr<UCapsuleComponent> CapsuleComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|TargetDummy")
	TObjectPtr<UStaticMeshComponent> DummyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|TargetDummy")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UBBHealthSet> HealthSet;

	/** Team ID for damage team-checks. Use a value different from any player team (e.g. 99). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|TargetDummy")
	int32 TeamID = 99;

	/** If true, health resets after depleting so the dummy can be hit repeatedly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|TargetDummy")
	bool bAutoResetHealth = true;

	/** Seconds to wait before resetting health after depletion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|TargetDummy", meta = (EditCondition = "bAutoResetHealth"))
	float HealthResetDelay = 3.0f;

private:
	UFUNCTION()
	void OnHealthDepleted(UAbilitySystemComponent* VictimASC, UAbilitySystemComponent* KillerASC);

	void ResetHealth();

	FTimerHandle HealthResetTimerHandle;
};
