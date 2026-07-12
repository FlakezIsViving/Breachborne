#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBVoidSingularityActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UAbilitySystemComponent;
class UGameplayEffect;

UCLASS()
class BREACHBORNE_API ABBVoidSingularityActor : public AActor
{
	GENERATED_BODY()

public:
	ABBVoidSingularityActor();

	void InitSingularity(UAbilitySystemComponent* InSourceASC, AActor* InInstigatorActor, TSubclassOf<UGameplayEffect> InStunGE,
		int32 InTeamID, float InRadius, float InWarningDelay, float InActiveDuration, bool bEmpowered);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Void|Singularity")
	TObjectPtr<USphereComponent> PullSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Void|Singularity")
	TObjectPtr<UStaticMeshComponent> VortexMesh;

	UPROPERTY(ReplicatedUsing = OnRep_Visuals)
	float Radius = 620.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Visuals)
	bool bActive = false;

private:
	UFUNCTION()
	void OnRep_Visuals();

	void ActivateVortex();
	void FinishVortex();
	void PullTick(float DeltaSeconds);
	void RefreshVisuals();
	void ApplyStun(UAbilitySystemComponent* TargetASC) const;

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	UPROPERTY()
	TWeakObjectPtr<AActor> InstigatorActor;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> StunEffectClass;

	int32 SourceTeamID = -1;
	float WarningDelay = 0.45f;
	float ActiveDuration = 1.75f;
	float PullSpeed = 820.0f;
	float StunDuration = 0.9f;
	TSet<TWeakObjectPtr<AActor>> StunnedActors;
	FTimerHandle ActivateHandle;
	FTimerHandle FinishHandle;
};
