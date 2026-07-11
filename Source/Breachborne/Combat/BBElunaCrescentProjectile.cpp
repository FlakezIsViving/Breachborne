#include "BBElunaCrescentProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ABBElunaCrescentProjectile::ABBElunaCrescentProjectile()
{
	ProjectileMesh->SetRelativeScale3D(FVector(0.4f));

	static ConstructorHelpers::FObjectFinder<UMaterial> BasicMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (BasicMat.Succeeded())
	{
		UMaterialInstanceDynamic* WhiteMat = UMaterialInstanceDynamic::Create(BasicMat.Object, this);
		if (WhiteMat)
		{
			WhiteMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(2.0f, 2.0f, 2.0f));
			ProjectileMesh->SetMaterial(0, WhiteMat);
		}
	}
}
