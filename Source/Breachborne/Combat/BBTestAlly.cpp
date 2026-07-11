#include "BBTestAlly.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Wisp/BBWispPawn.h"
#include "Breachborne/Breachborne.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"

ABBTestAlly::ABBTestAlly()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// Collision capsule — ECC_Pawn so wisp proximity and Q heal sphere detect us
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(42.0f, 96.0f);
	CapsuleComp->SetCollisionObjectType(ECC_Pawn);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Block);
	CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CapsuleComp->SetGenerateOverlapEvents(true);
	SetRootComponent(CapsuleComp);

	// Visible mesh — cyan cylinder so it's easy to spot (distinct from green ally bot)
	AllyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AllyMesh"));
	AllyMesh->SetupAttachment(CapsuleComp);
	AllyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AllyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		AllyMesh->SetStaticMesh(CylinderMesh.Object);
		AllyMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	}

	// GAS — local ASC + HealthSet (no PlayerState needed)
	AbilitySystemComponent = CreateDefaultSubobject<UBBAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	HealthSet = CreateDefaultSubobject<UBBHealthSet>(TEXT("HealthSet"));
}

void ABBTestAlly::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	InitAbilitySystem();

	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT("========================================"));
	UE_LOG(LogBreachborne, Warning, TEXT("  TEST ALLY [%s] SPAWNED"), *GetName());
	UE_LOG(LogBreachborne, Warning, TEXT("  Location: %s"), *GetActorLocation().ToString());
	UE_LOG(LogBreachborne, Warning, TEXT("  Team: %d | Health: %.0f / %.0f | Decay: %.1f/s (enabled=%s) | Life: %.1fs"),
		TeamID, GetCurrentHealth(), MaxHealth, HealthDecayRate, bHealthDecayEnabled ? TEXT("YES") : TEXT("NO"), LifeSpan);
	UE_LOG(LogBreachborne, Warning, TEXT("========================================"));
	UE_LOG(LogBreachborne, Warning, TEXT(""));
}

void ABBTestAlly::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || bDead)
	{
		return;
	}

	// Health decay
	if (bHealthDecayEnabled)
	{
		ApplyHealthDecay(DeltaTime);
	}

	// Life span timer (only counts down while alive)
	ElapsedLife += DeltaTime;
	if (ElapsedLife >= LifeSpan)
	{
		EnterWispForm();
	}
}

UAbilitySystemComponent* ABBTestAlly::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float ABBTestAlly::GetCurrentHealth() const
{
	if (HealthSet)
	{
		return HealthSet->GetHealth();
	}
	return 0.0f;
}

void ABBTestAlly::Kill()
{
	if (bDead)
	{
		return;
	}
	EnterWispForm();
}

void ABBTestAlly::Revive()
{
	if (!bDead)
	{
		return;
	}

	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bDead = false;

	// Teleport to the wisp's current location before destroying it
	// (wisp may have been carried by Eluna or moved since death)
	if (SpawnedWisp.IsValid())
	{
		SetActorLocation(SpawnedWisp->GetActorLocation());
	}

	// Destroy the wisp if it's still around — disable collision first so the player doesn't get stuck
	if (SpawnedWisp.IsValid() && !SpawnedWisp->IsPendingKillPending())
	{
		SpawnedWisp->SetActorEnableCollision(false);
		SpawnedWisp->Destroy();
	}
	SpawnedWisp = nullptr;

	// Restore visibility and collision
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	// Reset health and timers
	ElapsedLife = 0.0f;
	if (HealthSet)
	{
		HealthSet->SetHealth(MaxHealth);
	}

	UE_LOG(LogBreachborne, Warning, TEXT("TEST ALLY [%s] REVIVED at %.2f | Location=%s | Health=%.0f"),
		*GetName(), WorldTime, *GetActorLocation().ToString(), MaxHealth);
}

void ABBTestAlly::InitAbilitySystem()
{
	if (!AbilitySystemComponent || !HealthSet)
	{
		return;
	}

	// Both owner and avatar are this actor
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// Initialize health attributes
	HealthSet->SetMaxHealth(MaxHealth);
	HealthSet->SetHealth(MaxHealth);
	HealthSet->SetMaxShield(0.0f);
	HealthSet->SetShield(0.0f);
	HealthSet->SetArmor(0.0f);

	// Bind death callback
	HealthSet->OnHealthDepleted.AddDynamic(this, &ABBTestAlly::OnHealthDepleted);
}

