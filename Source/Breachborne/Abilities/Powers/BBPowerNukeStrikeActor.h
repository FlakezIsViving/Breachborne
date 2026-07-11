#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBPowerNukeStrikeActor.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class BREACHBORNE_API ABBPowerNukeStrikeActor : public AActor
{
	GENERATED_BODY()

public:
	ABBPowerNukeStrikeActor();

	virtual void Tick(float DeltaSeconds) override;

	void InitNukeStrike(UAbilitySystemComponent* InSourceASC, AActor* InInstigatorActor, TSubclassOf<UGameplayEffect> InDamageGE,
		float InRadius, float InImpactDelay, float InImpactDamage, float InBurnDuration, float InBurnDamagePerTick,
		float InBurnTickInterval);

	UFUNCTION(BlueprintPure, Category = "Breachborne|Powers|Nuke")
	float GetRadius() const { return Radius; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	TObjectPtr<USphereComponent> WarningRadius;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	TObjectPtr<UStaticMeshComponent> WarningMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(ReplicatedUsing = OnRep_Radius, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	float Radius = 850.0f;

	UPROPERTY(ReplicatedUsing = OnRep_LaunchLocation, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	FVector_NetQuantize LaunchLocation;

private:
	UFUNCTION()
	void OnRep_Radius();

	UFUNCTION()
	void OnRep_LaunchLocation();

	void Impact();
	void ApplyImpactDamage();
	void ApplyPowerDestruction();
	void SpawnBurnZone();
	void RefreshRadiusVisuals();
	void RefreshProjectileVisual();
	FVector GetProjectilePoint(float Alpha) const;

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TWeakObjectPtr<AActor> InstigatorActor;
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	float ImpactDelay = 2.5f;
	float ImpactDamage = 99999.0f;
	float BurnDuration = 2.5f;
	float BurnDamagePerTick = 75.0f;
	float BurnTickInterval = 0.5f;
	float TravelArcHeight = 1400.0f;
	float LocalElapsedTime = 0.0f;

	FTimerHandle ImpactTimerHandle;
};
