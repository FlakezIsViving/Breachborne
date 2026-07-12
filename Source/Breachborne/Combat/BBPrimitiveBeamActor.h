#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBPrimitiveBeamActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class BREACHBORNE_API ABBPrimitiveBeamActor : public AActor
{
	GENERATED_BODY()

public:
	ABBPrimitiveBeamActor();

	void InitBeam(const FVector& Start, const FVector& End, float Radius, float LifetimeSeconds, const FLinearColor& Color);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_BeamState();

	void RefreshBeam();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PrimitiveVisual")
	TObjectPtr<UStaticMeshComponent> BeamMesh;

	UPROPERTY(ReplicatedUsing = OnRep_BeamState)
	FVector_NetQuantize10 BeamStart = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing = OnRep_BeamState)
	FVector_NetQuantize10 BeamEnd = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing = OnRep_BeamState)
	float BeamRadius = 1.0f;

	UPROPERTY(ReplicatedUsing = OnRep_BeamState)
	FLinearColor BeamColor = FLinearColor::White;
};
