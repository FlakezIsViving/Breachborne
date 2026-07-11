#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBElunaReviveBeam.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UParticleSystemComponent;
class UDecalComponent;
class UMaterialInstanceDynamic;

/**
 * Eluna R revive beam — enhanced visual connecting Eluna to a target ally/wisp.
 * Concept: thick glowing beam with sparkles, source aura, target crescent moon.
 */
UCLASS()
class BREACHBORNE_API ABBElunaReviveBeam : public AActor
{
	GENERATED_BODY()

public:
	ABBElunaReviveBeam();

	void SetBeamTarget(const FVector& TargetLocation);
	void SetBeamSource(const FVector& SourceLocation);

	/** Update beam color based on distance-to-break ratio (0.0 = safe/green, 1.0 = breaking/red) */
	void SetDistanceRatio(float Ratio);

	/** Blueprint-assignable particle for beam sparkles ( Niagara or Cascade ) */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<class UParticleSystem> BeamParticles;

	/** Blueprint-assignable particle for source glow */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<class UParticleSystem> SourceGlowParticles;

	/** Blueprint-assignable particle for target glow */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<class UParticleSystem> TargetGlowParticles;

	/** Blueprint-assignable material for the crescent moon decal */
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<class UMaterialInterface> CrescentDecalMaterial;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Eluna|Beam")
	TObjectPtr<UStaticMeshComponent> BeamMesh;

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
	FVector CurrentSource = FVector::ZeroVector;
	FVector CurrentTarget = FVector::ZeroVector;
	bool bHasTarget = false;
	float BeamPulseTime = 0.0f;

	/** Cached dynamic material so we can change color at runtime */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BeamMaterialInstance;

	void UpdateBeamTransform();
};
