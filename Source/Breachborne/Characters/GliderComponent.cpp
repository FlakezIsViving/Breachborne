#include "GliderComponent.h"
#include "HunterCharacter.h"
#include "BBCharacterMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"

UGliderComponent::UGliderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UGliderComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGliderComponent, GliderState);
	DOREPLIFETIME(UGliderComponent, FuelLevel);
}

void UGliderComponent::BeginPlay()
{
	Super::BeginPlay();

	// Bind to landing event to auto-close glider
	if (ACharacter* OwnerChar = GetOwnerCharacter())
	{
		OwnerChar->LandedDelegate.AddDynamic(this, &UGliderComponent::OnCharacterLanded);
	}
}

void UGliderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// All glider logic is server-authoritative
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	switch (GliderState)
	{
	case EGliderState::Open:
	{
		const float PreviousFuel = FuelLevel;
		if (bFreeFuelUntilClosed)
		{
			FuelLevel = 1.0f;
			break;
		}

		FuelLevel = FMath::Max(FuelLevel - FuelDrainRate * DeltaTime, 0.0f);

		// Fuel empty: auto close.
		if (FuelLevel <= 0.0f)
		{
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("glider debug | fuel empty | owner=%s previousFuel=%.2f"),
				*GetOwner()->GetName(), PreviousFuel);
			CloseGlider();
		}
		break;
	}

	case EGliderState::Spiked:
	{
		// Count down spike stun
		SpikeTimer -= DeltaTime;
		if (SpikeTimer <= 0.0f)
		{
			RecoverFromSpike();
		}
		break;
	}

	case EGliderState::Closed:
	{
		if (FuelLevel < 1.0f)
		{
			const UBBCharacterMovementComponent* CMC = GetBBMovement();
			if (CMC && CMC->IsMovingOnGround())
			{
				const float PreviousFuel = FuelLevel;
				FuelLevel = FMath::Min(FuelLevel + FuelRefillRate * DeltaTime, 1.0f);
				if (PreviousFuel <= 0.0f || FuelLevel >= 1.0f)
				{
					UE_LOG(LogBreachborne, VeryVerbose, TEXT("glider debug | fuel refill | owner=%s previousFuel=%.2f fuel=%.2f grounded=1"),
						*GetOwner()->GetName(), PreviousFuel, FuelLevel);
				}
			}
		}
		break;
	}
	}
}

// --- Server RPCs ---

void UGliderComponent::ServerRequestOpenGlider_Implementation()
{
	if (GliderState != EGliderState::Closed)
	{
		return;
	}

	const ACharacter* OwnerChar = GetOwnerCharacter();
	if (!OwnerChar)
	{
		return;
	}

	const UCharacterMovementComponent* CMC = OwnerChar->GetCharacterMovement();
	if (!CMC || !CMC->IsFalling())
	{
		return;
	}

	if (FuelLevel < MinFuelToOpen)
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("glider debug | open denied | reason=no-fuel owner=%s fuel=%.2f min=%.2f"),
			*GetOwner()->GetName(), FuelLevel, MinFuelToOpen);
		return;
	}

	if (const UAbilitySystemComponent* ASC = GetASC())
	{
		if (ASC->HasMatchingGameplayTag(BBGameplayTags::State_Grounded))
		{
			return;
		}
	}

	// Block glider open while mantling
	if (const UBBCharacterMovementComponent* BBCMC = Cast<UBBCharacterMovementComponent>(CMC))
	{
		if (BBCMC->IsMantling())
		{
			return;
		}
	}

	// Check minimum altitude — trace down to see how far above ground we are
	FHitResult Hit;
	const FVector Start = OwnerChar->GetActorLocation();
	const FVector End = Start - FVector(0.0f, 0.0f, MinAltitudeForGlide);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerChar);

	const bool bHitGround = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params);
	if (bHitGround)
	{
		// Too close to the ground to open glider
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("GliderComponent: Open denied on %s — too close to ground (%.0f units)"),
			*GetOwner()->GetName(), Hit.Distance);
		return;
	}

	OpenGlider();
}

