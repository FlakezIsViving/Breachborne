#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BBContractSubsystem.generated.h"

class ABreachbornePlayerState;
class ABBCreepCamp;

UENUM(BlueprintType)
enum class EBBContractType : uint8
{
	None        UMETA(DisplayName = "None"),
	Brawler     UMETA(DisplayName = "Brawler"),    // Kill X hunters
	CreepFarm   UMETA(DisplayName = "Creep Farm"), // Clear X creep camps
};

USTRUCT(BlueprintType)
struct FBBContract
{
	GENERATED_BODY()

	/** Type of this contract */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Contracts")
	EBBContractType ContractType = EBBContractType::None;

	/** Objective count (how many kills/camps needed to complete) */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Contracts")
	int32 ObjectiveCount = 3;

	/** Current progress toward ObjectiveCount */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Contracts")
	int32 CurrentProgress = 0;

	/** Gold reward on completion */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Contracts")
	int32 GoldReward = 75;

	/** Whether the contract has been completed */
	UPROPERTY(BlueprintReadOnly, Category = "Breachborne|Contracts")
	bool bCompleted = false;

	bool IsComplete() const { return CurrentProgress >= ObjectiveCount; }
};

/**
 * WorldSubsystem that issues and tracks contracts for all teams.
 *
 * - Lives on the server; no client-side logic (GameState replicates results)
 * - Two contract types: Brawler (kills) and Creep Farm (camp clears)
 * - Called from: GameMode.AwardKillRewards (kills) and ABBCreepCamp.OnCampCleared (camps)
 * - On completion: pays out GoldReward to all squad members via AddGold
 */
UCLASS()
class BREACHBORNE_API UBBContractSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/**
	 * Issue a random contract to the given team.
	 * Called by GameMode when the match starts or when the merchant first unlocks.
	 */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Contracts")
	void IssueContractToTeam(int32 TeamID);

	/** Report a hunter kill for a team (called from GameMode.AwardKillRewards) */
	void ReportKill(int32 TeamID);

	/** Report a camp clear for a team (called from ABBCreepCamp.OnCampCleared) */
	void ReportCampClear(int32 TeamID);

	/** Get the current contract for a team (-1 = no contract) */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Contracts")
	FBBContract GetTeamContract(int32 TeamID) const;

private:
	/** TeamID → active contract */
	TMap<int32, FBBContract> TeamContracts;

	void ProgressContract(int32 TeamID, EBBContractType Type);
	void CompleteContract(int32 TeamID, FBBContract& Contract);
	void PayOutReward(int32 TeamID, int32 GoldAmount);
};
