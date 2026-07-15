#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Crysta_R.generated.h"

class UGameplayEffect;
class ABBPrimitiveBeamActor;

UCLASS()
class BREACHBORNE_API UGA_Crysta_R : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Crysta_R();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;
	static void CalculateLaneSegments(const FVector& CastLocation, const FVector& AimDirection,
		const FVector& TargetLocation, float InMaxRange, float InLauncherSideOffset,
		float InLauncherForwardOffset, TArray<FVector>& OutStarts, TArray<FVector>& OutEnds);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|R")
	float CooldownDuration = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|R")
	float MaxRange = 2300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|R")
	float LauncherSideOffset = 190.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|R")
	float LauncherForwardOffset = 115.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|R")
	float SalvoInterval = 0.18f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|R")
	float BeamSweepRadius = 55.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|R|Visuals")
	float IndicatorBeamRadius = 7.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|R")
	float ShotDamage = 42.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|R")
	float DetonationDamage = 45.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|R")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|R")
	TSubclassOf<UGameplayEffect> MarkEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Crysta|R|Visuals")
	TSubclassOf<ABBPrimitiveBeamActor> BeamVisualClass;

private:
	void FireSalvo(int32 SalvoIndex);
	void FinishSalvo();
	void HitAlongSegment(const FVector& Start, const FVector& End);
	void SpawnBeamVisual(const FVector& Start, const FVector& End, float Radius, float LifetimeSeconds, const FLinearColor& Color) const;
	void SpawnLaneIndicators();
	void ApplyDamage(UAbilitySystemComponent* TargetASC, float Damage) const;
	void ApplyMark(UAbilitySystemComponent* TargetASC) const;
	void DetonateOnce(UAbilitySystemComponent* TargetASC, AActor* TargetActor);
	void BuildLaneSegments(TArray<FVector>& OutStarts, TArray<FVector>& OutEnds) const;

	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;

	TArray<FTimerHandle> SalvoTimerHandles;
	TSet<TWeakObjectPtr<AActor>> DetonatedTargets;
	FVector CachedCastLocation = FVector::ZeroVector;
	FVector CachedTargetLocation = FVector::ZeroVector;
	FVector CachedAimDirection = FVector::ForwardVector;
	int32 CachedSourceTeamID = -1;
};
