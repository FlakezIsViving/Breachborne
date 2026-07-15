#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBVoidSnapProjectile.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UAbilitySystemComponent;
class UGameplayEffect;
class AHunterCharacter;

UCLASS()
class BREACHBORNE_API ABBVoidSnapProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABBVoidSnapProjectile();

	virtual void Tick(float DeltaSeconds) override;

	void InitSnap(AHunterCharacter* InSourceHunter, UAbilitySystemComponent* InSourceASC,
		TSubclassOf<UGameplayEffect> InDamageGE, TSubclassOf<UGameplayEffect> InStunGE,
		int32 InTeamID, const FVector& InDirection, bool bInEmpowered);
	bool IsLocationInsideCone(const FVector& Location) const;
	float GetTravelSpeed() const { return Speed; }
	float GetConeLength() const { return ConeLength; }
	float GetConeRadius() const { return ConeRadius; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Void|Snap")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Void|Snap")
	TObjectPtr<UStaticMeshComponent> ConeMesh;

private:
	UFUNCTION()
	void OnRep_Empowered();

	void ApplySnapVisuals();
	void MoveAndHit(float DeltaSeconds);
	void TryHitTargets();
	bool IsActorInsideCone(AActor* Actor) const;
	void ApplyDamageAndStun(UAbilitySystemComponent* TargetASC, AActor* TargetActor);

	UPROPERTY()
	TWeakObjectPtr<AHunterCharacter> SourceHunter;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> StunEffectClass;

	FVector Direction = FVector::ForwardVector;
	int32 SourceTeamID = -1;
	float Speed = 2300.0f;
	float Lifetime = 0.75f;
	float Elapsed = 0.0f;
	float ConeLength = 360.0f;
	float ConeRadius = 155.0f;
	float Damage = 45.0f;
	float StunDuration = 1.0f;
	UPROPERTY(ReplicatedUsing = OnRep_Empowered)
	bool bEmpowered = false;
	bool bHitTarget = false;
};
