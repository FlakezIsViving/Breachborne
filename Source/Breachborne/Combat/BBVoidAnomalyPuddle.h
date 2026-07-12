#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBVoidAnomalyPuddle.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class BREACHBORNE_API ABBVoidAnomalyPuddle : public AActor
{
	GENERATED_BODY()

public:
	ABBVoidAnomalyPuddle();

	void InitPuddle(UAbilitySystemComponent* InSourceASC, AActor* InInstigatorActor, TSubclassOf<UGameplayEffect> InDamageGE,
		int32 InTeamID, float InRadius, float InDuration, float InTickDamage, float InBurstDamage);

	void Burst();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Void|Anomaly")
	TObjectPtr<USphereComponent> DamageSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Void|Anomaly")
	TObjectPtr<UStaticMeshComponent> PuddleMesh;

	UPROPERTY(ReplicatedUsing = OnRep_Radius)
	float Radius = 460.0f;

private:
	UFUNCTION()
	void OnRep_Radius();

	void DamageTick();
	void RefreshVisuals();
	void ApplyDamage(UAbilitySystemComponent* TargetASC, float Damage) const;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	UPROPERTY()
	TWeakObjectPtr<AActor> InstigatorActor;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	int32 SourceTeamID = -1;
	float TickDamage = 18.0f;
	float BurstDamage = 85.0f;
	bool bBurst = false;
	FTimerHandle TickHandle;
	FTimerHandle BurstHandle;
};
