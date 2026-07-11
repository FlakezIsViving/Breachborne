#include "BBAllyBot.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Breachborne/Breachborne.h"

ABBAllyBot::ABBAllyBot()
{
	PrimaryActorTick.bCanEverTick = false;

	// Capsule must overlap on ECC_Pawn so wisp OverlapMultiByChannel detects us
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (Capsule)
	{
		Capsule->SetCollisionObjectType(ECC_Pawn);
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Capsule->SetGenerateOverlapEvents(true);
	}

	// Green visual mesh attached to the capsule
	BotMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BotMesh"));
	BotMesh->SetupAttachment(GetCapsuleComponent());
	BotMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BotMesh->SetRelativeLocation(FVector(0, 0, -90.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		BotMesh->SetStaticMesh(CylinderMesh.Object);
		BotMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	}

	// Rez range circle — flat cylinder on the ground
	RangeCircleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RangeCircleMesh"));
	RangeCircleMesh->SetupAttachment(GetCapsuleComponent());
	RangeCircleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Engine Cylinder: radius 50, height 100. Scale to RezRange radius and nearly flat.
	const float CircleScale = RezRange / 50.0f; // 400/50 = 8.0
	RangeCircleMesh->SetRelativeScale3D(FVector(CircleScale, CircleScale, 0.01f));
	RangeCircleMesh->SetRelativeLocation(FVector(0, 0, -88.0f)); // ground level
	if (CylinderMesh.Succeeded())
	{
		RangeCircleMesh->SetStaticMesh(CylinderMesh.Object);
	}

	// Networking
	bReplicates = true;
	SetReplicatingMovement(true);

	// Don't move — just stand there
	GetCharacterMovement()->DisableMovement();
}

void ABBAllyBot::BeginPlay()
{
	Super::BeginPlay();

	// Apply green dynamic material to bot body
	if (BotMesh)
	{
		UMaterialInstanceDynamic* GreenMat = BotMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (GreenMat)
		{
			GreenMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 0.8f, 0.1f));
		}
	}

	// Apply green dynamic material to range circle
	if (RangeCircleMesh)
	{
		UMaterialInstanceDynamic* CircleMat = RangeCircleMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (CircleMat)
		{
			CircleMat->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(0.0f, 1.0f, 0.2f));
		}
	}

	if (HasAuthority())
	{
		UE_LOG(LogBreachborne, Warning, TEXT(""));
		UE_LOG(LogBreachborne, Warning, TEXT("========================================"));
		UE_LOG(LogBreachborne, Warning, TEXT("  ALLY BOT [%s] SPAWNED"), *GetName());
		UE_LOG(LogBreachborne, Warning, TEXT("  Location: %s"), *GetActorLocation().ToString());
		UE_LOG(LogBreachborne, Warning, TEXT("  Team: %d"), TeamID);
		UE_LOG(LogBreachborne, Warning, TEXT("========================================"));
		UE_LOG(LogBreachborne, Warning, TEXT(""));
	}
}
