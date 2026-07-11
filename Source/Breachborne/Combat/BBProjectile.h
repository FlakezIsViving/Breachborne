#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

/**
 * Reusable replicated projectile actor. Server-spawned, replicates to clients.
 * Applies damage through the GAS pipeline on server overlap with enemy pawns.
 * Top-down: gravity disabled, flat trajectory.
 */
UCLASS()
class BREACHBORNE_API ABBProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABBProjectile();

	/**
	 * Initialize projectile with damage data. Call on server immediately after spawn.
	 * @param InSourceASC The ASC that fired this projectile (for damage effect context)
	 * @param InDamageGE The GameplayEffect class to apply on hit
	 * @param InDamage The base damage magnitude (SetByCaller)
	 * @param InTeamID The firing player's team ID (skip same-team hits)
	 */
	void InitProjectile(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageGE, float InDamage, int32 InTeamID);

	/** Set the projectile's velocity direction. Call after InitProjectile. */
	void FireInDirection(const FVector& Direction);

protected:
	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	// --- Tuning ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Projectile")
	float ProjectileSpeed = 3500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Projectile")
	float ProjectileLifetime = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Projectile")
	float CollisionRadius = 18.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Projectile")
	bool bPiercing = false;

	UFUNCTION()
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	virtual void OnLifetimeExpired();

	// --- Server-only damage state (not replicated, accessible to subclasses) ---

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	float BaseDamage = 0.0f;
	int32 SourceTeamID = -1;
	TSet<TWeakObjectPtr<AActor>> HitActors;

	FTimerHandle LifetimeTimerHandle;
};
