#include "BBPrimitiveVisuals.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

namespace BBPrimitiveVisuals
{
	namespace
	{
		UMaterialInterface* GetBasicShapeMaterial()
		{
			static TWeakObjectPtr<UMaterialInterface> CachedMaterial;
			if (CachedMaterial.IsValid())
			{
				return CachedMaterial.Get();
			}

			UMaterialInterface* Material = LoadObject<UMaterialInterface>(
				nullptr,
				TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
			CachedMaterial = Material;
			return Material;
		}
	}

	void ApplyColor(UStaticMeshComponent* MeshComponent, const FLinearColor& Color)
	{
		if (!MeshComponent)
		{
			return;
		}

		if (UMaterialInterface* Material = GetBasicShapeMaterial())
		{
			MeshComponent->SetMaterial(0, Material);
		}

		MeshComponent->SetVisibility(true, true);
		MeshComponent->SetHiddenInGame(false);
		MeshComponent->SetCastShadow(false);
		MeshComponent->SetReceivesDecals(false);
		const FVector VectorColor(Color.R, Color.G, Color.B);
		MeshComponent->SetVectorParameterValueOnMaterials(TEXT("Color"), VectorColor);
		MeshComponent->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), VectorColor);
		MeshComponent->SetVectorParameterValueOnMaterials(TEXT("Base Color"), VectorColor);
		MeshComponent->SetVectorParameterValueOnMaterials(TEXT("Tint"), VectorColor);
	}
}
