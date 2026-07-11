#include "BBRespawnBeacon.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Core/BreachborneTypes.h"
#include "Breachborne/Breachborne.h"

ABBRespawnBeacon::ABBRespawnBeacon()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = false;

	InteractSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractSphere"));
	InteractSphere->SetSphereRadius(InteractRadius);
	InteractSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(InteractSphere);

	BeaconMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeaconMesh"));
	BeaconMesh->SetupAttachment(InteractSphere);
	BeaconMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BeaconMesh->SetIsReplicated(true);
}

void ABBRespawnBeacon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBRespawnBeacon, bAvailable);
}

bool ABBRespawnBeacon::ServerActivateBeacon(AHunterCharacter* Activator)
{
	if (!HasAuthority() || !bAvailable || !Activator)
	{
		return false;
	}

	// Proximity check
	const float DistSq = FVector::DistSquared(GetActorLocation(), Activator->GetActorLocation());
	if (DistSq > FMath::Square(InteractRadius))
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("RespawnBeacon: Activator out of range"));
		return false;
	}

	ABreachbornePlayerState* ActivatorPS = Activator->GetPlayerState<ABreachbornePlayerState>();
	if (!ActivatorPS)
	{
		return false;
	}

	UE_LOG(LogBreachborne, Log, TEXT("RespawnBeacon: Activated by %s (Team %d)"),
		*ActivatorPS->GetPlayerName(), ActivatorPS->GetTeamID());

	// Consume the beacon
	bAvailable = false;

	// Fire delegate — GameMode handles the actual squad revival
	OnBeaconActivated.Broadcast(this, ActivatorPS);

	// Destroy after a short delay (let clients see the consumed state)
	SetLifeSpan(2.0f);

	return true;
}

void ABBRespawnBeacon::OnRep_Available()
{
	// Client cosmetic hook — play consumed VFX when bAvailable flips to false.
	// No logic required; the actor will be destroyed server-side shortly after.
	if (!bAvailable)
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("RespawnBeacon: Beacon consumed (client notification)"));
	}
}
