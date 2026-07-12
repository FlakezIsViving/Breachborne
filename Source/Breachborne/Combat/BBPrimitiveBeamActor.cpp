#include "BBPrimitiveBeamActor.h"

#include "Breachborne/Combat/BBPrimitiveVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABBPrimitiveBeamActor::ABBPrimitiveBeamActor()
{
	bReplicates = true;
	SetReplicatingMovement(false);

	BeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeamMesh"));
	BeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(BeamMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		BeamMesh->SetStaticMesh(CylinderMesh.Object);
	}
	RefreshBeam();
}

void ABBPrimitiveBeamActor::InitBeam(const FVector& Start, const FVector& End, float Radius, float LifetimeSeconds, const FLinearColor& Color)
{
	BeamStart = Start;
	BeamEnd = End;
	BeamRadius = FMath::Max(1.0f, Radius);
	BeamColor = Color;
	const FVector Delta = FVector(BeamEnd) - FVector(BeamStart);
	const float Length = Delta.Size();
	if (Length <= KINDA_SMALL_NUMBER)
	{
		Destroy();
		return;
	}

	RefreshBeam();
	SetLifeSpan(FMath::Max(0.05f, LifetimeSeconds));
	ForceNetUpdate();
}

void ABBPrimitiveBeamActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBPrimitiveBeamActor, BeamStart);
	DOREPLIFETIME(ABBPrimitiveBeamActor, BeamEnd);
	DOREPLIFETIME(ABBPrimitiveBeamActor, BeamRadius);
	DOREPLIFETIME(ABBPrimitiveBeamActor, BeamColor);
}

void ABBPrimitiveBeamActor::OnRep_BeamState()
{
	RefreshBeam();
}

void ABBPrimitiveBeamActor::RefreshBeam()
{
	if (!BeamMesh)
	{
		return;
	}

	const FVector Start(BeamStart);
	const FVector End(BeamEnd);
	const FVector Delta = End - Start;
	const float Length = Delta.Size();
	if (Length <= KINDA_SMALL_NUMBER)
	{
		BeamMesh->SetVisibility(false, true);
		return;
	}

	BeamMesh->SetVisibility(true, true);
	const FVector Direction = Delta / Length;
	SetActorLocation((Start + End) * 0.5f);
	SetActorRotation(FQuat::FindBetweenNormals(FVector::UpVector, Direction).Rotator());
	const float RadiusScale = BeamRadius / 50.0f;
	BeamMesh->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, Length / 100.0f));
	BBPrimitiveVisuals::ApplyColor(BeamMesh, BeamColor);
}
