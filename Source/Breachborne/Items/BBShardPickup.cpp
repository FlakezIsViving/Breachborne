#include "BBShardPickup.h"
#include "BBInventoryManager.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Breachborne.h"

ABBShardPickup::ABBShardPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	bReplicates = true;
	SetReplicatingMovement(true);

	// Magnet sphere — large radius for attraction trigger
	// Overlap events DISABLED until InitShards() is called — prevents instant collection during spawn
	MagnetSphere = CreateDefaultSubobject<USphereComponent>(TEXT("MagnetSphere"));
	MagnetSphere->InitSphereRadius(MagnetRadius);
	MagnetSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MagnetSphere->SetCollisionObjectType(ECC_WorldDynamic);
	MagnetSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	MagnetSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MagnetSphere->SetGenerateOverlapEvents(false);
	SetRootComponent(MagnetSphere);

	// Collection sphere — small radius for walk-over pickup
	// Also disabled until InitShards()
	CollectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollectionSphere"));
	CollectionSphere->InitSphereRadius(CollectionRadius);
	CollectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollectionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollectionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollectionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollectionSphere->SetGenerateOverlapEvents(false);
	CollectionSphere->SetupAttachment(MagnetSphere);

	// Visible mesh — red sphere
	ShardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShardMesh"));
	ShardMesh->SetupAttachment(MagnetSphere);
	ShardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		ShardMesh->SetStaticMesh(SphereMesh.Object);
	}

	// Load the engine's parameterized material so we can tint it red at BeginPlay
	static ConstructorHelpers::FObjectFinder<UMaterial> BaseMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (BaseMat.Succeeded())
	{
		ShardMesh->SetMaterial(0, BaseMat.Object);
	}
}

void ABBShardPickup::BeginPlay()
{
	Super::BeginPlay();

	// Create red dynamic material instance from the BasicShapeMaterial
	if (ShardMesh)
	{
		RedMaterial = ShardMesh->CreateDynamicMaterialInstance(0);
		if (RedMaterial)
		{
			// BasicShapeMaterial exposes "Color" parameter (not "BaseColor")
			RedMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.1f, 0.1f, 1.0f));
		}
	}

	UE_LOG(LogBreachborne, Warning, TEXT("ShardPickup BeginPlay: %s | Auth=%s | Location=%s | Hidden=%s | Collision=%s | ShardAmount=%d"),
		*GetName(),
		HasAuthority() ? TEXT("SERVER") : TEXT("CLIENT"),
		*GetActorLocation().ToString(),
		IsHidden() ? TEXT("YES") : TEXT("NO"),
		GetActorEnableCollision() ? TEXT("YES") : TEXT("NO"),
		ShardAmount);

	// Bind overlap events (server only for gameplay logic)
	if (HasAuthority())
	{
		MagnetSphere->OnComponentBeginOverlap.AddDynamic(this, &ABBShardPickup::OnMagnetOverlap);
		CollectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABBShardPickup::OnCollectionOverlap);
	}
}

void ABBShardPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBShardPickup, ShardAmount);
	DOREPLIFETIME(ABBShardPickup, PickupState);
	DOREPLIFETIME(ABBShardPickup, SourcePlayerState);
}

