#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBPrimitiveBurstActor.generated.h"

class UStaticMeshComponent;

/** Replicated short-lived sphere/disc used when a GameplayCue asset is unavailable. */
UCLASS()
class BREACHBORNE_API ABBPrimitiveBurstActor : public AActor
{
	GENERATED_BODY()

public:
	ABBPrimitiveBurstActor();

	void InitBurst(const FVector& Location, float Radius, float LifetimeSeconds,
		const FLinearColor& Color, bool bFlattenToDisc = false);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_VisualState();

	void RefreshVisuals();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|PrimitiveVisual")
	TObjectPtr<UStaticMeshComponent> BurstMesh;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	float VisualRadius = 50.0f;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	FLinearColor VisualColor = FLinearColor::White;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	bool bDisc = false;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	FVector_NetQuantize10 VisualLocation = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing = OnRep_VisualState)
	bool bVisualInitialized = false;
};
