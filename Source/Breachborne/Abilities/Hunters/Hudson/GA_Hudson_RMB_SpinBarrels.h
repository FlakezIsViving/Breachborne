#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Hudson_RMB_SpinBarrels.generated.h"

/** Hudson RMB - spin-up state that empowers LMB and displays a primitive ring. */
UCLASS()
class BREACHBORNE_API UGA_Hudson_RMB_SpinBarrels : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Hudson_RMB_SpinBarrels();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|RMB")
	float SpinUpSeconds = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|RMB")
	float WindDownCooldownSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|RMB")
	float VisualTickInterval = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|RMB")
	float TurnRateMultiplier = 0.5f;

private:
	void SpinTick();
	void PlaySpinLoopMontage();

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;

	FTimerHandle SpinTickHandle;
	FTimerHandle SpinLoopMontageHandle;
	float SpinStartTime = 0.0f;
	float OriginalYawRate = 720.0f;
	uint8 LastVisualStep = 0;
	bool bReachedSpunUp = false;
	bool bShouldApplyWindDownCooldown = false;
	TWeakObjectPtr<class ABBPrimitiveWedgeActor> ActiveSpinVisual;
};