void ABBShardPickup::InitShards(int32 Amount, APlayerState* Source)
{
	if (!HasAuthority())
	{
		return;
	}

	ShardAmount = Amount;
	SourcePlayerState = Source;

	// Scale mesh: 1 shard = BaseVisualScale, each additional adds ScalePerShard
	const float FinalScale = BaseVisualScale + ScalePerShard * FMath::Max(0, Amount - 1);
	if (ShardMesh)
	{
		ShardMesh->SetRelativeScale3D(FVector(FinalScale));
	}

	// Start despawn timer
	GetWorldTimerManager().SetTimer(
		DespawnTimerHandle,
		this,
		&ABBShardPickup::OnDespawnTimer,
		DespawnTime,
		false
	);

	// Start magnet activation delay
	GetWorldTimerManager().SetTimer(
		MagnetActivationTimerHandle,
		this,
		&ABBShardPickup::ActivateMagnet,
		MagnetActivationDelay,
		false
	);

	// NOW enable collision — after ShardAmount is set, so overlaps won't fire with default value.
	// Collection sphere activates immediately; magnet sphere waits for ActivateMagnet timer.
	CollectionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollectionSphere->SetGenerateOverlapEvents(true);

	// Update overlaps so any pawns already standing here trigger collection
	CollectionSphere->UpdateOverlaps();

	UE_LOG(LogBreachborne, Log, TEXT("ShardPickup: Spawned %d shards at %s (scale %.2f)"), Amount, *GetActorLocation().ToString(), FinalScale);
}

void ABBShardPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || PickupState != EShardPickupState::Magnetized)
	{
		return;
	}

	// Validate magnet target is still alive
	if (!MagnetTarget || MagnetTarget->IsPendingKillPending())
	{
		// Lost target — go back to idle
		MagnetTarget = nullptr;
		PickupState = EShardPickupState::Idle;
		SetActorTickEnabled(false);
		return;
	}

	// Check if target is still alive via PlayerState
	ABreachbornePlayerState* PS = MagnetTarget->GetPlayerState<ABreachbornePlayerState>();
	if (!PS || !PS->GetIsAlive())
	{
		MagnetTarget = nullptr;
		PickupState = EShardPickupState::Idle;
		SetActorTickEnabled(false);
		return;
	}

	// Move toward target
	const FVector TargetLoc = MagnetTarget->GetActorLocation();
	const FVector CurrentLoc = GetActorLocation();
	const FVector ToTarget = TargetLoc - CurrentLoc;
	const float Distance = ToTarget.Size();

	// Collect when close enough
	if (Distance < CollectionRadius)
	{
		ServerCollectShard(MagnetTarget);
		return;
	}

	// Lerp toward target with acceleration
	const FVector Direction = ToTarget.GetSafeNormal();
	const float MoveStep = MagnetSpeed * DeltaTime;
	const FVector NewLocation = CurrentLoc + Direction * FMath::Min(MoveStep, Distance);
	SetActorLocation(NewLocation);
}

void ABBShardPickup::OnMagnetOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || PickupState != EShardPickupState::Idle || !bMagnetActive)
	{
		return;
	}

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(OtherActor);
	if (!Hunter)
	{
		return;
	}

	// Check alive (both PlayerState flag and GAS tag)
	ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>();
	if (!PS || !PS->GetIsAlive())
	{
		return;
	}
	UAbilitySystemComponent* ASC = Hunter->GetAbilitySystemComponent();
	if (ASC && ASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
	{
		return;
	}

	// Don't auto-magnetize toward the player who dropped these shards (requires E-key)
	if (IsOwnShard(PS))
	{
		return;
	}

	// Start magnetizing toward this hunter
	MagnetTarget = Hunter;
	PickupState = EShardPickupState::Magnetized;
	SetActorTickEnabled(true);
}

void ABBShardPickup::OnCollectionOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || PickupState == EShardPickupState::PickedUp)
	{
		return;
	}

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(OtherActor);
	if (!Hunter)
	{
		return;
	}

	// Don't auto-collect own shards (requires E-key)
	ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>();
	if (IsOwnShard(PS))
	{
		return;
	}

	UE_LOG(LogBreachborne, Warning, TEXT("ShardPickup: CollectionOverlap on %s by %s (ShardAmount=%d)"),
		*GetName(), *Hunter->GetName(), ShardAmount);

	ServerCollectShard(Hunter);
}

