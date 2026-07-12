#include "BBPrimitiveBurstActor.h"

#include "Breachborne/Combat/BBPrimitiveVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABBPrimitiveBurstActor::ABBPrimitiveBurstActor()
{
	bReplicates = true;
	SetReplicatingMovement(false);

	BurstMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BurstMesh"));
	BurstMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(BurstMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		BurstMesh->SetStaticMesh(SphereMesh.Object);
	}
	RefreshVisuals();
}

void ABBPrimitiveBurstActor::InitBurst(const FVector& Location, float Radius, float LifetimeSeconds,
	const FLinearColor& Color, bool bFlattenToDisc)
{
	SetActorLocation(Location);
	VisualLocation = Location;
	VisualRadius = FMath::Max(1.0f, Radius);
	VisualColor = Color;
	bDisc = bFlattenToDisc;
	bVisualInitialized = true;
	RefreshVisuals();
	SetLifeSpan(FMath::Max(0.05f, LifetimeSeconds));
	ForceNetUpdate();
}

void ABBPrimitiveBurstActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBPrimitiveBurstActor, VisualRadius);
	DOREPLIFETIME(ABBPrimitiveBurstActor, VisualColor);
	DOREPLIFETIME(ABBPrimitiveBurstActor, bDisc);
	DOREPLIFETIME(ABBPrimitiveBurstActor, VisualLocation);
	DOREPLIFETIME(ABBPrimitiveBurstActor, bVisualInitialized);
}

void ABBPrimitiveBurstActor::OnRep_VisualState()
{
	RefreshVisuals();
}

void ABBPrimitiveBurstActor::RefreshVisuals()
{
	if (!BurstMesh)
	{
		return;
	}
	if (!bVisualInitialized)
	{
		BurstMesh->SetVisibility(false, true);
		return;
	}

	const float RadiusScale = VisualRadius / 50.0f;
	SetActorLocation(FVector(VisualLocation));
	BurstMesh->SetVisibility(true, true);
	BurstMesh->SetRelativeScale3D(bDisc
		? FVector(RadiusScale, RadiusScale, 0.06f)
		: FVector(RadiusScale));
	BBPrimitiveVisuals::ApplyColor(BurstMesh, VisualColor);
}
