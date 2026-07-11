/**
 * BB_Spec_GameSystems.spec.cpp
 *
 * Automation spec tests that require a UWorld context. These tests use the
 * FAutomationEditorCommon helper to create a transient world and tear it down.
 *
 * They are tagged ProductFilter so they run in the standard "Breachborne" suite.
 * Because they create a world they are slower than the pure-logic specs —
 * run the PureLogic suite on every compile, this suite on every PR.
 *
 * Run:  -ExecCmds="Automation RunTests Breachborne.GameSystems"
 *
 * NOTE: tests that require actual PIE (possessed controllers, replication,
 *       input) are in the PIE checklist at docs/PIEChecklist.md — they cannot
 *       be automated without a multi-client setup.
 */

#if WITH_AUTOMATION_TESTS && WITH_EDITOR

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Components/BrushComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

// --- Systems under test ---
#include "Breachborne/Core/BreachborneGameState.h"
#include "Breachborne/Core/BreachborneTypes.h"
#include "Breachborne/PvE/BBContractSubsystem.h"
#include "Breachborne/Storm/BBStormManager.h"
#include "Breachborne/Storm/StormShift_Default.h"
#include "Breachborne/Items/BBChestActor.h"
#include "Breachborne/Items/BBChestLootTable.h"
#include "Breachborne/Items/BBWorldItem.h"
#include "Breachborne/Vision/BBBrushVolume.h"

// ============================================================================
// Helper — create a minimal transient world with a GameState
// ============================================================================
namespace BBTestHelpers
{
	static UWorld* CreateTestWorld(const FString& TestName)
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, *TestName);
		if (World)
		{
			FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
			WorldContext.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
		}
		return World;
	}

	static void DestroyTestWorld(UWorld* World)
	{
		if (!World) return;
		World->BeginTearingDown();
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		World = nullptr;
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}
}

// ============================================================================
// 1. ABreachborneGameState — Day/Night Phase
//    AdvanceDayNightPhase() must toggle the replicated enum and increment cycle.
// ============================================================================

DEFINE_SPEC(FBBGameStateDayNightSpec, "Breachborne.GameSystems.GameState.DayNight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FBBGameStateDayNightSpec::Define()
{
	UWorld* TestWorld = nullptr;
	ABreachborneGameState* GameState = nullptr;

	BeforeEach([this, &TestWorld, &GameState]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_DayNightTest"));
		if (TestWorld)
		{
			GameState = TestWorld->SpawnActor<ABreachborneGameState>();
		}
	});

	AfterEach([this, &TestWorld, &GameState]()
	{
		GameState = nullptr;
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	Describe("AdvanceDayNightPhase", [this, &GameState]()
	{
		It("starts in Day phase", [this, &GameState]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			TestEqual("Initial phase is Day",
				GameState->GetDayNightPhase(), EBBDayNightPhase::Day);
		});

		It("first call transitions to Night", [this, &GameState]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			GameState->AdvanceDayNightPhase();
			TestEqual("After 1 advance: Night",
				GameState->GetDayNightPhase(), EBBDayNightPhase::Night);
		});

		It("second call transitions back to Day", [this, &GameState]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			GameState->AdvanceDayNightPhase();
			GameState->AdvanceDayNightPhase();
			TestEqual("After 2 advances: Day",
				GameState->GetDayNightPhase(), EBBDayNightPhase::Day);
		});

		It("IsNight returns true after first advance", [this, &GameState]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			GameState->AdvanceDayNightPhase();
			TestTrue("IsNight() == true after advance", GameState->IsNight());
		});

		It("DayNightCycle increments on each advance", [this, &GameState]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			const int32 InitialCycle = GameState->GetDayNightCycle();
			GameState->AdvanceDayNightPhase();
			TestEqual("Cycle increments by 1",
				GameState->GetDayNightCycle(), InitialCycle + 1);
			GameState->AdvanceDayNightPhase();
			TestEqual("Cycle increments again",
				GameState->GetDayNightCycle(), InitialCycle + 2);
		});
	});
}

// ============================================================================
// 2. ABreachborneGameState — Match Phase
//    SetMatchPhase must update the replicated MatchPhase field.
// ============================================================================

