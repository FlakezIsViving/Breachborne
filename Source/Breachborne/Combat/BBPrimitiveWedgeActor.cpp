#include "BBPrimitiveWedgeActor.h"

#include "Breachborne/Combat/BBPrimitiveVisuals.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABBPrimitiveWedgeActor::ABBPrimitiveWedgeActor()
{
	bReplicates = true;
	SetReplicatingMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	for (int32 Index = 0; Index < ArcSegments + 2; ++Index)
	{
		UStaticMeshComponent* Segment = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Segment_%02d"), Index));
		Segment->SetupAttachment(SceneRoot);
		Segment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Segment->SetCastShadow(false);
		Segment->SetVisibility(false, true);
		if (CylinderMesh.Succeeded())
		{
			Segment->SetStaticMesh(CylinderMesh.Object);
		}
		Segments.Add(Segment);
	}
}

void ABBPrimitiveWedgeActor::InitWedge(const FVector& InOrigin, const FVector& InDirection, float InRange,
	float InHalfAngleDegrees, float InThickness, float LifetimeSeconds, const FLinearColor& InColor)
{
	Origin = InOrigin + FVector(0.0f, 0.0f, 6.0f);
	Direction = InDirection.GetSafeNormal2D();
	if (FVector(Direction).IsNearlyZero())
	{
		Direction = FVector::ForwardVector;
	}
	Range = FMath::Max(1.0f, InRange);
	HalfAngleDegrees = FMath::Clamp(InHalfAngleDegrees, 1.0f, 179.0f);
	Thickness = FMath::Max(1.0f, InThickness);
	WedgeColor = InColor;
	RefreshWedge();
	SetLifeSpan(FMath::Max(0.05f, LifetimeSeconds));
	ForceNetUpdate();
}

void ABBPrimitiveWedgeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBPrimitiveWedgeActor, Origin);
	DOREPLIFETIME(ABBPrimitiveWedgeActor, Direction);
	DOREPLIFETIME(ABBPrimitiveWedgeActor, Range);
	DOREPLIFETIME(ABBPrimitiveWedgeActor, HalfAngleDegrees);
	DOREPLIFETIME(ABBPrimitiveWedgeActor, Thickness);
	DOREPLIFETIME(ABBPrimitiveWedgeActor, WedgeColor);
}

void ABBPrimitiveWedgeActor::UpdateWedge(float InRange, const FLinearColor& InColor)
{
	Range = FMath::Max(1.0f, InRange);
	WedgeColor = InColor;
	RefreshWedge();
	ForceNetUpdate();
}

void ABBPrimitiveWedgeActor::OnRep_WedgeState()
{
	RefreshWedge();
}

void ABBPrimitiveWedgeActor::RefreshWedge()
{
	if (Segments.Num() != ArcSegments + 2 || Range <= KINDA_SMALL_NUMBER)
	{
		for (UStaticMeshComponent* Segment : Segments)
		{
			if (Segment)
			{
				Segment->SetVisibility(false, true);
			}
		}
		return;
	}

	const FVector FlatDirection = FVector(Direction).GetSafeNormal2D();
	const float BaseYaw = FMath::Atan2(FlatDirection.Y, FlatDirection.X);
	const float HalfAngleRadians = FMath::DegreesToRadians(HalfAngleDegrees);
	auto PointAt = [this, BaseYaw](float Angle)
	{
		return FVector(Origin) + FVector(FMath::Cos(BaseYaw + Angle), FMath::Sin(BaseYaw + Angle), 0.0f) * Range;
	};

	const FVector Left = PointAt(-HalfAngleRadians);
	const FVector Right = PointAt(HalfAngleRadians);
	SetSegment(Segments[0], FVector(Origin), Left);
	SetSegment(Segments[1], FVector(Origin), Right);

	FVector Previous = Left;
	for (int32 Index = 1; Index <= ArcSegments; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / ArcSegments;
		const FVector Current = PointAt(FMath::Lerp(-HalfAngleRadians, HalfAngleRadians, Alpha));
		SetSegment(Segments[Index + 1], Previous, Current);
		Previous = Current;
	}
}

void ABBPrimitiveWedgeActor::SetSegment(UStaticMeshComponent* Segment, const FVector& Start, const FVector& End) const
{
	if (!Segment)
	{
		return;
	}
	const FVector Delta = End - Start;
	const float Length = Delta.Size();
	if (Length <= KINDA_SMALL_NUMBER)
	{
		Segment->SetVisibility(false, true);
		return;
	}
	Segment->SetVisibility(true, true);
	Segment->SetWorldLocationAndRotation((Start + End) * 0.5f,
		FQuat::FindBetweenNormals(FVector::UpVector, Delta / Length).Rotator());
	Segment->SetWorldScale3D(FVector(Thickness / 50.0f, Thickness / 50.0f, Length / 100.0f));
	BBPrimitiveVisuals::ApplyColor(Segment, WedgeColor);
}
