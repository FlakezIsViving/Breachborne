#include "BBChestActor.h"

#include "Breachborne/Breachborne.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Items/BBInventoryManager.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float ChestTickInterval = 0.1f;
}

ABBChestActor::ABBChestActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InteractZone = CreateDefaultSubobject<USphereComponent>(TEXT("InteractZone"));
	InteractZone->SetupAttachment(SceneRoot);
	InteractZone->SetSphereRadius(InteractRadius);
	InteractZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractZone->SetCollisionObjectType(ECC_WorldDynamic);
	InteractZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractZone->SetGenerateOverlapEvents(true);

	ClosedVisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ClosedVisualRoot"));
	ClosedVisualRoot->SetupAttachment(SceneRoot);

	ClosedMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ClosedMesh"));
	ClosedMesh->SetupAttachment(ClosedVisualRoot);
	ClosedMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	OpenVisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("OpenVisualRoot"));
	OpenVisualRoot->SetupAttachment(SceneRoot);

	OpenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OpenMesh"));
	OpenMesh->SetupAttachment(OpenVisualRoot);
	OpenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProgressVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProgressVisual"));
	ProgressVisual->SetupAttachment(SceneRoot);
	ProgressVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProgressVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));

	LootSpawnRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LootSpawnRoot"));
	LootSpawnRoot->SetupAttachment(SceneRoot);
	LootSpawnRoot->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		ClosedMesh->SetStaticMesh(CubeMesh.Object);
		ClosedMesh->SetRelativeScale3D(FVector(1.2f, 0.8f, 0.55f));
		OpenMesh->SetStaticMesh(CubeMesh.Object);
		OpenMesh->SetRelativeScale3D(FVector(1.25f, 0.85f, 0.25f));
		OpenMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		ProgressVisual->SetStaticMesh(CylinderMesh.Object);
	}

	UpdateVisualState();
	UpdateProgressVisual();
}

void ABBChestActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractZone)
	{
		InteractZone->SetSphereRadius(InteractRadius);
	}

	UpdateVisualState();
	UpdateProgressVisual();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(OpeningTickHandle, this, &ABBChestActor::OpeningTick, ChestTickInterval, true);
	}
}

void ABBChestActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABBChestActor, ChestState);
	DOREPLIFETIME(ABBChestActor, OpenProgress);
}

void ABBChestActor::ConfigureChest(EBBChestType InChestType, EBBChestTier InChestTier, float InOpenDuration, UBBChestLootTable* InLootTable, float InInteractRadius)
{
	if (!HasAuthority())
	{
		return;
	}

	ChestType = InChestType;
	ChestTier = InChestTier;
	OpenDuration = FMath::Max(0.1f, InOpenDuration);
	LootTable = InLootTable;
	InteractRadius = FMath::Max(50.0f, InInteractRadius);
	if (InteractZone)
	{
		InteractZone->SetSphereRadius(InteractRadius);
	}

	UpdateProgressVisual();
	ForceNetUpdate();
}

void ABBChestActor::OnRep_ChestState()
{
	UpdateVisualState();
}

void ABBChestActor::OnRep_OpenProgress()
{
	UpdateProgressVisual();
}

void ABBChestActor::OpeningTick()
{
	if (!HasAuthority() || ChestState == EBBChestState::Opened || ChestState == EBBChestState::Expired)
	{
		return;
	}

	if (ABreachbornePlayerState* ValidOpener = FindValidOpener())
	{
		ChestState = EBBChestState::Opening;
		OpeningPlayerState = ValidOpener;
		OpenProgress = FMath::Clamp(OpenProgress + (ChestTickInterval / FMath::Max(0.1f, OpenDuration)), 0.0f, 1.0f);
		if (OpenProgress >= 1.0f)
		{
			OpenChest();
			return;
		}
	}
	else
	{
		OpenProgress = 0.0f;
		ChestState = EBBChestState::Closed;
		OpeningPlayerState.Reset();
	}

	UpdateVisualState();
	UpdateProgressVisual();
	ForceNetUpdate();
}