void UGliderComponent::ServerRequestCloseGlider_Implementation()
{
	if (GliderState == EGliderState::Open)
	{
		CloseGlider();
	}
}

// --- State Transitions ---

void UGliderComponent::OpenGlider()
{
	if (const UAbilitySystemComponent* ASC = GetASC())
	{
		if (ASC->HasMatchingGameplayTag(BBGameplayTags::State_Grounded))
		{
			return;
		}
	}

	// Block glider open while mantling
	if (UBBCharacterMovementComponent* CMC = GetBBMovement())
	{
		if (CMC->IsMantling())
		{
			return;
		}
	}

	GliderState = EGliderState::Open;

	// Set custom movement mode
	if (UBBCharacterMovementComponent* CMC = GetBBMovement())
	{
		CMC->EnterGliding();
	}

	// Add gliding gameplay tag
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->AddLooseGameplayTag(BBGameplayTags::State_Gliding);
	}

	UpdateGliderVisual();

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("glider debug | opened | owner=%s fuel=%.2f"), *GetOwner()->GetName(), FuelLevel);
}

void UGliderComponent::CloseGlider()
{
	UE_LOG(LogBreachborne, Warning, TEXT("BB_DROP|GLIDER|Close owner=%s fuel=%.2f freeDrop=%d loc=%s"),
		*GetNameSafe(GetOwner()),
		FuelLevel,
		bFreeFuelUntilClosed ? 1 : 0,
		GetOwner() ? *GetOwner()->GetActorLocation().ToCompactString() : TEXT("no-owner"));

	bFreeFuelUntilClosed = false;
	GliderState = EGliderState::Closed;

	// Exit custom movement mode
	if (UBBCharacterMovementComponent* CMC = GetBBMovement())
	{
		if (CMC->IsGliding())
		{
			CMC->ExitGliding();
		}
	}

	// Remove gliding gameplay tag
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->RemoveLooseGameplayTag(BBGameplayTags::State_Gliding);
	}

	UpdateGliderVisual();

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("glider debug | closed | owner=%s fuel=%.2f"), *GetOwner()->GetName(), FuelLevel);
}

void UGliderComponent::TriggerSpike(AActor* Instigator)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (GliderState != EGliderState::Open)
	{
		return;
	}

	LastDamageInstigator = Instigator;
	bFreeFuelUntilClosed = false;
	SpikeTimer = SpikeDuration;
	GliderState = EGliderState::Spiked;

	// Switch movement to spiked
	if (UBBCharacterMovementComponent* CMC = GetBBMovement())
	{
		CMC->EnterSpiked();
	}

	// Swap gameplay tags: remove Gliding, add Spiked
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->RemoveLooseGameplayTag(BBGameplayTags::State_Gliding);
		ASC->AddLooseGameplayTag(BBGameplayTags::State_Spiked);
	}

	UpdateGliderVisual();

	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT("=== SPIKED: %s by %s ==="),
		*GetOwner()->GetName(), Instigator ? *Instigator->GetName() : TEXT("Self/Unknown"));
}

void UGliderComponent::RecoverFromSpike()
{
	bFreeFuelUntilClosed = false;
	GliderState = EGliderState::Closed;

	// Exit spiked movement mode
	if (UBBCharacterMovementComponent* CMC = GetBBMovement())
	{
		if (CMC->IsSpiked())
		{
			CMC->ExitSpiked();
		}
	}

	// Remove spiked tag
	if (UAbilitySystemComponent* ASC = GetASC())
	{
		ASC->RemoveLooseGameplayTag(BBGameplayTags::State_Spiked);
	}

	UpdateGliderVisual();

	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT("=== SPIKE RECOVERED: %s ==="), *GetOwner()->GetName());

	OnSpikeRecovered.Broadcast();
}

