#pragma once

#include "CoreMinimal.h"
#include "BBProjectile.h"
#include "BBGrenade.generated.h"

/**
 * AoE exploding projectile. Inherits movement, collision, replication from ABBProjectile.
 * Explodes on first enemy hit, wall hit, or after FuseTime expires.
 * Explosion damages all enemy hunters within ExplosionRadius via GAS pipeline.
 */
UCLASS()
class BREACHBORNE_API ABBGrenade : public ABBProjectile
{
	GENERATED_BODY()

public:
	ABBGrenade();

	virtual void BeginPlay() override;

protected:
	//~ ABBProjectile overrides — explode instead of single-target damage
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	virtual void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void OnLifetimeExpired() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Grenade")
	float ExplosionRadius = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Grenade")
	float FuseTime = 1.5f;

private:
	/** Perform the AoE explosion: sphere overlap, team check, apply damage GE. Server only. */
	void Explode();

	void OnFuseExpired();

	bool bHasExploded = false;
	FTimerHandle FuseTimerHandle;
};
