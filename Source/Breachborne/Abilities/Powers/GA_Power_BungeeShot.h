#pragma once

#include "CoreMinimal.h"
#include "BBPowerGameplayAbility.h"
#include "GA_Power_BungeeShot.generated.h"

class AHunterCharacter;

USTRUCT()
struct FBBBungeeAnchor
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	bool bIsEnemyHunter = false;

	bool IsValid() const { return bIsEnemyHunter ? Actor.IsValid() : !Location.IsNearlyZero(); }
	void Reset()
	{
		Actor.Reset();
		Location = FVector::ZeroVector;
		bIsEnemyHunter = false;
	}
};

UCLASS()
class BREACHBORNE_API UGA_Power_BungeeShot : public UBBPowerGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Power_BungeeShot();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers")
	float ProjectileDamage = 35.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Bungee")
	float MaxRange = 2600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Bungee")
	float EnemyObjectPullSpeed = 2600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Bungee")
	float EnemyEnemyPullSpeed = 2200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Bungee")
	float CollisionStunRange = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Bungee")
	float StunDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Bungee")
	float ObjectCordDuration = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Powers|Bungee")
	float ObjectCordBounceSpeed = 2400.0f;

	UPROPERTY()
	FBBBungeeAnchor PendingAnchor;

	bool TraceBungeeAnchor(FBBBungeeAnchor& OutAnchor) const;
	bool IsEnemyHunter(AActor* Actor, AHunterCharacter*& OutHunter) const;
	void ResolveBungeeLink(const FBBBungeeAnchor& First, const FBBBungeeAnchor& Second);
	void PullHunterToward(AHunterCharacter* Hunter, const FVector& Target, float PullSpeed) const;
	void ApplyStun(AHunterCharacter* Hunter) const;
	void SpawnVisualCord(const FVector& Start, const FVector& End, float Duration, bool bElastic) const;
};
