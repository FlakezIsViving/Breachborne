#include "BBContractSubsystem.h"
#include "EngineUtils.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Core/BreachborneGameState.h"
#include "Breachborne/Items/BBInventoryManager.h"
#include "Breachborne/Breachborne.h"

bool UBBContractSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only create on the server (GameMode world)
	UWorld* World = Cast<UWorld>(Outer);
	return World && World->GetNetMode() != NM_Client;
}

void UBBContractSubsystem::IssueContractToTeam(int32 TeamID)
{
	if (TeamContracts.Contains(TeamID))
	{
		return; // Team already has a contract
	}

	FBBContract Contract;

	// Randomly assign Brawler or Creep Farm
	const bool bBrawler = FMath::RandBool();
	Contract.ContractType    = bBrawler ? EBBContractType::Brawler : EBBContractType::CreepFarm;
	Contract.ObjectiveCount  = bBrawler ? 3 : 2;
	Contract.GoldReward      = bBrawler ? 60 : 80;
	Contract.CurrentProgress = 0;
	Contract.bCompleted      = false;

	TeamContracts.Add(TeamID, Contract);

	UE_LOG(LogBreachborne, Log, TEXT("Contracts: Team %d issued %s contract (%d/%d, reward: %dg)"),
		TeamID,
		Contract.ContractType == EBBContractType::Brawler ? TEXT("Brawler") : TEXT("Creep Farm"),
		0, Contract.ObjectiveCount, Contract.GoldReward);
}

void UBBContractSubsystem::ReportKill(int32 TeamID)
{
	ProgressContract(TeamID, EBBContractType::Brawler);
}

void UBBContractSubsystem::ReportCampClear(int32 TeamID)
{
	ProgressContract(TeamID, EBBContractType::CreepFarm);
}

FBBContract UBBContractSubsystem::GetTeamContract(int32 TeamID) const
{
	const FBBContract* Found = TeamContracts.Find(TeamID);
	return Found ? *Found : FBBContract();
}

void UBBContractSubsystem::ProgressContract(int32 TeamID, EBBContractType Type)
{
	FBBContract* Contract = TeamContracts.Find(TeamID);
	if (!Contract || Contract->bCompleted || Contract->ContractType != Type)
	{
		return;
	}

	++Contract->CurrentProgress;

	UE_LOG(LogBreachborne, Log, TEXT("Contracts: Team %d progress %d/%d"),
		TeamID, Contract->CurrentProgress, Contract->ObjectiveCount);

	if (Contract->IsComplete())
	{
		CompleteContract(TeamID, *Contract);
	}
}

void UBBContractSubsystem::CompleteContract(int32 TeamID, FBBContract& Contract)
{
	Contract.bCompleted = true;

	UE_LOG(LogBreachborne, Log, TEXT("Contracts: Team %d COMPLETED %s — paying %dg"),
		TeamID,
		Contract.ContractType == EBBContractType::Brawler ? TEXT("Brawler") : TEXT("Creep Farm"),
		Contract.GoldReward);

	PayOutReward(TeamID, Contract.GoldReward);
}

void UBBContractSubsystem::PayOutReward(int32 TeamID, int32 GoldAmount)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 PaidCount = 0;
	for (TActorIterator<ABreachbornePlayerState> It(World); It; ++It)
	{
		ABreachbornePlayerState* PS = *It;
		if (PS && PS->GetTeamID() == TeamID)
		{
			FBBInventoryManager::AddGold(PS, GoldAmount);
			++PaidCount;
		}
	}

	UE_LOG(LogBreachborne, Log, TEXT("Contracts: Paid %dg to %d members of team %d"), GoldAmount, PaidCount, TeamID);
}
