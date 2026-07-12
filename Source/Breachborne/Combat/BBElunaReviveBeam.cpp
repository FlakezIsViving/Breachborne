#include "BBElunaReviveBeam.h"
#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void ConfigureVisualMesh(UStaticMeshComponent* Mesh)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetReceivesDecals(false);
	}

	void SetMeshColorAndOpacity(UStaticMeshComponent* Mesh, const FLinearColor& Color, float Opacity)
	{
		if (!Mesh)
		{
			return;
		}
		const FVector VectorColor(Color.R, Color.G, Color.B);
		Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), VectorColor);
		Mesh->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), VectorColor);
		Mesh->SetVectorParameterValueOnMaterials(TEXT("Tint"), VectorColor);
		Mesh->SetScalarParameterValueOnMaterials(TEXT("Opacity"), Opacity);
		Mesh->SetScalarParameterValueOnMaterials(TEXT("Alpha"), Opacity);
	}
}

ABBElunaReviveBeam::ABBElunaReviveBeam()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(15.0f);
	SetMinNetUpdateFrequency(10.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeamMesh"));
	BeamMesh->SetupAttachment(SceneRoot);
	ConfigureVisualMesh(BeamMesh);

	CoreBeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoreBeamMesh"));
	CoreBeamMesh->SetupAttachment(SceneRoot);
	ConfigureVisualMesh(CoreBeamMesh);

	SourceHaloMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SourceHaloMesh"));
	SourceHaloMesh->SetupAttachment(SceneRoot);
	ConfigureVisualMesh(SourceHaloMesh);

	TargetHaloMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetHaloMesh"));
	TargetHaloMesh->SetupAttachment(SceneRoot);
	ConfigureVisualMesh(TargetHaloMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		BeamMesh->SetStaticMesh(CylinderMesh.Object);
		CoreBeamMesh->SetStaticMesh(CylinderMesh.Object);
		SourceHaloMesh->SetStaticMesh(CylinderMesh.Object);
		TargetHaloMesh->SetStaticMesh(CylinderMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentMaterial(
		TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent"));
	if (TranslucentMaterial.Succeeded())
	{
		BeamMesh->SetMaterial(0, TranslucentMaterial.Object);
		SourceHaloMesh->SetMaterial(0, TranslucentMaterial.Object);
		TargetHaloMesh->SetMaterial(0, TranslucentMaterial.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CoreMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (CoreMaterial.Succeeded())
	{
		CoreBeamMesh->SetMaterial(0, CoreMaterial.Object);
	}

	SourceLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SourceLight"));
	SourceLight->SetupAttachment(SceneRoot);
	SourceLight->SetIntensity(250.0f);
	SourceLight->SetAttenuationRadius(140.0f);
	SourceLight->bUseInverseSquaredFalloff = false;
	SourceLight->SetCastShadows(false);

	TargetLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TargetLight"));
	TargetLight->SetupAttachment(SceneRoot);
	TargetLight->SetIntensity(250.0f);
	TargetLight->SetAttenuationRadius(140.0f);
	TargetLight->bUseInverseSquaredFalloff = false;
	TargetLight->SetCastShadows(false);

	BeamPSC = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BeamPSC"));
	BeamPSC->SetupAttachment(SceneRoot);
	BeamPSC->bAutoActivate = true;

	SourcePSC = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SourcePSC"));
	SourcePSC->SetupAttachment(SceneRoot);
	SourcePSC->bAutoActivate = true;

	TargetPSC = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TargetPSC"));
	TargetPSC->SetupAttachment(SceneRoot);
	TargetPSC->bAutoActivate = true;

	TargetDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("TargetDecal"));
	TargetDecal->SetupAttachment(SceneRoot);
	TargetDecal->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	TargetDecal->DecalSize = FVector(90.0f, 90.0f, 90.0f);
	TargetDecal->SetVisibility(false);
}

void ABBElunaReviveBeam::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBElunaReviveBeam, CurrentSource);
	DOREPLIFETIME(ABBElunaReviveBeam, CurrentTarget);
	DOREPLIFETIME(ABBElunaReviveBeam, bHasTarget);
	DOREPLIFETIME(ABBElunaReviveBeam, DistanceRatio);
}

void ABBElunaReviveBeam::BeginPlay()
{
	Super::BeginPlay();
	if (BeamParticles)
	{
		BeamPSC->SetTemplate(BeamParticles);
	}
	if (SourceGlowParticles)
	{
		SourcePSC->SetTemplate(SourceGlowParticles);
	}
	if (TargetGlowParticles)
	{
		TargetPSC->SetTemplate(TargetGlowParticles);
	}
	if (CrescentDecalMaterial)
	{
		TargetDecal->SetDecalMaterial(CrescentDecalMaterial);
		TargetDecal->SetVisibility(true);
	}
	ApplyVisualStyle();
	UpdateBeamTransform();
}

void ABBElunaReviveBeam::SetBeamTarget(const FVector& TargetLocation)
{
	CurrentTarget = TargetLocation;
	bHasTarget = true;
	UpdateBeamTransform();
}

void ABBElunaReviveBeam::SetBeamSource(const FVector& SourceLocation)
{
	CurrentSource = SourceLocation;
	UpdateBeamTransform();
}

void ABBElunaReviveBeam::SetDistanceRatio(float Ratio)
{
	DistanceRatio = FMath::Clamp(Ratio, 0.0f, 1.0f);
	ApplyVisualStyle();
}

void ABBElunaReviveBeam::OnRep_BeamState()
{
	ApplyVisualStyle();
	UpdateBeamTransform();
}

void ABBElunaReviveBeam::ApplyVisualStyle()
{
	const FLinearColor SafeColor(0.58f, 0.82f, 1.0f, 1.0f);
	const FLinearColor EdgeColor(1.0f, 0.48f, 0.78f, 1.0f);
	const FLinearColor BeamColor = FMath::Lerp(SafeColor, EdgeColor, DistanceRatio);
	const FLinearColor CoreColor = FMath::Lerp(FLinearColor::White, BeamColor, 0.35f);
	SetMeshColorAndOpacity(BeamMesh, BeamColor, 0.5f);
	SetMeshColorAndOpacity(CoreBeamMesh, CoreColor, 1.0f);
	SetMeshColorAndOpacity(SourceHaloMesh, BeamColor, 0.35f);
	SetMeshColorAndOpacity(TargetHaloMesh, BeamColor, 0.35f);
	SourceLight->SetLightColor(BeamColor);
	TargetLight->SetLightColor(BeamColor);
}

void ABBElunaReviveBeam::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bHasTarget)
	{
		return;
	}
	BeamPulseTime += DeltaTime;
	UpdateBeamTransform();
}

