#include "BBSupplyDropActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABBSupplyDropActor::ABBSupplyDropActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DropVisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DropVisualRoot"));
	DropVisualRoot->SetupAttachment(SceneRoot);

	DropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DropMesh"));
	DropMesh->SetupAttachment(DropVisualRoot);
	DropMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LandingIndicator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LandingIndicator"));
	LandingIndicator->SetupAttachment(SceneRoot);
	LandingIndicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LandingIndicator->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));

	ChestSpawnRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ChestSpawnRoot"));
	ChestSpawnRoot->SetupAttachment(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		DropMesh->SetStaticMesh(CubeMesh.Object);
		DropMesh->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.8f));
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		LandingIndicator->SetStaticMesh(CylinderMesh.Object);
	}
}

void ABBSupplyDropActor::BeginPlay()
{
	Super::BeginPlay();

	if (!ChestActorClass)
	{
		ChestActorClass = ABBChestActor::StaticClass();
	}

	UpdateDropVisual();
}

void ABBSupplyDropActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bHasLanded)
	{
		UpdateDropVisual();
		return;
	}

	ReplicatedDropElapsed = FMath::Min(ReplicatedDropElapsed + DeltaSeconds, FallDuration);

	if (HasAuthority() && ReplicatedDropElapsed >= FallDuration)
	{
		LandDrop();
		return;
	}

	UpdateDropVisual();
}

void ABBSupplyDropActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABBSupplyDropActor, bHasLanded);
	DOREPLIFETIME(ABBSupplyDropActor, ReplicatedDropElapsed);
	DOREPLIFETIME(ABBSupplyDropActor, SpawnedChest);
}

float ABBSupplyDropActor::GetDropProgress() const
{
	return FMath::Clamp(ReplicatedDropElapsed / FMath::Max(0.1f, FallDuration), 0.0f, 1.0f);
}

void ABBSupplyDropActor::OnRep_DropState()
{
	UpdateDropVisual();
}

void ABBSupplyDropActor::LandDrop()
{
	if (!HasAuthority() || bHasLanded)
	{
		return;
	}

	bHasLanded = true;
	ReplicatedDropElapsed = FallDuration;
	SpawnSupplyChest();
	UpdateDropVisual();
	ForceNetUpdate();

	if (DespawnDelayAfterLanding > 0.0f)
	{
		GetWorldTimerManager().SetTimer(DespawnHandle, [WeakThis = TWeakObjectPtr<ABBSupplyDropActor>(this)]()
		{
			if (ABBSupplyDropActor* DropActor = WeakThis.Get())
			{
				DropActor->Destroy();
			}
		}, DespawnDelayAfterLanding, false);
	}
}

void ABBSupplyDropActor::SpawnSupplyChest()
{
	if (!HasAuthority() || SpawnedChest)
	{
		return;
	}

	const FVector SpawnLocation = ChestSpawnRoot ? ChestSpawnRoot->GetComponentLocation() : GetActorLocation();
	const FRotator SpawnRotation = GetActorRotation();
	UWorld* World = GetWorld();
	if (!World || !ChestActorClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ABBChestActor* Chest = World->SpawnActorDeferred<ABBChestActor>(ChestActorClass, FTransform(SpawnRotation, SpawnLocation), this, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Chest)
	{
		return;
	}

	Chest->ConfigureChest(EBBChestType::SupplyDrop, ChestTier, ChestOpenDuration, LootTable, ChestInteractRadius);
	Chest->FinishSpawning(FTransform(SpawnRotation, SpawnLocation));
	SpawnedChest = Chest;
}

void ABBSupplyDropActor::UpdateDropVisual()
{
	const float Progress = GetDropProgress();
	const float CurrentHeight = FMath::Lerp(DropHeight, 0.0f, Progress);

	if (DropVisualRoot)
	{
		DropVisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, CurrentHeight));
		DropVisualRoot->SetVisibility(!bHasLanded, true);
	}

	if (LandingIndicator)
	{
		const float RadiusScale = LandingIndicatorRadius / 100.0f;
		LandingIndicator->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, 0.02f));
		LandingIndicator->SetVisibility(!bHasLanded, true);
	}
}
