#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBCrystaBurstZone.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;
class AHunterCharacter;

UCLASS()
class BREACHBORNE_API ABBCrystaBurstZone : public AActor
{
	GENERATED_BODY()

public:
	ABBCrystaBurstZone();

	void InitBurstZone(AHunterCharacter* InSourceHunter, UAbilitySystemComponent* InSourceASC,
		TSubclassOf<UGameplayEffect> InDamageGE, TSubclassOf<UGameplayEffect> InMarkGE,
		int32 InTeamID, float InRadius, float InDuration, float InBurstDamage);

	void Detonate();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Crysta|Burst")
	TObjectPtr<USphereComponent> BurstSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Crysta|Burst")
	TObjectPtr<UStaticMeshComponent> ZoneMesh;

	UPROPERTY(ReplicatedUsing = OnRep_Radius)
	float Radius = 420.0f;

private:
	UFUNCTION()
	void OnRep_Radius();

	void MarkTick();
	void RefreshVisuals();
	void ApplyMark(UAbilitySystemComponent* TargetASC) const;
	void ApplyDamage(UAbilitySystemComponent* TargetASC, float Damage) const;

	UPROPERTY()
	TWeakObjectPtr<AHunterCharacter> SourceHunter;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> MarkEffectClass;

	int32 SourceTeamID = -1;
	float BurstDamage = 95.0f;
	bool bDetonated = false;
	FTimerHandle MarkTickHandle;
	FTimerHandle DetonateTimerHandle;
};
