#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BBBungeeCordActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class BREACHBORNE_API ABBBungeeCordActor : public AActor
{
	GENERATED_BODY()

public:
	ABBBungeeCordActor();

	void InitCord(AActor* InSourceOwner, const FVector& InStart, const FVector& InEnd, float InDuration, float InLaunchSpeed);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnCordOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	void UpdateCordTransform();

	UPROPERTY(VisibleAnywhere, Category = "Breachborne|Powers|Bungee")
	TObjectPtr<UBoxComponent> CordCollision;

	UPROPERTY(VisibleAnywhere, Category = "Breachborne|Powers|Bungee")
	TObjectPtr<UStaticMeshComponent> CordMesh;

	UPROPERTY(VisibleAnywhere, Category = "Breachborne|Powers|Bungee")
	TObjectPtr<UStaticMeshComponent> StartAnchorMesh;

	UPROPERTY(VisibleAnywhere, Category = "Breachborne|Powers|Bungee")
	TObjectPtr<UStaticMeshComponent> EndAnchorMesh;

	UPROPERTY()
	TWeakObjectPtr<AActor> SourceOwner;

	FVector StartPoint = FVector::ZeroVector;
	FVector EndPoint = FVector::ZeroVector;
	float LaunchSpeed = 2200.0f;
};
