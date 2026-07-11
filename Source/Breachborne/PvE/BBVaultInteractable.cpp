#include "BBVaultInteractable.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Items/BBWorldItem.h"
#include "Breachborne/Breachborne.h"

ABBVaultInteractable::ABBVaultInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	VaultMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VaultMesh"));
	RootComponent = VaultMesh;

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetSphereRadius(InteractRadius);
	InteractSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractSphere->SetupAttachment(RootComponent);
}

void ABBVaultInteractable::BeginPlay()
{
	Super::BeginPlay();
}

void ABBVaultInteractable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABBVaultInteractable, HackProgress);
	DOREPLIFETIME(ABBVaultInteractable, bHacking);
	DOREPLIFETIME(ABBVaultInteractable, bDestroyed);
}

void ABBVaultInteractable::ServerBeginHack(ABreachbornePlayerState* HackerPS)
{
	if (!HasAuthority() || bDestroyed || bLooted || bHacking || !HackerPS)
	{
		return;
	}

	APawn* HackerPawn = HackerPS->GetPawn();
	if (!HackerPawn)
	{
		return;
	}

	// Proximity check
	const float Dist = FVector::Dist(HackerPawn->GetActorLocation(), GetActorLocation());
	if (Dist > InteractRadius)
	{
		return;
	}

	CurrentHacker     = HackerPS;
	HackStartLocation = HackerPawn->GetActorLocation();
	ChannelElapsed    = 0.0f;
	HackProgress      = 0.0f;
	bHacking          = true;

	// Monitor hacker health for damage cancellation
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(HackerPS))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			HackerHealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(
				UBBHealthSet::GetHealthAttribute()).AddUObject(this, &ABBVaultInteractable::OnHackerTookDamage);
		}
	}

	// 10Hz tick
	GetWorldTimerManager().SetTimer(HackTickHandle, this, &ABBVaultInteractable::HackTick, 0.1f, true);

	UE_LOG(LogBreachborne, Log, TEXT("Vault [%s]: %s began hacking"), *GetName(), *HackerPS->GetPlayerName());
}

void ABBVaultInteractable::ServerCancelHack()
{
	if (!HasAuthority() || !bHacking)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(HackTickHandle);

	if (CurrentHacker)
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(CurrentHacker.Get()))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				ASC->GetGameplayAttributeValueChangeDelegate(UBBHealthSet::GetHealthAttribute())
					.Remove(HackerHealthHandle);
			}
		}
	}

	bHacking       = false;
	HackProgress   = 0.0f;
	ChannelElapsed = 0.0f;
	CurrentHacker  = nullptr;

	UE_LOG(LogBreachborne, Log, TEXT("Vault [%s]: Hack cancelled"), *GetName());
}

void ABBVaultInteractable::HackTick()
{
	if (!HasAuthority() || !CurrentHacker)
	{
		ServerCancelHack();
		return;
	}

	APawn* HackerPawn = CurrentHacker->GetPawn();
	if (!HackerPawn)
	{
		ServerCancelHack();
		return;
	}

	// Movement cancel check
	const float MoveDist = FVector::Dist2D(HackerPawn->GetActorLocation(), HackStartLocation);
	if (MoveDist > CancelMoveThreshold)
	{
		UE_LOG(LogBreachborne, Log, TEXT("Vault [%s]: Hack cancelled — hacker moved %.0f cm"), *GetName(), MoveDist);
		ServerCancelHack();
		return;
	}

	// Proximity cancel check
	const float Dist = FVector::Dist(HackerPawn->GetActorLocation(), GetActorLocation());
	if (Dist > InteractRadius * 1.2f) // small leeway
	{
		UE_LOG(LogBreachborne, Log, TEXT("Vault [%s]: Hack cancelled — hacker left radius"), *GetName());
		ServerCancelHack();
		return;
	}

	ChannelElapsed += 0.1f;
	HackProgress    = FMath::Clamp(ChannelElapsed / HackDuration, 0.0f, 1.0f);

	if (ChannelElapsed >= HackDuration)
	{
		// Hack complete!
		GetWorldTimerManager().ClearTimer(HackTickHandle);
		bHacking   = false;
		bLooted    = true;
		HackProgress = 1.0f;

		UE_LOG(LogBreachborne, Log, TEXT("Vault [%s]: Hacked by %s"), *GetName(), *CurrentHacker->GetPlayerName());

		OnVaultHacked.Broadcast(this, CurrentHacker.Get());
		SpawnReward();

		// Hide vault mesh — it's been looted
		VaultMesh->SetVisibility(false);
		VaultMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CurrentHacker = nullptr;
	}
}

void ABBVaultInteractable::OnHackerTookDamage(const FOnAttributeChangeData& Data)
{
	// Any health decrease cancels the hack
	if (Data.NewValue < Data.OldValue)
	{
		UE_LOG(LogBreachborne, Log, TEXT("Vault [%s]: Hack cancelled — hacker took damage"), *GetName());
		ServerCancelHack();
	}
}

void ABBVaultInteractable::SpawnReward()
{
	if (bDestroyed || !ItemDropClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector DropLoc = GetActorLocation() + FVector(0.0f, 0.0f, 60.0f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ABBWorldItem* Item = World->SpawnActor<ABBWorldItem>(ItemDropClass, DropLoc, FRotator::ZeroRotator, Params);
	if (Item)
	{
		Item->SetPickupItemID(VaultItemID);
	}
}

void ABBVaultInteractable::ApplyPowerDestruction_Implementation(const FBBPowerDestructionContext& Context)
{
	if (!HasAuthority() || bDestroyed)
	{
		return;
	}

	if (bHacking)
	{
		ServerCancelHack();
	}

	bDestroyed = true;
	bLooted = true;
	HackProgress = 0.0f;
	bHacking = false;

	ApplyDestroyedPresentation();

	ForceNetUpdate();
	UE_LOG(LogBreachborne, Log, TEXT("Vault [%s]: destroyed by %s"), *GetName(), *GetNameSafe(Context.InstigatorActor));
}

void ABBVaultInteractable::OnRep_Destroyed()
{
	ApplyDestroyedPresentation();
}

void ABBVaultInteractable::ApplyDestroyedPresentation()
{
	if (!bDestroyed)
	{
		return;
	}

	if (InteractSphere)
	{
		InteractSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (VaultMesh)
	{
		VaultMesh->SetVisibility(false, true);
		VaultMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
