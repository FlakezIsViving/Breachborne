#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ActiveGameplayEffectHandle.h"
#include "BBHudsonBarbedWireActor.generated.h"

class UAbilitySystemComponent;
class UBoxComponent;
class UGameplayEffect;
class UStaticMeshComponent;

/** Replicated primitive barbed-wire zone for Hudson's Q. */
UCLASS()
class BREACHBORNE_API ABBHudsonBarbedWireActor : public AActor
{
	GENERATED_BODY()

public:
	ABBHudsonBarbedWireActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void InitWire(UAbilitySystemComponent* InSourceASC, int32 InTeamID, float InDuration, float InDamagePerSecond);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Hudson|BarbedWire")
	TObjectPtr<UBoxComponent> WireBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Hudson|BarbedWire")
	TObjectPtr<UStaticMeshComponent> GroundPlate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Hudson|BarbedWire")
	TObjectPtr<UStaticMeshComponent> BarA;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Hudson|BarbedWire")
	TObjectPtr<UStaticMeshComponent> BarB;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Hudson|BarbedWire")
	TObjectPtr<UStaticMeshComponent> BarC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Hudson|BarbedWire")
	TObjectPtr<UStaticMeshComponent> BarD;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|BarbedWire")
	float TickInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|BarbedWire")
	float SlowDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|BarbedWire")
	float AirborneStunDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|BarbedWire")
	float AirborneStunReapplyDelay = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|BarbedWire")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|BarbedWire")
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|BarbedWire")
	TSubclassOf<UGameplayEffect> GroundedEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Hudson|BarbedWire")
	TSubclassOf<UGameplayEffect> StunEffectClass;

private:
	void ApplyWireTick();
	bool ResolveTarget(AActor* Actor, UAbilitySystemComponent*& OutASC, int32& OutTeamID) const;

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TMap<TWeakObjectPtr<AActor>, FActiveGameplayEffectHandle> ActiveSlowHandlesByActor;
	TMap<TWeakObjectPtr<AActor>, float> LastAirborneStunTimeByActor;
	int32 SourceTeamID = -1;
	float LifetimeSeconds = 8.0f;
	float DamagePerSecond = 20.0f;
	float ElapsedTickTime = 0.0f;
};
