#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBNapalmZone.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

/**
 * Persistent AoE DOT zone. Server-spawned, replicates to clients for visuals.
 * Ticks damage every DamageTickInterval to enemy hunters standing inside.
 * Self-destructs after ZoneDuration.
 */
UCLASS()
class BREACHBORNE_API ABBNapalmZone : public AActor
{
	GENERATED_BODY()

public:
	ABBNapalmZone();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Initialize the zone with damage parameters. Call on server immediately after spawn.
	 * @param InASC Source ASC for the GE spec context
	 * @param InDamageGE The damage GameplayEffect class to apply each tick
	 * @param InDamagePerTick Damage magnitude per tick (SetByCaller)
	 * @param InTeamID Source team ID (skip same-team damage)
	 */
	void InitZone(UAbilitySystemComponent* InASC, TSubclassOf<UGameplayEffect> InDamageGE, float InDamagePerTick, int32 InTeamID);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|NapalmZone")
	TObjectPtr<USphereComponent> ZoneCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|NapalmZone")
	TObjectPtr<UStaticMeshComponent> ZoneVisualMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|NapalmZone")
	float ZoneRadius = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|NapalmZone")
	float ZoneDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|NapalmZone")
	float WarningDuration = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|NapalmZone")
	float DamageTickInterval = 0.5f;

private:
	void BeginActiveZone();
	void OnDamageTick();
	void OnZoneExpired();

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	float DamagePerTick = 0.0f;
	int32 SourceTeamID = -1;
	bool bActiveCueAdded = false;

	FTimerHandle ActivationHandle;
	FTimerHandle DamageTickHandle;
	FTimerHandle DurationHandle;
};
