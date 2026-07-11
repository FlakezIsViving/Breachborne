#include "BBCreepCamp.h"
#include "BBCreepCharacter.h"
#include "BBContractSubsystem.h"
#include "AIController.h"
#include "EngineUtils.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Breachborne.h"

ABBCreepCamp::ABBCreepCamp()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false; // Camp itself has no visual — just a server-side spawner
}

void ABBCreepCamp::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	SpawnCreeps();
}

void ABBCreepCamp::SpawnCreeps()
{
	if (!CreepClass)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BBCreepCamp [%s]: CreepClass not set — assign one in the BP subclass."), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	LiveCreeps.Reset();
	AliveCreepCount = 0;

	const FVector Origin = GetActorLocation();

	for (int32 i = 0; i < CreepCount; ++i)
	{
		// Distribute creeps evenly around the camp origin
		const float Angle = (2.0f * PI / CreepCount) * i;
		const FVector SpawnOffset(
			FMath::Cos(Angle) * SpawnRadius,
			FMath::Sin(Angle) * SpawnRadius,
			0.0f);
		const FVector SpawnLoc = Origin + SpawnOffset;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ABBCreepCharacter* Creep = World->SpawnActor<ABBCreepCharacter>(CreepClass, SpawnLoc, FRotator::ZeroRotator, Params);
		if (Creep)
		{
			Creep->InitForCamp(this, SpawnLoc);
			LiveCreeps.Add(Creep);
			++AliveCreepCount;

			// Spawn an AIController and possess the creep
			AAIController* AIC = World->SpawnActor<AAIController>(AAIController::StaticClass(), SpawnLoc, FRotator::ZeroRotator);
			if (AIC)
			{
				AIC->Possess(Creep);
			}
		}
	}

	UE_LOG(LogBreachborne, Log, TEXT("BBCreepCamp [%s]: Spawned %d creeps"), *GetName(), AliveCreepCount);
}

void ABBCreepCamp::OnCreepDied(ABBCreepCharacter* Creep)
{
	if (!HasAuthority())
	{
		return;
	}

	LiveCreeps.Remove(Creep);
	--AliveCreepCount;

	UE_LOG(LogBreachborne, Log, TEXT("BBCreepCamp [%s]: Creep died, %d remaining"), *GetName(), AliveCreepCount);

	if (AliveCreepCount <= 0)
	{
		AliveCreepCount = 0;
		UE_LOG(LogBreachborne, Log, TEXT("BBCreepCamp [%s]: CLEARED"), *GetName());
		OnCampCleared.Broadcast(this);

		// Report to the contract subsystem — any living team nearby gets credit.
		// We award the team that landed the last kill (the last creep notified us).
		// The creep already rewarded the killer's gold/shards; here we credit the camp clear.
		// Identify the clearing team by finding the closest living hunter to the camp.
		if (UBBContractSubsystem* Contracts = GetWorld() ? GetWorld()->GetSubsystem<UBBContractSubsystem>() : nullptr)
		{
			int32 ClearingTeamID = -1;
			float ClosestDist = 3000.0f; // Only count teams within 3000cm
			const FVector CampLoc = GetActorLocation();

			for (TActorIterator<ABreachbornePlayerState> It(GetWorld()); It; ++It)
			{
				ABreachbornePlayerState* PS = *It;
				if (!PS || !PS->GetIsAlive())
				{
					continue;
				}
				if (APawn* Pawn = PS->GetPawn())
				{
					const float Dist = FVector::Dist(Pawn->GetActorLocation(), CampLoc);
					if (Dist < ClosestDist)
					{
						ClosestDist    = Dist;
						ClearingTeamID = PS->GetTeamID();
					}
				}
			}

			if (ClearingTeamID != -1)
			{
				Contracts->ReportCampClear(ClearingTeamID);
			}
		}

		if (bRespawns)
		{
			GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ABBCreepCamp::OnRespawnTimerFired, RespawnDelay, false);
		}
	}
}

void ABBCreepCamp::OnRespawnTimerFired()
{
	UE_LOG(LogBreachborne, Log, TEXT("BBCreepCamp [%s]: Respawning"), *GetName());
	SpawnCreeps();
}
