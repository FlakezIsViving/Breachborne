#include "BBBungeeCordActor.h"

#include "Breachborne/Breachborne.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ABBBungeeCordActor::ABBBungeeCordActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CordCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("CordCollision"));
	CordCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CordCollision->SetCollisionObjectType(ECC_WorldDynamic);
	CordCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	CordCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CordCollision->SetGenerateOverlapEvents(true);
	SetRootComponent(CordCollision);

	CordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CordMesh"));
	CordMesh->SetupAttachment(CordCollision);
	CordMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	StartAnchorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StartAnchorMesh"));
	StartAnchorMesh->SetupAttachment(CordCollision);
	StartAnchorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StartAnchorMesh->SetRelativeScale3D(FVector(0.55f));

	EndAnchorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EndAnchorMesh"));
	EndAnchorMesh->SetupAttachment(CordCollision);
	EndAnchorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EndAnchorMesh->SetRelativeScale3D(FVector(0.55f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		CordMesh->SetStaticMesh(CylinderMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		StartAnchorMesh->SetStaticMesh(SphereMesh.Object);
		EndAnchorMesh->SetStaticMesh(SphereMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (BasicMaterial.Succeeded())
	{
		UMaterialInstanceDynamic* PurpleMaterial = UMaterialInstanceDynamic::Create(BasicMaterial.Object, this);
		if (PurpleMaterial)
		{
			PurpleMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.65f, 0.15f, 1.0f, 1.0f));
			CordMesh->SetMaterial(0, PurpleMaterial);
			StartAnchorMesh->SetMaterial(0, PurpleMaterial);
			EndAnchorMesh->SetMaterial(0, PurpleMaterial);
		}
	}
}

void ABBBungeeCordActor::BeginPlay()
{
	Super::BeginPlay();
	CordCollision->OnComponentBeginOverlap.AddDynamic(this, &ABBBungeeCordActor::OnCordOverlap);
}

void ABBBungeeCordActor::InitCord(AActor* InSourceOwner, const FVector& InStart, const FVector& InEnd, float InDuration, float InLaunchSpeed)
{
	SourceOwner = InSourceOwner;
	StartPoint = InStart;
	EndPoint = InEnd;
	LaunchSpeed = InLaunchSpeed;
	UpdateCordTransform();
	SetLifeSpan(FMath::Max(0.5f, InDuration));
}

void ABBBungeeCordActor::UpdateCordTransform()
{
	FVector VisualStart = StartPoint;
	FVector VisualEnd = EndPoint;
	const float SharedZ = FMath::Max(VisualStart.Z, VisualEnd.Z);
	VisualStart.Z = SharedZ;
	VisualEnd.Z = SharedZ;

	const FVector Delta = VisualEnd - VisualStart;
	const float Length = Delta.Size();
	if (Length <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector Mid = (VisualStart + VisualEnd) * 0.5f;
	const FRotator Rotation = FRotationMatrix::MakeFromZ(Delta).Rotator();
	SetActorLocationAndRotation(Mid, Rotation);

	CordCollision->SetBoxExtent(FVector(55.0f, 150.0f, Length * 0.5f));
	CordMesh->SetRelativeRotation(FRotator::ZeroRotator);
	CordMesh->SetRelativeScale3D(FVector(0.09f, 0.09f, Length / 100.0f));
	StartAnchorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -Length * 0.5f));
	EndAnchorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, Length * 0.5f));
}

void ABBBungeeCordActor::OnCordOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !OtherActor)
	{
		return;
	}

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(OtherActor);
	if (!Hunter)
	{
		return;
	}

	FVector VisualStart = StartPoint;
	FVector VisualEnd = EndPoint;
	const float SharedZ = FMath::Max(VisualStart.Z, VisualEnd.Z);
	VisualStart.Z = SharedZ;
	VisualEnd.Z = SharedZ;
	const FVector Segment = VisualEnd - VisualStart;
	const float SegmentLenSq = Segment.SizeSquared();
	if (SegmentLenSq <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector HunterLoc = Hunter->GetActorLocation();
	const float T = FMath::Clamp(FVector::DotProduct(HunterLoc - VisualStart, Segment) / SegmentLenSq, 0.0f, 1.0f);
	const FVector ClosestPoint = VisualStart + Segment * T;
	FVector LaunchDir = (HunterLoc - ClosestPoint).GetSafeNormal();
	if (LaunchDir.IsNearlyZero())
	{
		LaunchDir = FVector::CrossProduct(Segment.GetSafeNormal(), FVector::UpVector).GetSafeNormal();
	}

	Hunter->LaunchCharacter(LaunchDir * LaunchSpeed + FVector(0.0f, 0.0f, 260.0f), true, true);
	UE_LOG(LogBreachborne, Log, TEXT("PowerAbility: BungeeShot cord bounced %s"), *GetNameSafe(Hunter));
}
