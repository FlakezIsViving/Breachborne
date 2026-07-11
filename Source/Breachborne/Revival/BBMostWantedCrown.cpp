#include "BBMostWantedCrown.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"

ABBMostWantedCrown::ABBMostWantedCrown()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = false;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetSphereRadius(PickupRadius);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &ABBMostWantedCrown::OnPickupOverlap);
	SetRootComponent(PickupSphere);

	CrownMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrownMesh"));
	CrownMesh->SetupAttachment(PickupSphere);
	CrownMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrownMesh->SetIsReplicated(true);
}

void ABBMostWantedCrown::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBMostWantedCrown, CrownHolder);
	DOREPLIFETIME(ABBMostWantedCrown, bClaimed);
	DOREPLIFETIME_CONDITION(ABBMostWantedCrown, CrownTimerRemaining, COND_None); // Low-freq is fine via NetUpdateFrequency
}

void ABBMostWantedCrown::BeginPlay()
{
	Super::BeginPlay();
	CrownTimerRemaining = CrownDuration;
}

void ABBMostWantedCrown::OnPickupOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bClaimed)
	{
		return;
	}

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(OtherActor);
	if (!Hunter)
	{
		return;
	}

	ABreachbornePlayerState* HunterPS = Hunter->GetPlayerState<ABreachbornePlayerState>();
	if (!HunterPS || !HunterPS->GetIsAlive())
	{
		return;
	}

	// Claim the crown
	bClaimed = true;
	CrownHolder = HunterPS;
	CrownTimerRemaining = CrownDuration;
	TimeSinceLastBroadcast = 0.0f;

	// Tag the holder's ASC with State.MostWanted
	if (UAbilitySystemComponent* ASC = Hunter->GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(BBGameplayTags::State_MostWanted);
	}

	// Attach crown mesh to the hunter (cosmetic — clients replicate via mesh visibility/location)
	// For now, hide the world mesh; we'll handle visual separately via GameplayCues later.
	CrownMesh->SetVisibility(false, true);

	// Start the countdown ticker at 1Hz
	GetWorldTimerManager().SetTimer(CountdownTickHandle, this, &ABBMostWantedCrown::CrownCountdownTick, 1.0f, true);

	UE_LOG(LogBreachborne, Warning, TEXT("=== MOST WANTED: %s claimed the crown ==="), *HunterPS->GetPlayerName());

	OnCrownPickedUp.Broadcast(this, HunterPS);
}

void ABBMostWantedCrown::CrownCountdownTick()
{
	if (!HasAuthority() || !bClaimed)
	{
		return;
	}

	CrownTimerRemaining = FMath::Max(0.0f, CrownTimerRemaining - 1.0f);
	TimeSinceLastBroadcast += 1.0f;

	// Fire periodic location broadcast
	if (TimeSinceLastBroadcast >= BroadcastInterval)
	{
		TimeSinceLastBroadcast = 0.0f;
		FireLocationBroadcast();
	}

	// Timer expired → reward trigger
	if (CrownTimerRemaining <= 0.0f)
	{
		GetWorldTimerManager().ClearTimer(CountdownTickHandle);

		UE_LOG(LogBreachborne, Warning, TEXT("=== MOST WANTED: Timer expired for %s — squad revive reward ==="),
			CrownHolder ? *CrownHolder->GetPlayerName() : TEXT("Unknown"));

		OnCrownTimerExpired.Broadcast(this);

		// Reset crown to unclaimed at holder's current location
		if (CrownHolder)
		{
			if (UAbilitySystemComponent* ASC = CrownHolder->GetAbilitySystemComponent())
			{
				ASC->RemoveLooseGameplayTag(BBGameplayTags::State_MostWanted);
			}
		}

		FVector DropLocation = GetActorLocation();
		if (CrownHolder)
		{
			// Use controller's pawn location if available
			if (const AController* C = CrownHolder->GetOwner<AController>())
			{
				if (const APawn* P = C->GetPawn())
				{
					DropLocation = P->GetActorLocation();
				}
			}
		}

		ServerDropCrown(DropLocation);
	}
}

void ABBMostWantedCrown::FireLocationBroadcast()
{
	if (!CrownHolder)
	{
		return;
	}

	// The GameMode (or a multicast RPC from here) handles the UI ping.
	// For now log on server — GameMode binds to OnCrownPickedUp to set up its own broadcast multicast.
	if (const AController* C = CrownHolder->GetOwner<AController>())
	{
		if (const APawn* P = C->GetPawn())
		{
			UE_LOG(LogBreachborne, Warning, TEXT("MOST WANTED LOCATION BROADCAST: %s is at %s"),
				*CrownHolder->GetPlayerName(), *P->GetActorLocation().ToString());
		}
	}
}

void ABBMostWantedCrown::ServerOnHolderDied(ABreachbornePlayerState* KillerPS, const FVector& DeathLocation)
{
	if (!HasAuthority() || !bClaimed)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(CountdownTickHandle);

	// Remove MostWanted tag from the dead holder
	if (CrownHolder)
	{
		if (UAbilitySystemComponent* ASC = CrownHolder->GetAbilitySystemComponent())
		{
			ASC->RemoveLooseGameplayTag(BBGameplayTags::State_MostWanted);
		}
	}

	UE_LOG(LogBreachborne, Warning, TEXT("=== MOST WANTED: Crown holder %s eliminated by %s — killer squad revive ==="),
		CrownHolder ? *CrownHolder->GetPlayerName() : TEXT("Unknown"),
		KillerPS ? *KillerPS->GetPlayerName() : TEXT("Unknown"));

	OnCrownHolderKilled.Broadcast(this, KillerPS);

	// Drop crown at death location and reset
	ServerDropCrown(DeathLocation);
}

void ABBMostWantedCrown::ServerDropCrown(const FVector& NewLocation)
{
	if (!HasAuthority())
	{
		return;
	}

	CrownHolder = nullptr;
	bClaimed = false;
	CrownTimerRemaining = CrownDuration;
	TimeSinceLastBroadcast = 0.0f;

	SetActorLocation(NewLocation);
	CrownMesh->SetVisibility(true, true);

	GetWorldTimerManager().ClearTimer(CountdownTickHandle);

	UE_LOG(LogBreachborne, Log, TEXT("MostWantedCrown: Dropped at %s — available for pickup"), *NewLocation.ToString());
}

void ABBMostWantedCrown::OnRep_CrownHolder()
{
	// Client cosmetic hook
}

void ABBMostWantedCrown::OnRep_Claimed()
{
	// Client cosmetic hook — show/hide crown mesh based on claimed state
	CrownMesh->SetVisibility(!bClaimed, true);
}
