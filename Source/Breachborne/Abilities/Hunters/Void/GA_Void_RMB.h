#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Void_RMB.generated.h"

class ABBVoidSnapProjectile;
class UGameplayEffect;

UCLASS()
class BREACHBORNE_API UGA_Void_RMB : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Void_RMB();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|RMB")
	float CooldownDuration = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|RMB")
	float SpawnForwardOffset = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|RMB")
	TSubclassOf<ABBVoidSnapProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|RMB")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Void|RMB")
	TSubclassOf<UGameplayEffect> StunEffectClass;

private:
	bool ConsumeEmpowered() const;

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};
