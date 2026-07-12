#include "BBElunaCrescentProjectile.h"
#include "Breachborne/Combat/BBPrimitiveVisuals.h"
#include "Components/StaticMeshComponent.h"

ABBElunaCrescentProjectile::ABBElunaCrescentProjectile()
{
	ProjectileMesh->SetRelativeScale3D(FVector(0.45f, 0.16f, 0.52f));
	BBPrimitiveVisuals::ApplyColor(ProjectileMesh, FLinearColor(0.86f, 0.92f, 1.0f, 1.0f));
}
