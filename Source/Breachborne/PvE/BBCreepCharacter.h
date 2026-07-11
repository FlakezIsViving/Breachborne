#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "BBCreepCharacter.generated.h"

class UAbilitySystemComponent;
class UBBHealthSet;
class UBBCombatSet;
class UBBDamageExecution;
class ABBCreepCamp;
struct FOnAttributeChangeData;
struct FGameplayEffectSpec;
struct FActiveGameplayEffectHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCreepKilled,
	ABBCreepCharacter*, Creep,
	UAbilitySystemComponent*, KillerASC);

/**
 * Networked PvE creep. Owned by an ABBCreepCamp spawner.
 *
 * - Has its own ASC (actor-owned; no PlayerState for PvE)
 * - Replication mode: Minimal (simulated proxies see no ability details, just position)
 * - BehaviorTree ticks on server only; clients see replicated position/rotation
 * - AI states: Idle → Aggro → Attack → Leash (back to Idle when too far from camp)
 * - On death: fires OnCreepKilled, notifies camp, destroys self after a short delay
 */
UCLASS()
class BREACHBORNE_API ABBCreepCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABBCreepCharacter();

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Called by the camp after spawn to link back to the camp actor */
	void InitForCamp(ABBCreepCamp* OwningCamp, FVector HomeLocation);

	/** Attack range — shared with BT task */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Creep")
	float AttackRange = 200.0f;

	/** Damage dealt per auto-attack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Creep")
	float AttackDamage = 12.0f;

	/** Seconds between attacks */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Creep")
	float AttackCooldown = 1.2f;

	/** How far from home before the creep leashes back */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Creep")
	float LeashRadius = 1500.0f;

	/** Max health (configured per camp tier in Blueprint) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Creep")
	float MaxHealth = 120.0f;

	/** Shard reward on kill */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Creep")
	int32 ShardReward = 2;

	/** Gold reward on kill */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Creep")
	int32 GoldReward = 5;

	/** Fired on server when the creep's health reaches 0 */
	UPROPERTY(BlueprintAssignable, Category = "Breachborne|Creep")
	FOnCreepKilled OnCreepKilled;

	/** Home position — leash target when too far from camp */
	FVector HomeLocation = FVector::ZeroVector;

	/** Owning camp (non-replicated — server only bookkeeping) */
	UPROPERTY()
	TObjectPtr<ABBCreepCamp> OwningCamp;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UBBHealthSet> HealthSet;

	UPROPERTY()
	TObjectPtr<UBBCombatSet> CombatSet;

private:
	/** Attribute change delegate handle — monitored for health drop to 0 */
	FDelegateHandle HealthChangedHandle;

	void OnHealthChanged(const FOnAttributeChangeData& Data);

	/**
	 * Fires whenever a GameplayEffect is applied to this creep's ASC.
	 * Used to cache the last damage instigator so OnHealthChanged can reward the killer
	 * without needing FGameplayEffectModCallbackData (avoids UE5.7 include-chain issues).
	 */
	void OnEffectAppliedToSelf(UAbilitySystemComponent* Source,
	                           const FGameplayEffectSpec& SpecApplied,
	                           FActiveGameplayEffectHandle ActiveHandle);

	/** Cached last damage instigator — updated by OnEffectAppliedToSelf. Server only. */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> LastDamageInstigatorASC;

	/** Apply a melee attack to the current aggro target */
	void PerformAttack(AActor* Target);

	FTimerHandle AttackTimerHandle;
};
