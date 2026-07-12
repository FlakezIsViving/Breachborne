#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBPrimitiveWedgeActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/** Replicated low-cost sector outline used when authored cone VFX are unavailable. */
UCLASS()
class BREACHBORNE_API ABBPrimitiveWedgeActor : public AActor
{
	GENERATED_BODY()

public:
	ABBPrimitiveWedgeActor();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitWedge(const FVector& InOrigin, const FVector& InDirection, float InRange,
		float InHalfAngleDegrees, float InThickness, float LifetimeSeconds, const FLinearColor& InColor);
	void UpdateWedge(float InRange, const FLinearColor& InColor);

private:
	static constexpr int32 ArcSegments = 8;
	void RefreshWedge();
	void SetSegment(UStaticMeshComponent* Segment, const FVector& Start, const FVector& End) const;

	UFUNCTION()
	void OnRep_WedgeState();

	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Segments;

	UPROPERTY(ReplicatedUsing = OnRep_WedgeState)
	FVector_NetQuantize Origin = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing = OnRep_WedgeState)
	FVector_NetQuantizeNormal Direction = FVector::ForwardVector;

	UPROPERTY(ReplicatedUsing = OnRep_WedgeState)
	float Range = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_WedgeState)
	float HalfAngleDegrees = 30.0f;

	UPROPERTY(ReplicatedUsing = OnRep_WedgeState)
	float Thickness = 5.0f;

	UPROPERTY(ReplicatedUsing = OnRep_WedgeState)
	FLinearColor WedgeColor = FLinearColor::White;
};