void ABBShardPickup::ServerCollectShard(AHunterCharacter* Hunter)
{
	if (!Hunter || PickupState == EShardPickupState::PickedUp)
	{
		return;
	}

	ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>();
	if (!PS || !PS->GetIsAlive())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("ShardPickup: REJECTED collection by %s (Alive=%s, PS=%s)"),
			*Hunter->GetName(),
			PS ? (PS->GetIsAlive() ? TEXT("YES") : TEXT("NO")) : TEXT("N/A"),
			PS ? TEXT("valid") : TEXT("null"));
		return;
	}

	// Also reject if the hunter has the State_Dead tag (set by HealthSet before PS->bIsAlive updates)
	UAbilitySystemComponent* ASC = Hunter->GetAbilitySystemComponent();
	if (ASC && ASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
	{
		UE_LOG(LogBreachborne, Warning, TEXT("ShardPickup: REJECTED collection by %s (has State_Dead tag)"),
			*Hunter->GetName());
		return;
	}

	// Add shards to inventory
	FBBInventoryManager::AddShards(PS, ShardAmount);

	UE_LOG(LogBreachborne, Warning, TEXT("ShardPickup: %s collected %d shards (from %s)"), *PS->GetPlayerName(), ShardAmount, *GetName());

	// Mark as picked up (replicates to clients)
	PickupState = EShardPickupState::PickedUp;
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	// Destroy after a short delay to let replication propagate
	SetLifeSpan(1.0f);
}

void ABBShardPickup::ActivateMagnet()
{
	bMagnetActive = true;

	// Enable magnet sphere collision now that the delay has passed
	MagnetSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MagnetSphere->SetGenerateOverlapEvents(true);

	// Check if any hunters are already inside the magnet sphere
	CheckExistingMagnetOverlaps();
}

void ABBShardPickup::CheckExistingMagnetOverlaps()
{
	if (!HasAuthority() || PickupState != EShardPickupState::Idle)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	MagnetSphere->GetOverlappingActors(OverlappingActors, AHunterCharacter::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		AHunterCharacter* Hunter = Cast<AHunterCharacter>(Actor);
		if (!Hunter)
		{
			continue;
		}

		ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>();
		UAbilitySystemComponent* HunterASC = Hunter->GetAbilitySystemComponent();
		if (PS && PS->GetIsAlive() && !IsOwnShard(PS) && !(HunterASC && HunterASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead)))
		{
			MagnetTarget = Hunter;
			PickupState = EShardPickupState::Magnetized;
			SetActorTickEnabled(true);
			return; // Magnetize to the first valid hunter found
		}
	}
}

bool ABBShardPickup::IsOwnShard(const APlayerState* Checker) const
{
	return SourcePlayerState != nullptr && Checker == SourcePlayerState;
}

void ABBShardPickup::ServerRequestPickup_Implementation(APlayerController* RequestingPlayer)
{
	if (!HasAuthority() || !RequestingPlayer || PickupState == EShardPickupState::PickedUp)
	{
		return;
	}

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(RequestingPlayer->GetPawn());
	if (!Hunter)
	{
		return;
	}

	// Validate proximity
	const float DistSq = FVector::DistSquared(Hunter->GetActorLocation(), GetActorLocation());
	const float MaxPickupRange = 300.0f;
	if (DistSq > MaxPickupRange * MaxPickupRange)
	{
		return;
	}

	ServerCollectShard(Hunter);
}

void ABBShardPickup::OnDespawnTimer()
{
	if (PickupState != EShardPickupState::PickedUp)
	{
		UE_LOG(LogBreachborne, Log, TEXT("ShardPickup: Despawning uncollected %d shards at %s"), ShardAmount, *GetActorLocation().ToString());
		Destroy();
	}
}

void ABBShardPickup::OnRep_PickupState()
{
	UE_LOG(LogBreachborne, Warning, TEXT("ShardPickup OnRep_PickupState: %s | State=%d | Location=%s"),
		*GetName(), (int32)PickupState, *GetActorLocation().ToString());

	if (PickupState == EShardPickupState::PickedUp)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}
}
