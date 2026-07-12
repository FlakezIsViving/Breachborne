#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBElunaReviveBeam.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UParticleSystemComponent;
class UDecalComponent;

/**
 * Replicated Eluna R tether visual. A narrow translucent beam links Eluna and
 * her target while restrained endpoint halos identify the channel participants.
 */
UCLASS()
class BREACHBORNE_API ABBElunaReviveBeam : public AActor
{
	GENERATED_BODY()

public:
	ABBElunaReviveBeam();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetBeamTarget(const FVector& TargetLocation);
	void SetBeamSource(const FVector& SourceLocation);
	void SetDistanceRatio(float Ratio);

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<class UParticleSystem> BeamParticles;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<class UParticleSystem> SourceGlowParticles;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<class UParticleSystem> TargetGlowParticles;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<class UMaterialInterface> CrescentDecalMaterial;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<UStaticMeshComponent> BeamMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<UStaticMeshComponent> CoreBeamMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<UStaticMeshComponent> SourceHaloMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<UStaticMeshComponent> TargetHaloMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<UPointLightComponent> SourceLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<UPointLightComponent> TargetLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<UParticleSystemComponent> BeamPSC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<UParticleSystemComponent> SourcePSC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<UParticleSystemComponent> TargetPSC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<UDecalComponent> TargetDecal;

private:
	UPROPERTY(ReplicatedUsing = OnRep_BeamState)
	FVector_NetQuantize CurrentSource = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing = OnRep_BeamState)
	FVector_NetQuantize CurrentTarget = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing = OnRep_BeamState)
	bool bHasTarget = false;

	UPROPERTY(ReplicatedUsing = OnRep_BeamState)
	float DistanceRatio = 0.0f;

	UFUNCTION()
	void OnRep_BeamState();

	float BeamPulseTime = 0.0f;

	void UpdateBeamTransform();
	void ApplyVisualStyle();
};
