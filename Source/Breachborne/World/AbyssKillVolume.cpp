#include "AbyssKillVolume.h"
#include "Components/BoxComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Breachborne/Characters/GliderComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Breachborne.h"

AAbyssKillVolume::AAbyssKillVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	KillVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("KillVolume"));
	KillVolume->SetBoxExtent(FVector(50000.0f, 50000.0f, 500.0f));
	KillVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	KillVolume->SetGenerateOverlapEvents(true);
	SetRootComponent(KillVolume);

	// Only needs to exist on the server for gameplay
	bReplicates = false;
}

void AAbyssKillVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	// Server only — kill any character that enters the abyss
	if (!HasAuthority())
	{
		return;
	}

	// Works for AHunterCharacter, ABBGlideBot, or any actor implementing IAbilitySystemInterface
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OtherActor);
	if (!ASI)
	{
		return;
	}

	UAbilitySystemComponent* VictimASC = ASI->GetAbilitySystemComponent();
	if (!VictimASC)
	{
		return;
	}

	// Already dead — don't double-kill
	if (VictimASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
	{
		return;
	}

	// Determine kill credit: if spiked, the spiker gets the kill
	UAbilitySystemComponent* KillerASC = nullptr;
	UGliderComponent* Glider = OtherActor->FindComponentByClass<UGliderComponent>();
	const bool bWasSpiked = Glider && Glider->GetGliderState() == EGliderState::Spiked;

	if (bWasSpiked)
	{
		// Resolve the instigator who caused the spike to their ASC
		AActor* SpikeInstigator = Glider->GetLastDamageInstigator();
		if (SpikeInstigator)
		{
			if (IAbilitySystemInterface* InstigatorASI = Cast<IAbilitySystemInterface>(SpikeInstigator))
			{
				KillerASC = InstigatorASI->GetAbilitySystemComponent();
			}
			else if (APlayerState* InstigatorPS = Cast<APlayerState>(SpikeInstigator))
			{
				if (IAbilitySystemInterface* PSASI = Cast<IAbilitySystemInterface>(InstigatorPS))
				{
					KillerASC = PSASI->GetAbilitySystemComponent();
				}
			}
		}
	}

	// Kill via HealthSet — find it on the ASC owner (PlayerState for hunters, self for bots)
	// GetSet returns const; const_cast is safe here — we're the server applying a kill
	UBBHealthSet* HealthSet = const_cast<UBBHealthSet*>(VictimASC->GetSet<UBBHealthSet>());
	if (HealthSet)
	{
		HealthSet->SetHealth(0.0f);
		HealthSet->SetShield(0.0f);
	}

	// Add dead tag
	VictimASC->AddLooseGameplayTag(BBGameplayTags::State_Dead);

	// Close glider if still in open state — use ForceClose (works for bots with no owning connection)
	if (Glider && Glider->GetGliderState() == EGliderState::Open)
	{
		Glider->ForceCloseGlider();
	}

	// Resolve names for logging
	ABreachbornePlayerState* VictimPS = Cast<ABreachbornePlayerState>(VictimASC->GetOwnerActor());
	const FString VictimName = VictimPS ? VictimPS->GetPlayerName() : OtherActor->GetName();

	// Log with banner
	if (bWasSpiked)
	{
		ABreachbornePlayerState* KillerPS = KillerASC ? Cast<ABreachbornePlayerState>(KillerASC->GetOwnerActor()) : nullptr;
		const FString KillerName = KillerPS ? KillerPS->GetPlayerName() : (KillerASC && KillerASC->GetOwner() ? KillerASC->GetOwner()->GetName() : TEXT("Unknown"));
		UE_LOG(LogBreachborne, Warning, TEXT(""));
		UE_LOG(LogBreachborne, Warning, TEXT("=== ABYSS SPIKE KILL: %s killed by %s ==="),
			*VictimName, *KillerName);
	}
	else
	{
		UE_LOG(LogBreachborne, Warning, TEXT(""));
		UE_LOG(LogBreachborne, Warning, TEXT("=== ABYSS DEATH: %s fell off the map ==="),
			*VictimName);
	}

	// Broadcast OnHealthDepleted — this feeds into GameMode's death handler
	// (pawn disable, kill tracking, dev respawn)
	if (HealthSet)
	{
		HealthSet->OnHealthDepleted.Broadcast(VictimASC, KillerASC);
	}

	// Push updated health values to clients (only applies to player-owned PlayerStates)
	if (VictimPS)
	{
		VictimPS->UpdateHealthProxy();
	}
}
