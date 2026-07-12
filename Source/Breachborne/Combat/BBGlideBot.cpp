#include "BBGlideBot.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/GliderComponent.h"
#include "Breachborne/Characters/BBCharacterMovementComponent.h"
#include "Breachborne/Breachborne.h"
#include "UObject/ConstructorHelpers.h"

ABBGlideBot::ABBGlideBot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UBBCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicatingMovement(true);

	// Capsule collision (inherited from ACharacter) — ECC_Pawn so projectiles detect it
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

	// Visible mesh — sphere so it's easy to distinguish from target dummies
	BotMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BotMesh"));
	BotMesh->SetupAttachment(GetCapsuleComponent());
	BotMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		BotMesh->SetStaticMesh(SphereMesh.Object);
		BotMesh->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));
	}

	// GAS — own ASC + HealthSet (actor-owned, no PlayerState)
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	HealthSet = CreateDefaultSubobject<UBBHealthSet>(TEXT("HealthSet"));

	// Glider component — manages glide state, heat, spiking
	GliderComponent = CreateDefaultSubobject<UGliderComponent>(TEXT("GliderComponent"));

	// CMC config — bot faces waypoint direction, not a cursor
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->bOrientRotationToMovement = false;
		CMC->bUseControllerDesiredRotation = false;
	}
}

void ABBGlideBot::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(HasAuthority());

	if (!HasAuthority())
	{
		return;
	}

	// Init ASC — both owner and avatar are this actor
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	// Save spawn location for respawning
	SpawnLocation = GetActorLocation();

	// Convert relative waypoints to world space
	WorldWaypointA = GetActorTransform().TransformPosition(WaypointA);
	WorldWaypointB = GetActorTransform().TransformPosition(WaypointB);

	// Bind death callback
	if (HealthSet)
	{
		HealthSet->OnHealthDepleted.AddDynamic(this, &ABBGlideBot::OnHealthDepleted);
	}

	// Bind spike recovery callback
	if (GliderComponent)
	{
		GliderComponent->OnSpikeRecovered.AddLambda([this]() { OnSpikeRecovered(); });
	}

	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT("========================================"));
	UE_LOG(LogBreachborne, Warning, TEXT("  GLIDE BOT [%s] SPAWNED"), *GetName());
	UE_LOG(LogBreachborne, Warning, TEXT("  Location: %s"), *SpawnLocation.ToString());
	UE_LOG(LogBreachborne, Warning, TEXT("  Waypoint A: %s"), *WorldWaypointA.ToString());
	UE_LOG(LogBreachborne, Warning, TEXT("  Waypoint B: %s"), *WorldWaypointB.ToString());
	UE_LOG(LogBreachborne, Warning, TEXT("  PatrolSpeed: %.0f  |  Team: %d"), PatrolSpeed, TeamID);
	UE_LOG(LogBreachborne, Warning, TEXT("  AutoRespawn: %s (%.1fs delay)"), bAutoRespawn ? TEXT("YES") : TEXT("NO"), RespawnDelay);
	UE_LOG(LogBreachborne, Warning, TEXT("========================================"));
	UE_LOG(LogBreachborne, Warning, TEXT(""));

	// Start patrol
	LaunchAndGlide();
}

void ABBGlideBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	if (!GliderComponent)
	{
		return;
	}

	const EGliderState GlideState = GliderComponent->GetGliderState();
	const UCharacterMovementComponent* CMC = GetCharacterMovement();

	if (GlideState == EGliderState::Open)
	{
		// Actively patrolling while gliding
		UpdatePatrol(DeltaTime);
	}
	else if (GlideState == EGliderState::Closed && CMC)
	{
		if (CMC->IsFalling())
		{
			// Post-spike recovery while still airborne — reopen glider
			GliderComponent->ForceOpenGlider();
		}
		else if (CMC->IsMovingOnGround())
		{
			// Landed after a spike — relaunch
			LaunchAndGlide();
		}
	}
	// Spiked state: do nothing, GliderComponent handles recovery
}