void ABBChestActor::OpenChest()
{
	if (!HasAuthority() || ChestState == EBBChestState::Opened || ChestState == EBBChestState::Expired)
	{
		return;
	}

	ChestState = EBBChestState::Opened;
	OpenProgress = 1.0f;
	GetWorldTimerManager().ClearTimer(OpeningTickHandle);

	const FName RequiredKeyID = ResolveRequiredKeyItemID();
	if (bConsumeKeyOnOpen && !RequiredKeyID.IsNone())
	{
		if (ABreachbornePlayerState* OpenerPS = OpeningPlayerState.Get())
		{
			FBBInventoryManager::ConsumeConsumable(OpenerPS, RequiredKeyID, FMath::Max(1, RequiredKeyCount));
		}
	}

	const int32 SpawnedCount = SpawnLoot();
	UpdateVisualState();
	UpdateProgressVisual();
	ForceNetUpdate();

	GetWorldTimerManager().SetTimer(ExpireHandle, this, &ABBChestActor::ExpireOpenChest, OpenDespawnDelay, false);
	UE_LOG(LogBreachborne, Log, TEXT("Chest: %s opened and spawned %d loot pickups"), *GetName(), SpawnedCount);
}

void ABBChestActor::ExpireOpenChest()
{
	if (!HasAuthority() || ChestState == EBBChestState::Expired)
	{
		return;
	}

	ChestState = EBBChestState::Expired;
	SetActorEnableCollision(false);
	UpdateVisualState();
	ForceNetUpdate();
}

void ABBChestActor::UpdateVisualState()
{
	const bool bShowClosed = ChestState == EBBChestState::Closed || ChestState == EBBChestState::Opening;
	const bool bShowOpen = ChestState == EBBChestState::Opened;
	const bool bShowProgress = ChestState == EBBChestState::Opening && OpenProgress > 0.0f;

	if (ClosedVisualRoot)
	{
		ClosedVisualRoot->SetVisibility(bShowClosed, true);
	}
	if (OpenVisualRoot)
	{
		OpenVisualRoot->SetVisibility(bShowOpen, true);
	}
	if (ProgressVisual)
	{
		ProgressVisual->SetVisibility(bShowProgress, true);
	}
	if (InteractZone && (ChestState == EBBChestState::Opened || ChestState == EBBChestState::Expired))
	{
		InteractZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void ABBChestActor::UpdateProgressVisual()
{
	if (!ProgressVisual)
	{
		return;
	}

	const float RadiusScale = (InteractRadius / 100.0f) * FMath::Clamp(OpenProgress, 0.02f, 1.0f);
	ProgressVisual->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, 0.025f));
}

ABreachbornePlayerState* ABBChestActor::FindValidOpener() const
{
	if (!InteractZone)
	{
		return nullptr;
	}

	const FName RequiredKeyID = ResolveRequiredKeyItemID();
	const int32 KeyCount = FMath::Max(1, RequiredKeyCount);

	TArray<AActor*> OverlappingActors;
	InteractZone->GetOverlappingActors(OverlappingActors, AHunterCharacter::StaticClass());
	for (AActor* Actor : OverlappingActors)
	{
		const AHunterCharacter* Hunter = Cast<AHunterCharacter>(Actor);
		ABreachbornePlayerState* PS = Hunter ? Hunter->GetPlayerState<ABreachbornePlayerState>() : nullptr;
		if (!PS || !PS->GetIsAlive())
		{
			continue;
		}

		if (!RequiredKeyID.IsNone() && !FBBInventoryManager::HasConsumable(PS, RequiredKeyID, KeyCount))
		{
			continue;
		}

		return PS;
	}

	return nullptr;
}

FName ABBChestActor::ResolveRequiredKeyItemID() const
{
	switch (ChestType)
	{
	case EBBChestType::YellowKey:
		return FName(TEXT("YellowKey"));
	case EBBChestType::RedKey:
		return FName(TEXT("RedKey"));
	case EBBChestType::CustomKey:
		return RequiredKeyItemID;
	default:
		return NAME_None;
	}
}

bool ABBChestActor::DoesChestRequireKey() const
{
	return !ResolveRequiredKeyItemID().IsNone();
}

int32 ABBChestActor::SpawnLoot()
{
	const FVector SpawnOrigin = LootSpawnRoot ? LootSpawnRoot->GetComponentLocation() : GetActorLocation();
	return LootTable
		? LootTable->SpawnLoot(GetWorld(), SpawnOrigin, ChestTier)
		: UBBChestLootTable::SpawnFallbackLoot(GetWorld(), SpawnOrigin, ChestTier);
}