DEFINE_SPEC(FBBGameStateMatchPhaseSpec, "Breachborne.GameSystems.GameState.MatchPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FBBGameStateMatchPhaseSpec::Define()
{
	UWorld* TestWorld = nullptr;
	ABreachborneGameState* GameState = nullptr;

	BeforeEach([this, &TestWorld, &GameState]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_MatchPhaseTest"));
		if (TestWorld)
		{
			GameState = TestWorld->SpawnActor<ABreachborneGameState>();
		}
	});

	AfterEach([this, &TestWorld, &GameState]()
	{
		GameState = nullptr;
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	Describe("SetMatchPhase", [this, &GameState]()
	{
		It("starts in WaitingForPlayers", [this, &GameState]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			TestEqual("Initial phase is WaitingForPlayers",
				GameState->GetMatchPhase(), EMatchPhase::WaitingForPlayers);
		});

		It("transitions to Dropping", [this, &GameState]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			GameState->SetMatchPhase(EMatchPhase::Dropping);
			TestEqual("Phase is Dropping",
				GameState->GetMatchPhase(), EMatchPhase::Dropping);
		});

		It("transitions to Playing", [this, &GameState]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			GameState->SetMatchPhase(EMatchPhase::Playing);
			TestEqual("Phase is Playing",
				GameState->GetMatchPhase(), EMatchPhase::Playing);
		});

		It("transitions to Ended", [this, &GameState]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			GameState->SetMatchPhase(EMatchPhase::Ended);
			TestEqual("Phase is Ended",
				GameState->GetMatchPhase(), EMatchPhase::Ended);
		});

		It("ElapsedMatchTime starts at zero", [this, &GameState]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			TestEqual("Elapsed time zero", GameState->GetElapsedMatchTime(), 0.f);
		});
	});
}

// ============================================================================
// 3. UBBContractSubsystem — Subsystem Existence and Contract Issuance
//    ShouldCreateSubsystem returns false on clients. In editor/game world
//    it should be present (NetMode is NM_Standalone or NM_DedicatedServer).
// ============================================================================

DEFINE_SPEC(FBBContractSubsystemSpec, "Breachborne.GameSystems.ContractSubsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FBBContractSubsystemSpec::Define()
{
	UWorld* TestWorld = nullptr;

	BeforeEach([this, &TestWorld]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_ContractTest"));
	});

	AfterEach([this, &TestWorld]()
	{
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	Describe("Subsystem availability", [this, &TestWorld]()
	{
		It("subsystem exists in a standalone world", [this, &TestWorld]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			UBBContractSubsystem* Contracts =
				TestWorld->GetSubsystem<UBBContractSubsystem>();
			TestNotNull("ContractSubsystem not null in standalone world", Contracts);
		});
	});

	Describe("IssueContractToTeam", [this, &TestWorld]()
	{
		It("contract is None before issuance", [this, &TestWorld]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			UBBContractSubsystem* Contracts =
				TestWorld->GetSubsystem<UBBContractSubsystem>();
			if (!TestNotNull("Subsystem valid", Contracts)) return;

			const FBBContract C = Contracts->GetTeamContract(0);
			TestEqual("No contract before issue", C.ContractType, EBBContractType::None);
		});

		It("IssueContractToTeam assigns Brawler or CreepFarm (not None)", [this, &TestWorld]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			UBBContractSubsystem* Contracts =
				TestWorld->GetSubsystem<UBBContractSubsystem>();
			if (!TestNotNull("Subsystem valid", Contracts)) return;

			Contracts->IssueContractToTeam(0);
			const FBBContract C = Contracts->GetTeamContract(0);
			TestNotEqual("Contract type is assigned", C.ContractType, EBBContractType::None);
		});

		It("issued contract has positive ObjectiveCount", [this, &TestWorld]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			UBBContractSubsystem* Contracts =
				TestWorld->GetSubsystem<UBBContractSubsystem>();
			if (!TestNotNull("Subsystem valid", Contracts)) return;

			Contracts->IssueContractToTeam(1);
			const FBBContract C = Contracts->GetTeamContract(1);
			TestTrue("ObjectiveCount > 0", C.ObjectiveCount > 0);
		});

		It("issued contract has positive GoldReward", [this, &TestWorld]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			UBBContractSubsystem* Contracts =
				TestWorld->GetSubsystem<UBBContractSubsystem>();
			if (!TestNotNull("Subsystem valid", Contracts)) return;

			Contracts->IssueContractToTeam(2);
			const FBBContract C = Contracts->GetTeamContract(2);
			TestTrue("GoldReward > 0", C.GoldReward > 0);
		});

		It("different teams get independent contracts", [this, &TestWorld]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			UBBContractSubsystem* Contracts =
				TestWorld->GetSubsystem<UBBContractSubsystem>();
			if (!TestNotNull("Subsystem valid", Contracts)) return;

			// Team 10 gets a contract; Team 11 has none yet
			Contracts->IssueContractToTeam(10);
			const FBBContract C10 = Contracts->GetTeamContract(10);
			const FBBContract C11 = Contracts->GetTeamContract(11);

			TestNotEqual("Team 10 has a contract", C10.ContractType, EBBContractType::None);
			TestEqual("Team 11 still has no contract", C11.ContractType, EBBContractType::None);
		});
	});

	Describe("ReportKill on Brawler contract", [this, &TestWorld]()
	{
		It("one kill increments Brawler progress by 1", [this, &TestWorld]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			UBBContractSubsystem* Contracts =
				TestWorld->GetSubsystem<UBBContractSubsystem>();
			if (!TestNotNull("Subsystem valid", Contracts)) return;

			// Issue contracts until we get a Brawler (random; seed with a few teams)
			int32 BrawlerTeam = -1;
			for (int32 i = 20; i < 30; ++i)
			{
				Contracts->IssueContractToTeam(i);
				if (Contracts->GetTeamContract(i).ContractType == EBBContractType::Brawler)
				{
					BrawlerTeam = i;
					break;
				}
			}

			// If we didn't land a Brawler in 10 tries, skip gracefully (not a test failure)
			if (BrawlerTeam == -1)
			{
				AddWarning("Could not obtain a Brawler contract in 10 random draws — skipped.");
				return;
			}

			const int32 Before = Contracts->GetTeamContract(BrawlerTeam).CurrentProgress;
			Contracts->ReportKill(BrawlerTeam);
			const int32 After = Contracts->GetTeamContract(BrawlerTeam).CurrentProgress;
			TestEqual("Kill increments Brawler progress", After, Before + 1);
		});
	});
}