void UGliderComponent::OnCharacterLanded(const FHitResult& Hit)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	UE_LOG(LogBreachborne, Warning, TEXT("BB_DROP|GLIDER|Landed owner=%s state=%d fuel=%.2f hitActor=%s hitComp=%s loc=%s"),
		*GetNameSafe(GetOwner()),
		static_cast<int32>(GliderState),
		FuelLevel,
		*GetNameSafe(Hit.GetActor()),
		*GetNameSafe(Hit.GetComponent()),
		*GetOwner()->GetActorLocation().ToCompactString());

	// Force close on landing regardless of current state
	if (GliderState == EGliderState::Open)
	{
		CloseGlider();
	}
	else if (GliderState == EGliderState::Spiked)
	{
		RecoverFromSpike();
	}

	// Reset heat on landing
	UE_LOG(LogBreachborne, Verbose, TEXT("glider debug | landed | owner=%s fuel=%.2f"),
		*GetOwner()->GetName(), FuelLevel);
}

// --- OnRep ---

void UGliderComponent::OnRep_GliderState()
{
	UpdateGliderVisual();

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("GliderComponent [Client]: State changed to %d on %s"),
		static_cast<uint8>(GliderState), *GetOwner()->GetName());
}

// --- Visual ---

void UGliderComponent::UpdateGliderVisual() const
{
	// Only AHunterCharacter has a glider indicator mesh — safe to skip for bots/other owners
	if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetOwner()))
	{
		if (UStaticMeshComponent* GliderMesh = Hunter->GetGliderIndicatorMesh())
		{
			GliderMesh->SetVisibility(GliderState == EGliderState::Open);
		}
	}
}

// --- Force Open/Close (server-only, for bots/AI) ---

void UGliderComponent::ForceOpenGlider()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (GliderState == EGliderState::Closed)
	{
		// Force means bypass normal fuel validation for bots/test helpers.
		if (FuelLevel < MinFuelToOpen)
		{
			FuelLevel = MinFuelToOpen;
		}
		OpenGlider();
	}
}

void UGliderComponent::ForceOpenFreeDropGlider()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	FuelLevel = 1.0f;
	bFreeFuelUntilClosed = true;

	if (GliderState == EGliderState::Closed)
	{
		OpenGlider();
	}

	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|GLIDER|FreeDropOpen owner=%s fuel=%.2f"),
		*GetNameSafe(GetOwner()),
		FuelLevel);
	UE_LOG(LogBreachborne, Warning, TEXT("BB_DROP|GLIDER|FreeDropOpen owner=%s state=%d fuel=%.2f loc=%s"),
		*GetNameSafe(GetOwner()),
		static_cast<int32>(GliderState),
		FuelLevel,
		*GetOwner()->GetActorLocation().ToCompactString());
}

void UGliderComponent::ForceCloseGlider()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (GliderState == EGliderState::Open)
	{
		CloseGlider();
	}
}

void UGliderComponent::CancelForMantle()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (GliderState == EGliderState::Open)
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | glider cancel | owner=%s previousState=Open"),
			*GetNameSafe(GetOwner()));
		CloseGlider();
	}
	else if (GliderState == EGliderState::Spiked)
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | glider cancel | owner=%s previousState=Spiked"),
			*GetNameSafe(GetOwner()));
		RecoverFromSpike();
	}
	else
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | glider cancel | owner=%s previousState=Closed"),
			*GetNameSafe(GetOwner()));
	}
}

// --- Cached Accessors ---

ACharacter* UGliderComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

UBBCharacterMovementComponent* UGliderComponent::GetBBMovement() const
{
	if (const ACharacter* OwnerChar = GetOwnerCharacter())
	{
		return Cast<UBBCharacterMovementComponent>(OwnerChar->GetCharacterMovement());
	}
	return nullptr;
}

UAbilitySystemComponent* UGliderComponent::GetASC() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return ASI->GetAbilitySystemComponent();
	}
	return nullptr;
}