void ABBTestAlly::ApplyHealthDecay(float DeltaTime)
{
	if (!HealthSet || HealthDecayRate <= 0.0f)
	{
		return;
	}

	const float DecayAmount = HealthDecayRate * DeltaTime;
	const float CurrentHP = HealthSet->GetHealth();
	const float NewHP = FMath::Max(CurrentHP - DecayAmount, 0.0f);
	HealthSet->SetHealth(NewHP);

	if (NewHP <= 0.0f)
	{
		EnterWispForm();
	}
}

void ABBTestAlly::EnterWispForm()
{
	if (bDead)
	{
		return;
	}
	bDead = true;

	UE_LOG(LogBreachborne, Warning, TEXT("TEST ALLY [%s] DIED after %.2fs — entering wisp form"), *GetName(), ElapsedLife);

	// Hide self and disable collision (actor stays alive so we can revive later)
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	// Spawn wisp
	ABBWispPawn* Wisp = SpawnWispForNearestPlayer();
	if (Wisp)
	{
		SpawnedWisp = Wisp;
		UE_LOG(LogBreachborne, Warning, TEXT("TEST ALLY: spawned wisp %s"), *Wisp->GetName());

		// Bind to wisp events so we auto-revive when rez bar fills
		Wisp->OnWispReviveReady.AddDynamic(this, &ABBTestAlly::OnWispReadyToRevive);
		Wisp->OnWispDied.AddDynamic(this, &ABBTestAlly::OnWispDied);
	}
	else
	{
		UE_LOG(LogBreachborne, Warning, TEXT("TEST ALLY: no player found to own wisp — wisp not spawned"));
	}
}

ABBWispPawn* ABBTestAlly::SpawnWispForNearestPlayer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Find nearest hunter with a valid PlayerState
	AHunterCharacter* NearestHunter = nullptr;
	float BestDistSq = MAX_FLT;

	for (TActorIterator<AHunterCharacter> It(World); It; ++It)
	{
		AHunterCharacter* Hunter = *It;
		if (!Hunter)
		{
			continue;
		}

		ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>();
		if (!PS)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Hunter->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			NearestHunter = Hunter;
		}
	}

	if (!NearestHunter)
	{
		return nullptr;
	}

	ABreachbornePlayerState* OwnerPS = NearestHunter->GetPlayerState<ABreachbornePlayerState>();
	if (!OwnerPS)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;

	ABBWispPawn* Wisp = World->SpawnActor<ABBWispPawn>(
		ABBWispPawn::StaticClass(),
		GetActorLocation(),
		FRotator::ZeroRotator,
		Params);

	if (Wisp)
	{
		Wisp->InitWisp(OwnerPS);
	}

	return Wisp;
}

void ABBTestAlly::OnHealthDepleted(UAbilitySystemComponent* VictimASC, UAbilitySystemComponent* KillerASC)
{
	UE_LOG(LogBreachborne, Warning, TEXT("TEST ALLY [%s] OnHealthDepleted called"), *GetName());
	EnterWispForm();
}

void ABBTestAlly::OnWispReadyToRevive(ABBWispPawn* Wisp)
{
	if (!Wisp || Wisp != SpawnedWisp.Get())
	{
		return;
	}

	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	UE_LOG(LogBreachborne, Warning, TEXT("TEST ALLY [%s] AUTO-REVIVE triggered at %.2f (wisp rez bar full)"),
		*GetName(), WorldTime);
	Revive();
}

void ABBTestAlly::OnWispDied(ABBWispPawn* Wisp, bool bWasExecuted)
{
	if (!Wisp || Wisp != SpawnedWisp.Get())
	{
		return;
	}

	UE_LOG(LogBreachborne, Warning, TEXT("TEST ALLY [%s] wisp died (executed=%s) — ally remains dead until manual revive"),
		*GetName(), bWasExecuted ? TEXT("YES") : TEXT("NO"));

	SpawnedWisp = nullptr;
}
