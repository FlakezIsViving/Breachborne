#include "BBTargetDummy.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Combat/BBVoidSwappableComponent.h"
#include "UObject/ConstructorHelpers.h"

ABBTargetDummy::ABBTargetDummy()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	// Collision capsule — ECC_Pawn so projectiles, grenades, and napalm zones detect it
	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComp"));
	CapsuleComp->InitCapsuleSize(42.0f, 96.0f);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CapsuleComp->SetCollisionObjectType(ECC_Pawn);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Block);
	CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CapsuleComp->SetGenerateOverlapEvents(true);
	SetRootComponent(CapsuleComp);

	// Visible mesh — tall cube so it's easy to spot in the level
	DummyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMesh"));
	DummyMesh->SetupAttachment(CapsuleComp);
	DummyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		DummyMesh->SetStaticMesh(CubeMesh.Object);
		DummyMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
	}

	// GAS — own ASC + HealthSet (ASC lives directly on this actor, not on a PlayerState)
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	HealthSet = CreateDefaultSubobject<UBBHealthSet>(TEXT("HealthSet"));
	VoidSwappableComponent = CreateDefaultSubobject<UBBVoidSwappableComponent>(TEXT("VoidSwappableComponent"));
}

void ABBTargetDummy::BeginPlay()
{
	Super::BeginPlay();

	if (!GetRootComponent())
	{
		UE_LOG(LogBreachborne, Error,
			TEXT("TargetDummy: discarding malformed placed actor without a root component: %s"),
			*GetPathName());
		if (HasAuthority())
		{
			SetReplicates(false);
			Destroy();
		}
		return;
	}

	if (AbilitySystemComponent)
	{
		// Both owner and avatar are this actor (no PlayerState for dummies)
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		UE_LOG(LogBreachborne, Warning, TEXT(""));
		UE_LOG(LogBreachborne, Warning, TEXT("========================================"));
		UE_LOG(LogBreachborne, Warning, TEXT("  TARGET DUMMY [%s] SPAWNED"), *GetName());
		UE_LOG(LogBreachborne, Warning, TEXT("  Location: %s"), *GetActorLocation().ToString());
		UE_LOG(LogBreachborne, Warning, TEXT("  Team: %d"), TeamID);
		UE_LOG(LogBreachborne, Warning, TEXT("  Health: %.0f / %.0f"), HealthSet ? HealthSet->GetHealth() : -1.0f, HealthSet ? HealthSet->GetMaxHealth() : -1.0f);
		UE_LOG(LogBreachborne, Warning, TEXT("  AutoReset: %s (%.1fs delay)"), bAutoResetHealth ? TEXT("YES") : TEXT("NO"), HealthResetDelay);
		UE_LOG(LogBreachborne, Warning, TEXT("  VoidSwappable: %s (replicates=%d movement=%d)"),
			VoidSwappableComponent ? TEXT("YES") : TEXT("NO"),
			GetIsReplicated() ? 1 : 0,
			IsReplicatingMovement() ? 1 : 0);
		UE_LOG(LogBreachborne, Warning, TEXT("========================================"));
		UE_LOG(LogBreachborne, Warning, TEXT(""));
	}

	// Bind to health depletion for auto-reset
	if (HealthSet)
	{
		HealthSet->OnHealthDepleted.AddDynamic(this, &ABBTargetDummy::OnHealthDepleted);
	}
}

UAbilitySystemComponent* ABBTargetDummy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABBTargetDummy::OnHealthDepleted(UAbilitySystemComponent* VictimASC, UAbilitySystemComponent* KillerASC)
{
	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT(">>> TARGET DUMMY [%s] KILLED! <<<"), *GetName());
	UE_LOG(LogBreachborne, Warning, TEXT("    Killer ASC owner: %s"),
		KillerASC && KillerASC->GetOwner() ? *KillerASC->GetOwner()->GetName() : TEXT("Unknown"));

	if (bAutoResetHealth)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("    Will reset health in %.1fs"), HealthResetDelay);
		UE_LOG(LogBreachborne, Warning, TEXT(""));

		GetWorldTimerManager().SetTimer(
			HealthResetTimerHandle,
			this,
			&ABBTargetDummy::ResetHealth,
			HealthResetDelay,
			false
		);
	}
}

void ABBTargetDummy::ResetHealth()
{
	if (HealthSet && AbilitySystemComponent)
	{
		HealthSet->SetHealth(HealthSet->GetMaxHealth());
		HealthSet->SetShield(HealthSet->GetMaxShield());

		// Remove dead tag so PostGameplayEffectExecute can fire death again
		AbilitySystemComponent->RemoveLooseGameplayTag(BBGameplayTags::State_Dead);

		UE_LOG(LogBreachborne, Warning, TEXT(""));
		UE_LOG(LogBreachborne, Warning, TEXT(">>> TARGET DUMMY [%s] HEALTH RESET to %.0f <<<"), *GetName(), HealthSet->GetHealth());
		UE_LOG(LogBreachborne, Warning, TEXT(""));
	}
}
