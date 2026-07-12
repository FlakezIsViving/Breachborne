#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Combat/BBProjectile.h"
#include "BBVoidOrbProjectile.generated.h"

class UGameplayEffect;

UCLASS()
class BREACHBORNE_API ABBVoidOrbProjectile : public ABBProjectile
{
	GENERATED_BODY()

public:
	ABBVoidOrbProjectile();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitVoidOrb(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageGE,
		TSubclassOf<UGameplayEffect> InSlowGE, float InDamage, int32 InTeamID, bool bInCharged);

protected:
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	UFUNCTION()
	void OnRep_Charged();

	void ApplyOrbVisuals();
	void ApplyDamage(UAbilitySystemComponent* TargetASC) const;
	void ApplySlow(UAbilitySystemComponent* TargetASC) const;
	void EmitDamageEvent(AActor* TargetActor) const;

	TSubclassOf<UGameplayEffect> SlowEffectClass;
	UPROPERTY(ReplicatedUsing = OnRep_Charged)
	bool bCharged = false;
};
