#include "BBElunaReviveBeam.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/DecalComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Particles/ParticleSystem.h"

ABBElunaReviveBeam::ABBElunaReviveBeam()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	// Beam mesh — thicker cylinder with emissive material
	BeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeamMesh"));
	BeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(BeamMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		BeamMesh->SetStaticMesh(CylinderMesh.Object);
	}

	// Create a dynamic emissive material
	static ConstructorHelpers::FObjectFinder<UMaterial> BasicMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (BasicMat.Succeeded())
	{
		BeamMaterialInstance = UMaterialInstanceDynamic::Create(BasicMat.Object, this);
		if (BeamMaterialInstance)
		{
			BeamMaterialInstance->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 0.6f, 1.0f));
			BeamMesh->SetMaterial(0, BeamMaterialInstance);
		}
	}

	BeamMesh->SetRelativeScale3D(FVector(0.15f, 0.15f, 1.0f));

	// Source glow light
	SourceLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SourceLight"));
	SourceLight->SetupAttachment(BeamMesh);
	SourceLight->SetRelativeLocation(FVector(0.0f, 0.0f, -500.0f));
	SourceLight->SetLightColor(FLinearColor(0.3f, 0.7f, 1.0f));
	SourceLight->SetIntensity(5000.0f);
	SourceLight->SetAttenuationRadius(300.0f);
	SourceLight->SetSourceRadius(50.0f);
	SourceLight->bUseInverseSquaredFalloff = false;
	SourceLight->SetCastShadows(false);

	// Target glow light
	TargetLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TargetLight"));
	TargetLight->SetupAttachment(BeamMesh);
	TargetLight->SetRelativeLocation(FVector(0.0f, 0.0f, 500.0f));
	TargetLight->SetLightColor(FLinearColor(0.3f, 0.7f, 1.0f));
	TargetLight->SetIntensity(3000.0f);
	TargetLight->SetAttenuationRadius(200.0f);
	TargetLight->SetSourceRadius(30.0f);
	TargetLight->bUseInverseSquaredFalloff = false;
	TargetLight->SetCastShadows(false);

	// Particle system placeholders
	BeamPSC = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BeamPSC"));
	BeamPSC->SetupAttachment(BeamMesh);
	BeamPSC->SetRelativeLocation(FVector::ZeroVector);
	BeamPSC->bAutoActivate = true;

	SourcePSC = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("SourcePSC"));
	SourcePSC->SetupAttachment(BeamMesh);
	SourcePSC->SetRelativeLocation(FVector(0.0f, 0.0f, -500.0f));
	SourcePSC->bAutoActivate = true;

	TargetPSC = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TargetPSC"));
	TargetPSC->SetupAttachment(BeamMesh);
	TargetPSC->SetRelativeLocation(FVector(0.0f, 0.0f, 500.0f));
	TargetPSC->bAutoActivate = true;

	// Crescent moon decal at target
	TargetDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("TargetDecal"));
	TargetDecal->SetupAttachment(BeamMesh);
	TargetDecal->SetRelativeLocation(FVector(0.0f, 0.0f, 500.0f));
	TargetDecal->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	TargetDecal->DecalSize = FVector(150.0f, 150.0f, 150.0f);
	TargetDecal->SetVisibility(false);
}

void ABBElunaReviveBeam::BeginPlay()
{
	Super::BeginPlay();

	if (BeamParticles && BeamPSC)
	{
		BeamPSC->SetTemplate(BeamParticles);
	}
	if (SourceGlowParticles && SourcePSC)
	{
		SourcePSC->SetTemplate(SourceGlowParticles);
	}
	if (TargetGlowParticles && TargetPSC)
	{
		TargetPSC->SetTemplate(TargetGlowParticles);
	}
	if (CrescentDecalMaterial && TargetDecal)
	{
		TargetDecal->SetDecalMaterial(CrescentDecalMaterial);
		TargetDecal->SetVisibility(true);
	}
}

void ABBElunaReviveBeam::SetBeamTarget(const FVector& TargetLocation)
{
	CurrentTarget = TargetLocation;
	bHasTarget = true;
}

void ABBElunaReviveBeam::SetBeamSource(const FVector& SourceLocation)
{
	CurrentSource = SourceLocation;
}

void ABBElunaReviveBeam::SetDistanceRatio(float Ratio)
{
	if (!BeamMaterialInstance)
	{
		return;
	}

	Ratio = FMath::Clamp(Ratio, 0.0f, 1.0f);

	// Green (safe) -> Yellow (mid) -> Red (breaking)
	// Use two-stage lerp: green->yellow for 0-0.5, yellow->red for 0.5-1.0
	FLinearColor Color;
	if (Ratio <= 0.5f)
	{
		const float T = Ratio * 2.0f;
		Color = FMath::Lerp(FLinearColor(0.0f, 1.0f, 0.0f), FLinearColor(1.0f, 1.0f, 0.0f), T);
	}
	else
	{
		const float T = (Ratio - 0.5f) * 2.0f;
		Color = FMath::Lerp(FLinearColor(1.0f, 1.0f, 0.0f), FLinearColor(1.0f, 0.0f, 0.0f), T);
	}

	BeamMaterialInstance->SetVectorParameterValue(TEXT("Color"), Color);

	// Also tint the lights to match
	if (SourceLight)
	{
		SourceLight->SetLightColor(Color);
	}
	if (TargetLight)
	{
		TargetLight->SetLightColor(Color);
	}
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
	const FVector Direction = CurrentTarget - CurrentSource;
	const float Distance = Direction.Size();

	if (Distance < 1.0f)
	{
		return;
	}

	const FVector MidPoint = (CurrentSource + CurrentTarget) * 0.5f;
	const FRotator BeamRot = Direction.GetSafeNormal().Rotation() + FRotator(90.0f, 0.0f, 0.0f);

	SetActorLocation(MidPoint);
	SetActorRotation(BeamRot);

	// Pulsing thickness
	const float Pulse = 1.0f + 0.15f * FMath::Sin(BeamPulseTime * 4.0f);
	BeamMesh->SetRelativeScale3D(FVector(0.12f * Pulse, 0.12f * Pulse, Distance * 0.5f));

	// Update light positions to source/target
	if (SourceLight)
	{
		SourceLight->SetRelativeLocation(FVector(0.0f, 0.0f, -Distance * 0.5f));
	}
	if (TargetLight)
	{
		TargetLight->SetRelativeLocation(FVector(0.0f, 0.0f, Distance * 0.5f));
	}
	if (SourcePSC)
	{
		SourcePSC->SetRelativeLocation(FVector(0.0f, 0.0f, -Distance * 0.5f));
	}
	if (TargetPSC)
	{
		TargetPSC->SetRelativeLocation(FVector(0.0f, 0.0f, Distance * 0.5f));
	}
	if (TargetDecal)
	{
		TargetDecal->SetRelativeLocation(FVector(0.0f, 0.0f, Distance * 0.5f));
	}
}
