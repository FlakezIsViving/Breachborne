#pragma once

#include "CoreMinimal.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "GA_Eluna_Shift.generated.h"

/**
 * Eluna Shift — Lunar Dash (Ground version).
 *
 * Two-charge dash usable anywhere (ground or air). Launches in input direction.
 * Passing through an allied wisp collects it and fully refunds this dash.
 * LocalPredicted. 6s cooldown per charge (2 charges).
 */
UCLASS()
class BREACHBORNE_API UGA_Eluna_GroundDash : public UBBGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Eluna_GroundDash();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** Authoritative path collection shared by activation and packaged regression smoke. */
	bool TryCollectWispAlongPath(const FVector& DashStart, const FVector& DashEnd);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Shift")
	float DashDistance = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Shift")
	float DashSpeed = 1600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Shift")
	float DashLift = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Shift")
	float CooldownDuration = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Shift")
	float WispDetectionRadius = 100.0f;

private:
	UPROPERTY()
	FGameplayTagContainer CooldownTagContainer;
};

/**
 * Eluna Shift — Lunar Dash (Aerial version).
 *
 * Usable only while airborne. 1.5x dash distance.
 * Granted BEFORE GroundDash so input dispatcher tries this first.
 */
UCLASS()
class BREACHBORNE_API UGA_Eluna_AerialDash : public UGA_Eluna_GroundDash
{
	GENERATED_BODY()

public:
	UGA_Eluna_AerialDash();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual const FGameplayTagContainer* GetCooldownTags() const override;
};
