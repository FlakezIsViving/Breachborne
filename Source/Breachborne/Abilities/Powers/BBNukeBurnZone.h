#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBNukeBurnZone.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class BREACHBORNE_API ABBNukeBurnZone : public AActor
{
	GENERATED_BODY()

public:
	ABBNukeBurnZone();

	void InitBurnZone(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageGE, float InRadius,
		float InDuration, float InDamagePerTick, float InTickInterval);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	TObjectPtr<USphereComponent> ZoneCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Powers|Nuke")
	TObjectPtr<UStaticMeshComponent> ZoneVisualMesh;

private:
	void DamageTick();
	void ExpireZone();
	void RefreshVisualScale(float Radius);

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	float DamagePerTick = 0.0f;
	float DamageTickInterval = 0.5f;

	FTimerHandle DamageTickHandle;
	FTimerHandle DurationHandle;
};
