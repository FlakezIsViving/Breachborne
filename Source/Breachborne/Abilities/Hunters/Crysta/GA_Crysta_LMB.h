#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Crysta_LMB.generated.h"

class ABBCrystaLMBProjectile;
class UGameplayEffect;

UCLASS()
class BREACHBORNE_API UGA_Crysta_LMB : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Crysta_LMB();
	static void CalculateShotSpawnLocations(const FVector& Origin, const FVector& Direction, bool bEmpowered,
		float ForwardOffset, float ShotSpacing, TArray<FVector>& OutLocations);

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|LMB")
	float FireInterval = 0.38f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|LMB")
	float BaseDamage = 38.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|LMB")
	float DetonationDamage = 35.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|LMB")
	float SpawnForwardOffset = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|LMB")
	float EmpoweredShotSpacing = 95.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|LMB")
	TSubclassOf<ABBCrystaLMBProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|LMB")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|LMB")
	TSubclassOf<UGameplayEffect> MarkEffectClass;

private:
	void FireShot();
	void SpawnProjectile(const FVector& Direction, bool bEmpoweredShot, const FVector& SpawnLocation);

	FTimerHandle FireTimerHandle;
};
