#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBElunaRootZone.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

/**
 * Eluna RMB root zone — orange circle that grows at the enemy's base,
 * then applies damage + root to enemies in radius.
 */
UCLASS()
class BREACHBORNE_API ABBElunaRootZone : public AActor
{
	GENERATED_BODY()

public:
	ABBElunaRootZone();

	void InitZone(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageGE, float InDamage, int32 InTeamID, float InRadius, float InDuration);

protected:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|RootZone")
	TObjectPtr<USphereComponent> RootSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|RootZone")
	TObjectPtr<UStaticMeshComponent> CircleMesh;

private:
	void ApplyRootAndDamage();

	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	float BaseDamage = 0.0f;
	int32 SourceTeamID = -1;
	float RootRadius = 300.0f;
	float RootDuration = 1.5f;
	float ElapsedTime = 0.0f;
	bool bInitialized = false;
	bool bHasPopped = false;
};