void ABBElunaReviveBeam::UpdateBeamTransform()
{
	if (!bHasTarget)
	{
		return;
	}
	const FVector Source = CurrentSource;
	const FVector Target = CurrentTarget;
	const FVector Delta = Target - Source;
	const float Distance = Delta.Size();
	if (Distance < 1.0f)
	{
		return;
	}

	const FVector MidPoint = (Source + Target) * 0.5f;
	const FRotator BeamRotation = FQuat::FindBetweenNormals(FVector::UpVector, Delta / Distance).Rotator();
	const float Pulse = 1.0f + 0.05f * FMath::Sin(BeamPulseTime * 5.0f);
	const float CylinderLengthScale = Distance / 100.0f;

	BeamMesh->SetWorldLocationAndRotation(MidPoint, BeamRotation);
	BeamMesh->SetWorldScale3D(FVector(0.14f * Pulse, 0.14f * Pulse, CylinderLengthScale));
	CoreBeamMesh->SetWorldLocationAndRotation(MidPoint, BeamRotation);
	CoreBeamMesh->SetWorldScale3D(FVector(0.035f, 0.035f, CylinderLengthScale * 1.002f));

	SourceHaloMesh->SetWorldLocation(Source + FVector(0.0f, 0.0f, 4.0f));
	SourceHaloMesh->SetWorldRotation(FRotator::ZeroRotator);
	SourceHaloMesh->SetWorldScale3D(FVector(1.8f, 1.8f, 0.025f));
	TargetHaloMesh->SetWorldLocation(Target + FVector(0.0f, 0.0f, 4.0f));
	TargetHaloMesh->SetWorldRotation(FRotator::ZeroRotator);
	TargetHaloMesh->SetWorldScale3D(FVector(1.8f, 1.8f, 0.025f));

	SourceLight->SetWorldLocation(Source + FVector(0.0f, 0.0f, 35.0f));
	TargetLight->SetWorldLocation(Target + FVector(0.0f, 0.0f, 35.0f));
	BeamPSC->SetWorldLocation(MidPoint);
	SourcePSC->SetWorldLocation(Source);
	TargetPSC->SetWorldLocation(Target);
	TargetDecal->SetWorldLocation(Target + FVector(0.0f, 0.0f, 8.0f));
}
