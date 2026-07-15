#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Combat/BBProjectile.h"
#include "BBCrystaLMBProjectile.generated.h"

class UGameplayEffect;

/** Crysta basic projectile; optionally detonates and/or applies Reverberation. */
UCLASS()
class BREACHBORNE_API ABBCrystaLMBProjectile : public ABBProjectile
{
	GENERATED_BODY()

public:
	ABBCrystaLMBProjectile();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitCrystaProjectile(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageGE,
		TSubclassOf<UGameplayEffect> InMarkGE, float InDamage, float InDetonationDamage, int32 InTeamID,
		bool bInCanDetonate, bool bInAppliesMark, bool bInEmpoweredShot = false);

#if WITH_AUTOMATION_TESTS
	void ProcessOverlapForAutomation(AActor* OtherActor);
#endif

protected:
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

private:
	UFUNCTION()
	void OnRep_EmpoweredShot();

	void ApplyShotVisuals();
	void ApplyDamage(UAbilitySystemComponent* TargetASC, float Damage) const;
	void ApplyMark(UAbilitySystemComponent* TargetASC) const;
	bool DetonateMark(UAbilitySystemComponent* TargetASC, AActor* TargetActor) const;

	TSubclassOf<UGameplayEffect> MarkEffectClass;
	float DetonationDamage = 35.0f;
	bool bCanDetonate = true;
	bool bAppliesMark = false;
	UPROPERTY(ReplicatedUsing = OnRep_EmpoweredShot)
	bool bEmpoweredShot = false;
};
