#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBCrystaFluxSnareProjectile.generated.h"

class AHunterCharacter;
class USphereComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class BREACHBORNE_API ABBCrystaFluxSnareProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABBCrystaFluxSnareProjectile();

	virtual void Tick(float DeltaSeconds) override;

	void InitFluxSnare(AHunterCharacter* InSourceHunter, UAbilitySystemComponent* InSourceASC,
		TSubclassOf<UGameplayEffect> InDamageGE, TSubclassOf<UGameplayEffect> InMarkGE,
		TSubclassOf<UGameplayEffect> InGroundedGE, int32 InTeamID, const FVector& InInitialDirection);

	void StartReturn();
	bool IsReturning() const { return bReturning; }
	FVector GetCurrentTravelDirection() const { return CurrentDirection; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Crysta|FluxSnare")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Crysta|FluxSnare")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

private:
	UFUNCTION()
	void OnRep_Returning();

	void ApplyLegVisuals();
	void MoveProjectile(float DeltaSeconds);
	void ProcessHit(AActor* Actor);
	void ApplyDamage(UAbilitySystemComponent* TargetASC, float Damage) const;
	void ApplyEffect(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, float Duration) const;
	void PullTarget(AActor* Actor) const;

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY()
	TWeakObjectPtr<AHunterCharacter> SourceHunter;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> MarkEffectClass;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> GroundedEffectClass;

	UPROPERTY(ReplicatedUsing = OnRep_Returning)
	bool bReturning = false;

	FVector CurrentDirection = FVector::ForwardVector;
	FVector SpawnLocation = FVector::ZeroVector;
	int32 SourceTeamID = -1;
	float OutboundSpeed = 2300.0f;
	float ReturnSpeed = 2600.0f;
	float MaxRange = 1350.0f;
	float MaxLifetime = 1.8f;
	float ElapsedTime = 0.0f;
	float OutboundDamage = 45.0f;
	float ReturnDamage = 55.0f;
	float GroundedDuration = 1.0f;
	float SteeringStrength = 5.5f;
	float ReturnEndDistance = 90.0f;
	float PullImpulse = 850.0f;
	TSet<TWeakObjectPtr<AActor>> OutboundHits;
	TSet<TWeakObjectPtr<AActor>> ReturnHits;
};
