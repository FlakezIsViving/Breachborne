#pragma once

#include "CoreMinimal.h"
#include "BBProjectile.h"
#include "BBElunaStickyProjectile.generated.h"

class ABBElunaRootZone;

/**
 * Eluna RMB projectile — bigger white sphere that sticks to enemies or ground,
 * then spawns an orange root zone after a delay.
 */
UCLASS()
class BREACHBORNE_API ABBElunaStickyProjectile : public ABBProjectile
{
	GENERATED_BODY()

public:
	ABBElunaStickyProjectile();

	virtual void InitProjectile(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageGE, float InDamage, int32 InTeamID);

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	float ExplosionDelay = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	float RootRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	float RootDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|RMB")
	TSubclassOf<ABBElunaRootZone> RootZoneClass;

protected:
	virtual void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	void Explode();
	void StickToActor(AActor* TargetActor);

	FTimerHandle ExplosionTimerHandle;
	TWeakObjectPtr<AActor> StuckTarget;
	FVector StuckOffset;
	bool bHasStuck = false;
	bool bHasExploded = false;
};