UAbilitySystemComponent* ABBGlideBot::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABBGlideBot::LaunchAndGlide()
{
	// Launch upward
	LaunchCharacter(FVector(0.0f, 0.0f, 500.0f), false, true);

	// Defer glider open — needs a tick to enter Falling mode first
	GetWorldTimerManager().SetTimer(
		GlideOpenTimerHandle,
		this,
		&ABBGlideBot::DeferredOpenGlider,
		0.15f,
		false
	);
}

void ABBGlideBot::DeferredOpenGlider()
{
	if (bIsDead || !GliderComponent)
	{
		return;
	}

	GliderComponent->ForceOpenGlider();
}

void ABBGlideBot::UpdatePatrol(float DeltaTime)
{
	const FVector CurrentTarget = bHeadingToB ? WorldWaypointB : WorldWaypointA;
	const FVector CurrentLoc = GetActorLocation();

	// Direction to target (2D — altitude managed by gliding physics)
	FVector ToTarget = CurrentTarget - CurrentLoc;
	ToTarget.Z = 0.0f;

	const float DistToTarget = ToTarget.Size();

	if (DistToTarget < 100.0f)
	{
		// Reached waypoint — switch to the other one
		bHeadingToB = !bHeadingToB;
		return;
	}

	// Face the waypoint — this is what PhysGliding uses for movement direction
	const FVector Dir = ToTarget.GetSafeNormal();
	const FRotator DesiredRot = Dir.Rotation();
	SetActorRotation(FRotator(0.0f, DesiredRot.Yaw, 0.0f));
}

void ABBGlideBot::OnHealthDepleted(UAbilitySystemComponent* VictimASC, UAbilitySystemComponent* KillerASC)
{
	bIsDead = true;

	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT(">>> GLIDE BOT [%s] KILLED! <<<"), *GetName());
	UE_LOG(LogBreachborne, Warning, TEXT("    Killer ASC owner: %s"),
		KillerASC && KillerASC->GetOwner() ? *KillerASC->GetOwner()->GetName() : TEXT("Unknown"));

	// Clear any pending timers
	GetWorldTimerManager().ClearTimer(GlideOpenTimerHandle);

	// Hide and disable
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->DisableMovement();
	}

	if (bAutoRespawn)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("    Will respawn in %.1fs"), RespawnDelay);
		UE_LOG(LogBreachborne, Warning, TEXT(""));

		GetWorldTimerManager().SetTimer(
			RespawnTimerHandle,
			this,
			&ABBGlideBot::Respawn,
			RespawnDelay,
			false
		);
	}
}

void ABBGlideBot::OnSpikeRecovered()
{
	if (bIsDead)
	{
		return;
	}

	// If still airborne after spike recovery, immediately reopen glider
	const UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (CMC && CMC->IsFalling())
	{
		GliderComponent->ForceOpenGlider();
	}
}

void ABBGlideBot::Respawn()
{
	if (!HealthSet || !AbilitySystemComponent)
	{
		return;
	}

	// Reset health
	HealthSet->SetHealth(HealthSet->GetMaxHealth());
	HealthSet->SetShield(HealthSet->GetMaxShield());

	// Remove dead tag
	AbilitySystemComponent->RemoveLooseGameplayTag(BBGameplayTags::State_Dead);

	// Teleport to spawn
	SetActorLocation(SpawnLocation);

	// Unhide and re-enable
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->SetMovementMode(MOVE_Walking);
	}

	bIsDead = false;
	bHeadingToB = true;

	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT(">>> GLIDE BOT [%s] RESPAWNED at %s <<<"), *GetName(), *SpawnLocation.ToString());
	UE_LOG(LogBreachborne, Warning, TEXT(""));

	// Resume patrol
	LaunchAndGlide();
}
