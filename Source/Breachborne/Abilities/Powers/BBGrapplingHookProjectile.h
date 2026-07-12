#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBGrapplingHookProjectile.generated.h"

class AHunterCharacter;
class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class BREACHBORNE_API ABBGrapplingHookProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABBGrapplingHookProjectile();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitGrapple(AHunterCharacter* InOwnerHunter, const FVector& Direction, float InRange, float InInitialPullSpeed, float InMaxPullSpeed, float InStopDistance, float InMaxPullDuration, float InPullTickInterval);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnHookHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnHookOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_AttachmentState();

private:
	void AttachHook(const FHitResult& Hit);
	void UpdateChainVisual();

	UPROPERTY(VisibleAnywhere, Category = "Breachborne|Grapple")
	TObjectPtr<USphereComponent> CollisionComp;

	UPROPERTY(VisibleAnywhere, Category = "Breachborne|Grapple")
	TObjectPtr<UStaticMeshComponent> HookMesh;

	UPROPERTY(VisibleAnywhere, Category = "Breachborne|Grapple")
	TObjectPtr<UStaticMeshComponent> ChainMesh;

	UPROPERTY(VisibleAnywhere, Category = "Breachborne|Grapple")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY()
	TWeakObjectPtr<AHunterCharacter> OwnerHunter;

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentState)
	TObjectPtr<AActor> AttachedActor;

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentState)
	FVector_NetQuantize AttachedLocation = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing = OnRep_AttachmentState)
	bool bAttached = false;

	FVector StartLocation = FVector::ZeroVector;
	float Range = 2200.0f;
	float InitialPullSpeed = 1900.0f;
	float MaxPullSpeed = 4400.0f;
	float StopDistance = 140.0f;
	float MaxPullDuration = 2.2f;
	float PullTickInterval = 0.02f;
};
