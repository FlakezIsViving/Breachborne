#include "BBMapDataAsset.h"

FVector2D UBBMapDataAsset::WorldToMapUV(const FVector& WorldLocation) const
{
	const float Width = FMath::Max(1.0f, WorldMax.X - WorldMin.X);
	const float Height = FMath::Max(1.0f, WorldMax.Y - WorldMin.Y);
	return FVector2D(
		FMath::Clamp((WorldLocation.X - WorldMin.X) / Width, 0.0f, 1.0f),
		FMath::Clamp((WorldLocation.Y - WorldMin.Y) / Height, 0.0f, 1.0f));
}
