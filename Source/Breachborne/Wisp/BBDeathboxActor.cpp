#include "BBDeathboxActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Breachborne.h"

ABBDeathboxActor::ABBDeathboxActor()
{
	bReplicates = true;
	bAlwaysRelevant = true; // Visible to all clients regardless of distance
	PrimaryActorTick.bCanEverTick = false;

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetSphereRadius(InteractRadius);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(InteractSphere);

	DeathboxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DeathboxMesh"));
	DeathboxMesh->SetupAttachment(InteractSphere);
	DeathboxMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DeathboxMesh->SetIsReplicated(true);
}

void ABBDeathboxActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBDeathboxActor, InventorySnapshot);
	DOREPLIFETIME(ABBDeathboxActor, VictimName);
	DOREPLIFETIME(ABBDeathboxActor, ReviveProgress);
	DOREPLIFETIME(ABBDeathboxActor, bIsBeingRevived);
}

void ABBDeathboxActor::InitDeathbox(ABreachbornePlayerState* InVictimPS, const FRepInventoryData& InInventory)
{
	if (!HasAuthority() || !InVictimPS)
	{
		return;
	}

	VictimPlayerState = InVictimPS;
	InventorySnapshot = InInventory;
	VictimName = InVictimPS->GetPlayerName();

	UE_LOG(LogBreachborne, Log, TEXT("DeathboxActor: Initialized for %s at %s"),
		*VictimName, *GetActorLocation().ToString());

	// Auto-destroy after DeathboxLifetime seconds
	GetWorldTimerManager().SetTimer(LifetimeHandle, [WeakThis = TWeakObjectPtr<ABBDeathboxActor>(this)]()
	{
		if (ABBDeathboxActor* Box = WeakThis.Get())
		{
			UE_LOG(LogBreachborne, Log, TEXT("DeathboxActor: Lifetime expired for %s — destroying"), *Box->VictimName);
			Box->Destroy();
		}
	}, DeathboxLifetime, false);
}

void ABBDeathboxActor::ServerBeginReviveChannel(AHunterCharacter* Reviver)
{
	if (!HasAuthority() || !Reviver || !VictimPlayerState)
	{
		return;
	}

	// Cancel any existing channel first
	if (bIsBeingRevived)
	{
		ServerCancelReviveChannel();
	}

	// Proximity check
	const float DistSq = FVector::DistSquared(GetActorLocation(), Reviver->GetActorLocation());
	if (DistSq > FMath::Square(InteractRadius))
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("DeathboxActor: Reviver %s out of range (%.0f > %.0f)"),
			*Reviver->GetName(), FMath::Sqrt(DistSq), InteractRadius);
		return;
	}

	// Team check: reviver must be on the same team as the victim
	const ABreachbornePlayerState* ReviverPS = Reviver->GetPlayerState<ABreachbornePlayerState>();
	if (!ReviverPS || ReviverPS->GetTeamID() != VictimPlayerState->GetTeamID())
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("DeathboxActor: Reviver %s is enemy team — cannot revive %s"),
			*Reviver->GetName(), *VictimName);
		return;
	}

	UE_LOG(LogBreachborne, Log, TEXT("DeathboxActor: %s beginning revive channel on %s's deathbox"),
		*ReviverPS->GetPlayerName(), *VictimName);

	CurrentReviver = Reviver;
	ChannelElapsed = 0.0f;
	bIsBeingRevived = true;

	// Monitor reviver's health — any damage cancels the channel
	UAbilitySystemComponent* ReviverASC = Reviver->GetAbilitySystemComponent();
	if (ReviverASC)
	{
		if (const UBBHealthSet* ReviverHealth = ReviverASC->GetSet<UBBHealthSet>())
		{
			ReviverDamageHandle = ReviverASC->GetGameplayAttributeValueChangeDelegate(
				UBBHealthSet::GetHealthAttribute()).AddUObject(this, &ABBDeathboxActor::OnReviverTakeDamage);
		}
	}

	// Tick the channel at 10Hz
	GetWorldTimerManager().SetTimer(ReviveTickHandle, this, &ABBDeathboxActor::ReviveChannelTick, 0.1f, true);
}

void ABBDeathboxActor::ServerCancelReviveChannel()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ReviveTickHandle);

	// Unbind damage monitor
	if (IsValid(CurrentReviver))
	{
		if (UAbilitySystemComponent* ReviverASC = CurrentReviver->GetAbilitySystemComponent())
		{
			ReviverASC->GetGameplayAttributeValueChangeDelegate(
				UBBHealthSet::GetHealthAttribute()).Remove(ReviverDamageHandle);
		}
	}

	bIsBeingRevived = false;
	ReviveProgress = 0.0f;
	ChannelElapsed = 0.0f;
	CurrentReviver = nullptr;
	ReviverDamageHandle.Reset();

	OnReviveCancelled.Broadcast(this);

	UE_LOG(LogBreachborne, Log, TEXT("DeathboxActor: Revive channel cancelled on %s's deathbox"), *VictimName);
}

void ABBDeathboxActor::ReviveChannelTick()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValid(CurrentReviver))
	{
		ServerCancelReviveChannel();
		return;
	}

	// Check proximity — moving away cancels channel
	const float DistSq = FVector::DistSquared(GetActorLocation(), CurrentReviver->GetActorLocation());
	if (DistSq > FMath::Square(InteractRadius * 1.2f)) // 20% leeway before cancel
	{
		UE_LOG(LogBreachborne, Log, TEXT("DeathboxActor: Reviver moved too far — cancelling channel"));
		ServerCancelReviveChannel();
		return;
	}

	ChannelElapsed += 0.1f;
	ReviveProgress = FMath::Clamp(ChannelElapsed / ReviveChannelDuration, 0.0f, 1.0f);

	if (ChannelElapsed >= ReviveChannelDuration)
	{
		// Channel complete — notify GameMode
		AHunterCharacter* CompletedReviver = CurrentReviver.Get();

		// Clean up state before firing delegate (GameMode may destroy this actor)
		GetWorldTimerManager().ClearTimer(ReviveTickHandle);
		bIsBeingRevived = false;

		if (IsValid(CurrentReviver))
		{
			if (UAbilitySystemComponent* ReviverASC = CurrentReviver->GetAbilitySystemComponent())
			{
				ReviverASC->GetGameplayAttributeValueChangeDelegate(
					UBBHealthSet::GetHealthAttribute()).Remove(ReviverDamageHandle);
			}
		}
		CurrentReviver = nullptr;
		ReviverDamageHandle.Reset();

		UE_LOG(LogBreachborne, Log, TEXT("DeathboxActor: Revive channel complete for %s!"), *VictimName);
		OnReviveComplete.Broadcast(this, CompletedReviver);
	}
}

void ABBDeathboxActor::OnReviverTakeDamage(const FOnAttributeChangeData& Data)
{
	// Health dropped → reviver took damage — cancel the channel
	if (Data.NewValue < Data.OldValue)
	{
		UE_LOG(LogBreachborne, Log, TEXT("DeathboxActor: Reviver took damage (%.1f → %.1f) — cancelling revive channel"),
			Data.OldValue, Data.NewValue);
		ServerCancelReviveChannel();
	}
}

void ABBDeathboxActor::OnRep_ReviveProgress()
{
	// Client cosmetic hook — drives progress bar on HUD.
	// No logic required; UI reads ReviveProgress directly via getter.
}