// ============================================================================
// 4. ABBBrushVolume — Spawning Without Crash
//    The volume is server-side only (bReplicates=false). Verify it spawns
//    in a test world without crashing and has collision enabled.
// ============================================================================

DEFINE_SPEC(FBBBrushVolumeSpec, "Breachborne.GameSystems.BrushVolume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FBBBrushVolumeSpec::Define()
{
	UWorld* TestWorld = nullptr;

	BeforeEach([this, &TestWorld]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_BrushVolumeTest"));
	});

	AfterEach([this, &TestWorld]()
	{
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	Describe("ABBBrushVolume", [this, &TestWorld]()
	{
		It("spawns without crash", [this, &TestWorld]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			ABBBrushVolume* Volume = TestWorld->SpawnActor<ABBBrushVolume>();
			TestNotNull("BrushVolume spawned", Volume);
		});

		It("does not replicate  [server-only actor]", [this, &TestWorld]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			ABBBrushVolume* Volume = TestWorld->SpawnActor<ABBBrushVolume>();
			if (!TestNotNull("Volume spawned", Volume)) return;
			TestFalse("bReplicates is false", Volume->GetIsReplicated());
		});

		It("brush component generates overlap events", [this, &TestWorld]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			ABBBrushVolume* Volume = TestWorld->SpawnActor<ABBBrushVolume>();
			if (!TestNotNull("Volume spawned", Volume)) return;
			if (!TestNotNull("BrushComponent valid", Volume->GetBrushComponent())) return;
			TestTrue("GenerateOverlapEvents enabled",
				Volume->GetBrushComponent()->GetGenerateOverlapEvents());
		});
	});
}

// ============================================================================
// 5. ABBChestActor / UBBChestLootTable
//    Chests should spawn as replicated world actors and their fallback loot path
//    should produce independent ABBWorldItem pickups.
// ============================================================================

DEFINE_SPEC(FBBChestLootSpec, "Breachborne.GameSystems.ChestLoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FBBChestLootSpec::Define()
{
	UWorld* TestWorld = nullptr;

	BeforeEach([this, &TestWorld]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_ChestLootTest"));
	});

	AfterEach([this, &TestWorld]()
	{
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	Describe("ABBChestActor", [this, &TestWorld]()
	{
		It("spawns replicated and starts closed", [this, &TestWorld]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			ABBChestActor* Chest = TestWorld->SpawnActor<ABBChestActor>();
			if (!TestNotNull("Chest spawned", Chest)) return;
			TestTrue("Chest replicates", Chest->GetIsReplicated());
			TestEqual("Initial state is Closed", Chest->GetChestState(), EBBChestState::Closed);
			TestEqual("Initial progress is zero", Chest->GetOpenProgress(), 0.0f);
		});
	});

	Describe("UBBChestLootTable fallback", [this, &TestWorld]()
	{
		It("spawns loose world item pickups", [this, &TestWorld]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			const int32 SpawnedCount = UBBChestLootTable::SpawnFallbackLoot(TestWorld, FVector::ZeroVector, EBBChestTier::Common);
			TestTrue("Spawned at least one pickup", SpawnedCount > 0);

			int32 WorldItemCount = 0;
			for (TActorIterator<ABBWorldItem> It(TestWorld); It; ++It)
			{
				WorldItemCount++;
			}
			TestEqual("Spawn count matches world item count", WorldItemCount, SpawnedCount);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS && WITH_EDITOR
