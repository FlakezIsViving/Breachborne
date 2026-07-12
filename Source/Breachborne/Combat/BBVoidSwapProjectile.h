#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBVoidSwapProjectile.generated.h"

class AHunterCharacter;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class BREACHBORNE_API ABBVoidSwapProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABBVoidSwapProjectile();

	virtual void Tick(float DeltaSeconds) override;

	void InitSwapProjectile(AHunterCharacter* InSourceHunter, const FVector& InLaunchLocation, const FVector& InTargetLocation,
		float InTravelDuration, float InRadius, bool bInEmpowered);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Void|Swap")
	TObjectPtr<USphereComponent> SwapSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Void|Swap")
	TObjectPtr<UStaticMeshComponent> ZoneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Void|Swap")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(ReplicatedUsing = OnRep_SwapVisuals)
	FVector_NetQuantize LaunchLocation;

	UPROPERTY(ReplicatedUsing = OnRep_SwapVisuals)
	FVector_NetQuantize TargetLocation;

	UPROPERTY(ReplicatedUsing = OnRep_SwapVisuals)
	float Radius = 360.0f;

	UPROPERTY(ReplicatedUsing = OnRep_SwapVisuals)
	bool bEmpowered = false;

private:
	UFUNCTION()
	void OnRep_SwapVisuals();

	void RefreshVisuals();
	void UpdateArc(float DeltaSeconds);
	void ExecuteSwap();
	FVector GetArcPoint(float Alpha) const;
	void GatherEligibleActors(const FVector& Center, TArray<AActor*>& OutActors) const;
	bool IsActorEligible(AActor* Actor) const;
	void TeleportActor(AActor* Actor, const FVector& NewLocation) const;

	UPROPERTY()
	TWeakObjectPtr<AHunterCharacter> SourceHunter;

	float TravelDuration = 1.5f;
	float TravelArcHeight = 360.0f;
	float Elapsed = 0.0f;
	bool bSwapped = false;
};
