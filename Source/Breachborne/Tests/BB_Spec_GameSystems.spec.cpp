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
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Misc/AutomationTest.h"
#include "Components/BrushComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

// --- Systems under test ---
#include "Breachborne/Core/BreachborneGameState.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Core/BreachborneTypes.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/Hunters/Crysta/GA_Crysta_LMB.h"
#include "Breachborne/Abilities/Hunters/Crysta/GA_Crysta_Passive.h"
#include "Breachborne/Abilities/Hunters/Crysta/GA_Crysta_R.h"
#include "Breachborne/Abilities/Hunters/Crysta/GA_Crysta_Shift.h"
#include "Breachborne/Abilities/Hunters/Eluna/GA_Eluna_Shift.h"
#include "Breachborne/Abilities/Hunters/Kingpin/GA_Kingpin_AerialDash.h"
#include "Breachborne/Abilities/Hunters/Kingpin/GA_Kingpin_GroundDash.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/PvE/BBContractSubsystem.h"
#include "Breachborne/Storm/BBStormManager.h"
#include "Breachborne/Storm/StormShift_Default.h"
#include "Breachborne/Items/BBChestActor.h"
#include "Breachborne/Items/BBChestLootTable.h"
#include "Breachborne/Items/BBWorldItem.h"
#include "Breachborne/Vision/BBBrushVolume.h"
#include "Breachborne/Combat/BBCrystaBurstZone.h"
#include "Breachborne/Combat/BBCrystaFluxSnareProjectile.h"
#include "Breachborne/Combat/BBCrystaLMBProjectile.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBMoonlightZone.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Combat/BBPrimitiveWedgeActor.h"
#include "Breachborne/Combat/BBTestAlly.h"
#include "Breachborne/Combat/BBTargetDummy.h"
#include "Breachborne/Combat/BBVoidAnomalyPuddle.h"
#include "Breachborne/Combat/BBVoidOrbProjectile.h"
#include "Breachborne/Combat/BBVoidSingularityActor.h"
#include "Breachborne/Combat/BBVoidSnapProjectile.h"
#include "Breachborne/Combat/BBVoidSwapProjectile.h"
#include "Breachborne/Combat/BBVoidSwappableComponent.h"
#include "Breachborne/Combat/BBReverberationMarkEffect.h"
#include "Breachborne/Wisp/BBWispPawn.h"

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

	static UStaticMeshComponent* FindMesh(AActor* Actor, const FName ComponentName)
	{
		if (!Actor)
		{
			return nullptr;
		}
		TArray<UStaticMeshComponent*> Meshes;
		Actor->GetComponents<UStaticMeshComponent>(Meshes);
		for (UStaticMeshComponent* Mesh : Meshes)
		{
			if (Mesh && Mesh->GetFName() == ComponentName)
			{
				return Mesh;
			}
		}
		return nullptr;
	}
}

// ============================================================================
// 1. ABreachborneGameState — Day/Night Phase
//    AdvanceDayNightPhase() must toggle the replicated enum and increment cycle.
// ============================================================================

BEGIN_DEFINE_SPEC(FBBGameStateDayNightSpec, "Breachborne.GameSystems.GameState.DayNight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UWorld* TestWorld = nullptr;
	ABreachborneGameState* GameState = nullptr;
END_DEFINE_SPEC(FBBGameStateDayNightSpec)

void FBBGameStateDayNightSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_DayNightTest"));
		if (TestWorld)
		{
			GameState = TestWorld->SpawnActor<ABreachborneGameState>();
		}
	});

	AfterEach([this]()
	{
		GameState = nullptr;
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	Describe("AdvanceDayNightPhase", [this]()
	{
		It("starts in Day phase", [this]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			TestEqual("Initial phase is Day",
				GameState->GetDayNightPhase(), EBBDayNightPhase::Day);
		});

		It("first call transitions to Night", [this]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			GameState->AdvanceDayNightPhase();
			TestEqual("After 1 advance: Night",
				GameState->GetDayNightPhase(), EBBDayNightPhase::Night);
		});

		It("second call transitions back to Day", [this]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			GameState->AdvanceDayNightPhase();
			GameState->AdvanceDayNightPhase();
			TestEqual("After 2 advances: Day",
				GameState->GetDayNightPhase(), EBBDayNightPhase::Day);
		});

		It("IsNight returns true after first advance", [this]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			GameState->AdvanceDayNightPhase();
			TestTrue("IsNight() == true after advance", GameState->IsNight());
		});

		It("DayNightCycle increments on each advance", [this]()
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

BEGIN_DEFINE_SPEC(FBBGameStateMatchPhaseSpec, "Breachborne.GameSystems.GameState.MatchPhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UWorld* TestWorld = nullptr;
	ABreachborneGameState* GameState = nullptr;
END_DEFINE_SPEC(FBBGameStateMatchPhaseSpec)

void FBBGameStateMatchPhaseSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_MatchPhaseTest"));
		if (TestWorld)
		{
			GameState = TestWorld->SpawnActor<ABreachborneGameState>();
		}
	});

	AfterEach([this]()
	{
		GameState = nullptr;
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	Describe("SetMatchPhase", [this]()
	{
		It("starts in WaitingForPlayers", [this]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			TestEqual("Initial phase is WaitingForPlayers",
				GameState->GetMatchPhase(), EMatchPhase::WaitingForPlayers);
		});

		It("transitions to Dropping", [this]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			GameState->SetMatchPhase(EMatchPhase::Dropping);
			TestEqual("Phase is Dropping",
				GameState->GetMatchPhase(), EMatchPhase::Dropping);
		});

		It("transitions to Playing", [this]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			GameState->SetMatchPhase(EMatchPhase::Playing);
			TestEqual("Phase is Playing",
				GameState->GetMatchPhase(), EMatchPhase::Playing);
		});

		It("transitions to Ended", [this]()
		{
			if (!TestNotNull("GameState valid", GameState)) return;
			GameState->SetMatchPhase(EMatchPhase::Ended);
			TestEqual("Phase is Ended",
				GameState->GetMatchPhase(), EMatchPhase::Ended);
		});

		It("ElapsedMatchTime starts at zero", [this]()
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

BEGIN_DEFINE_SPEC(FBBStormControlSpec, "Breachborne.GameSystems.Storm.Control",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UWorld* TestWorld = nullptr;
	ABBStormManager* StormManager = nullptr;
END_DEFINE_SPEC(FBBStormControlSpec)

void FBBStormControlSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_StormControlTest"));
		if (TestWorld)
		{
			StormManager = TestWorld->SpawnActor<ABBStormManager>();
		}
	});

	AfterEach([this]()
	{
		StormManager = nullptr;
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	It("defaults lobby storms on", [this]()
	{
		const FBBLobbySettings Settings;
		TestTrue("Storm defaults enabled", Settings.bStormEnabled);
	});

	It("suspends and re-enables the storm manager", [this]()
	{
		if (!TestNotNull("Storm manager valid", StormManager)) return;
		StormManager->SetStormEnabled(false);
		TestFalse("Storm manager disabled", StormManager->IsStormEnabled());
		StormManager->SetStormEnabled(true);
		TestTrue("Storm manager re-enabled", StormManager->IsStormEnabled());
	});
}

BEGIN_DEFINE_SPEC(FBBMatchAbilityResetSpec, "Breachborne.GameSystems.Match.AbilityReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UWorld* TestWorld = nullptr;
	ABreachbornePlayerState* PlayerState = nullptr;
END_DEFINE_SPEC(FBBMatchAbilityResetSpec)

void FBBMatchAbilityResetSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_MatchAbilityResetTest"));
		if (TestWorld)
		{
			PlayerState = TestWorld->SpawnActor<ABreachbornePlayerState>();
		}
	});

	AfterEach([this]()
	{
		PlayerState = nullptr;
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	It("clears stale life state, cooldowns, and effects before a rematch", [this]()
	{
		if (!TestNotNull("PlayerState valid", PlayerState)) return;
		UBBAbilitySystemComponent* ASC = PlayerState->GetBBAbilitySystemComponent();
		if (!TestNotNull("AbilitySystem valid", ASC)) return;

		ASC->AddLooseGameplayTag(BBGameplayTags::State_Dead);
		ASC->AddLooseGameplayTag(BBGameplayTags::State_Wisp);
		ASC->AddLooseGameplayTag(BBGameplayTags::State_Hudson_Firing);
		ASC->AddLooseGameplayTag(BBGameplayTags::Cooldown_Hunter_Ghost_Shift);

		UGameplayEffect* MatchEffect = NewObject<UGameplayEffect>(PlayerState);
		MatchEffect->DurationPolicy = EGameplayEffectDurationType::Infinite;
		const FGameplayEffectSpec EffectSpec(MatchEffect, ASC->MakeEffectContext(), 1.0f);
		const FActiveGameplayEffectHandle EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(EffectSpec);
		TestTrue("Match effect applied", EffectHandle.IsValid());

		ASC->ResetForNewMatch();

		TestFalse("Dead state cleared", ASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead));
		TestFalse("Wisp state cleared", ASC->HasMatchingGameplayTag(BBGameplayTags::State_Wisp));
		TestFalse("Hunter combat state cleared", ASC->HasMatchingGameplayTag(BBGameplayTags::State_Hudson_Firing));
		TestFalse("Loose cooldown cleared", ASC->HasMatchingGameplayTag(BBGameplayTags::Cooldown_Hunter_Ghost_Shift));
		TestEqual("Active match effects cleared", ASC->GetActiveEffects(FGameplayEffectQuery()).Num(), 0);
	});
}

BEGIN_DEFINE_SPEC(FBBContractSubsystemSpec, "Breachborne.GameSystems.ContractSubsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UWorld* TestWorld = nullptr;
END_DEFINE_SPEC(FBBContractSubsystemSpec)

void FBBContractSubsystemSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_ContractTest"));
	});

	AfterEach([this]()
	{
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	Describe("Subsystem availability", [this]()
	{
		It("subsystem exists in a standalone world", [this]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			UBBContractSubsystem* Contracts =
				TestWorld->GetSubsystem<UBBContractSubsystem>();
			TestNotNull("ContractSubsystem not null in standalone world", Contracts);
		});
	});

	Describe("IssueContractToTeam", [this]()
	{
		It("contract is None before issuance", [this]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			UBBContractSubsystem* Contracts =
				TestWorld->GetSubsystem<UBBContractSubsystem>();
			if (!TestNotNull("Subsystem valid", Contracts)) return;

			const FBBContract C = Contracts->GetTeamContract(0);
			TestEqual("No contract before issue", C.ContractType, EBBContractType::None);
		});

		It("IssueContractToTeam assigns Brawler or CreepFarm (not None)", [this]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			UBBContractSubsystem* Contracts =
				TestWorld->GetSubsystem<UBBContractSubsystem>();
			if (!TestNotNull("Subsystem valid", Contracts)) return;

			Contracts->IssueContractToTeam(0);
			const FBBContract C = Contracts->GetTeamContract(0);
			TestNotEqual("Contract type is assigned", C.ContractType, EBBContractType::None);
		});

		It("issued contract has positive ObjectiveCount", [this]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			UBBContractSubsystem* Contracts =
				TestWorld->GetSubsystem<UBBContractSubsystem>();
			if (!TestNotNull("Subsystem valid", Contracts)) return;

			Contracts->IssueContractToTeam(1);
			const FBBContract C = Contracts->GetTeamContract(1);
			TestTrue("ObjectiveCount > 0", C.ObjectiveCount > 0);
		});

		It("issued contract has positive GoldReward", [this]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			UBBContractSubsystem* Contracts =
				TestWorld->GetSubsystem<UBBContractSubsystem>();
			if (!TestNotNull("Subsystem valid", Contracts)) return;

			Contracts->IssueContractToTeam(2);
			const FBBContract C = Contracts->GetTeamContract(2);
			TestTrue("GoldReward > 0", C.GoldReward > 0);
		});

		It("different teams get independent contracts", [this]()
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

	Describe("ReportKill on Brawler contract", [this]()
	{
		It("one kill increments Brawler progress by 1", [this]()
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

BEGIN_DEFINE_SPEC(FBBBrushVolumeSpec, "Breachborne.GameSystems.BrushVolume",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UWorld* TestWorld = nullptr;
END_DEFINE_SPEC(FBBBrushVolumeSpec)

void FBBBrushVolumeSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_BrushVolumeTest"));
	});

	AfterEach([this]()
	{
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	Describe("ABBBrushVolume", [this]()
	{
		It("spawns without crash", [this]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			ABBBrushVolume* Volume = TestWorld->SpawnActor<ABBBrushVolume>();
			TestNotNull("BrushVolume spawned", Volume);
		});

		It("does not replicate  [server-only actor]", [this]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			ABBBrushVolume* Volume = TestWorld->SpawnActor<ABBBrushVolume>();
			if (!TestNotNull("Volume spawned", Volume)) return;
			TestFalse("bReplicates is false", Volume->GetIsReplicated());
		});

		It("brush component generates overlap events", [this]()
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

BEGIN_DEFINE_SPEC(FBBChestLootSpec, "Breachborne.GameSystems.ChestLoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UWorld* TestWorld = nullptr;
END_DEFINE_SPEC(FBBChestLootSpec)

void FBBChestLootSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_ChestLootTest"));
	});

	AfterEach([this]()
	{
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	Describe("ABBChestActor", [this]()
	{
		It("spawns replicated and starts closed", [this]()
		{
			if (!TestNotNull("World valid", TestWorld)) return;
			ABBChestActor* Chest = TestWorld->SpawnActor<ABBChestActor>();
			if (!TestNotNull("Chest spawned", Chest)) return;
			TestTrue("Chest replicates", Chest->GetIsReplicated());
			TestEqual("Initial state is Closed", Chest->GetChestState(), EBBChestState::Closed);
			TestEqual("Initial progress is zero", Chest->GetOpenProgress(), 0.0f);
		});
	});

	Describe("UBBChestLootTable fallback", [this]()
	{
		It("spawns loose world item pickups", [this]()
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

// ============================================================================
// 6. Primitive VFX geometry
//    Objective fallback geometry must match gameplay radii and empowered state.
// ============================================================================

BEGIN_DEFINE_SPEC(FBBPrimitiveVFXSpec, "Breachborne.GameSystems.PrimitiveVFX",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UWorld* TestWorld = nullptr;
END_DEFINE_SPEC(FBBPrimitiveVFXSpec)

void FBBPrimitiveVFXSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld = BBTestHelpers::CreateTestWorld(TEXT("BB_PrimitiveVFXTest"));
	});

	AfterEach([this]()
	{
		BBTestHelpers::DestroyTestWorld(TestWorld);
		TestWorld = nullptr;
	});

	It("sizes Crysta Q disc to the gameplay radius", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		ABBCrystaBurstZone* Zone = TestWorld->SpawnActor<ABBCrystaBurstZone>();
		if (!TestNotNull("Crysta zone spawned", Zone)) return;
		Zone->InitBurstZone(nullptr, nullptr, nullptr, nullptr, 0, 400.0f, 60.0f, 0.0f);
		UStaticMeshComponent* Mesh = BBTestHelpers::FindMesh(Zone, TEXT("ZoneMesh"));
		if (!TestNotNull("Zone mesh valid", Mesh)) return;
		TestTrue("400 radius uses 8x basic cylinder scale", FMath::IsNearlyEqual(Mesh->GetRelativeScale3D().X, 8.0f));
	});

	It("attaches Eluna Q to the first allied wisp instead of its dead hunter body", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;

		AHunterCharacter* Eluna = TestWorld->SpawnActor<AHunterCharacter>(
			FVector(0.0f, 0.0f, 200.0f), FRotator::ZeroRotator);
		ABreachbornePlayerState* ElunaPS = TestWorld->SpawnActor<ABreachbornePlayerState>();
		AHunterCharacter* DeadHunter = TestWorld->SpawnActor<AHunterCharacter>(
			FVector(250.0f, 0.0f, 200.0f), FRotator::ZeroRotator);
		ABreachbornePlayerState* DeadPS = TestWorld->SpawnActor<ABreachbornePlayerState>();
		ABBWispPawn* Wisp = TestWorld->SpawnActor<ABBWispPawn>(
			FVector(350.0f, 0.0f, 200.0f), FRotator::ZeroRotator);
		ABBMoonlightZone* Zone = TestWorld->SpawnActor<ABBMoonlightZone>(
			FVector(0.0f, 0.0f, 200.0f), FRotator::ZeroRotator);
		if (!TestNotNull("Eluna spawned", Eluna) || !TestNotNull("Eluna PS spawned", ElunaPS)
			|| !TestNotNull("Dead hunter spawned", DeadHunter) || !TestNotNull("Dead PS spawned", DeadPS)
			|| !TestNotNull("Wisp spawned", Wisp) || !TestNotNull("Zone spawned", Zone)) return;

		ElunaPS->SetTeamID(0);
		ElunaPS->SetIsAlive(true);
		Eluna->SetPlayerState(ElunaPS);
		DeadPS->SetTeamID(0);
		DeadPS->SetIsAlive(false);
		DeadHunter->SetPlayerState(DeadPS);
		Wisp->InitWisp(DeadPS);

		Zone->InitZone(Eluna, 12.0f, 0.5f, 400.0f, 2.5f, 0.5f, 5.0f, 90.0f);
		Zone->TossToLocation(FVector(1000.0f, 0.0f, 200.0f));
		Zone->Tick(0.3f);

		TestEqual("Q links to allied wisp", Zone->GetAttachedTarget(), static_cast<AActor*>(Wisp));
		TestFalse("Q stops traveling after wisp link", Zone->IsTraveling());
	});

	It("keeps Eluna Q on its carrier and drops it when that hunter dies", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;

		AHunterCharacter* Eluna = TestWorld->SpawnActor<AHunterCharacter>(
			FVector(0.0f, 0.0f, 200.0f), FRotator::ZeroRotator);
		ABreachbornePlayerState* ElunaPS = TestWorld->SpawnActor<ABreachbornePlayerState>();
		ABBMoonlightZone* Zone = TestWorld->SpawnActor<ABBMoonlightZone>(
			FVector(0.0f, 0.0f, 200.0f), FRotator::ZeroRotator);
		if (!TestNotNull("Eluna spawned", Eluna) || !TestNotNull("Eluna PS spawned", ElunaPS)
			|| !TestNotNull("Zone spawned", Zone)) return;

		ElunaPS->SetTeamID(0);
		ElunaPS->SetIsAlive(true);
		Eluna->SetPlayerState(ElunaPS);
		Zone->InitZone(Eluna, 12.0f, 0.5f, 400.0f, 2.5f, 0.5f, 5.0f, 90.0f);

		Eluna->SetActorLocation(FVector(300.0f, 120.0f, 200.0f));
		Zone->Tick(0.1f);
		TestTrue("Initial Q follows Eluna", Zone->GetActorLocation().Equals(Eluna->GetActorLocation(), 1.0f));

		const FVector DropLocation = Zone->GetActorLocation();
		ElunaPS->SetIsAlive(false);
		Eluna->SetActorLocation(FVector(900.0f, 120.0f, 200.0f));
		Zone->Tick(0.1f);
		TestNull("Dead carrier is detached", Zone->GetAttachedTarget());
		TestTrue("Q remains where its carrier died", Zone->GetActorLocation().Equals(DropLocation, 1.0f));
	});

	It("sizes Void Q disc to the gameplay radius", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		ABBVoidAnomalyPuddle* Puddle = TestWorld->SpawnActor<ABBVoidAnomalyPuddle>();
		if (!TestNotNull("Void puddle spawned", Puddle)) return;
		Puddle->InitPuddle(nullptr, nullptr, nullptr, 0, 460.0f, 60.0f, 0.0f, 0.0f);
		UStaticMeshComponent* Mesh = BBTestHelpers::FindMesh(Puddle, TEXT("PuddleMesh"));
		if (!TestNotNull("Puddle mesh valid", Mesh)) return;
		TestTrue("460 radius uses 9.2x basic cylinder scale", FMath::IsNearlyEqual(Mesh->GetRelativeScale3D().X, 9.2f));
	});

	It("shows empowered Void state on replicated primitive actors", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		ABBVoidOrbProjectile* Orb = TestWorld->SpawnActor<ABBVoidOrbProjectile>();
		ABBVoidSnapProjectile* NormalSnap = TestWorld->SpawnActor<ABBVoidSnapProjectile>();
		ABBVoidSnapProjectile* Snap = TestWorld->SpawnActor<ABBVoidSnapProjectile>();
		ABBVoidSwapProjectile* Swap = TestWorld->SpawnActor<ABBVoidSwapProjectile>();
		if (!TestNotNull("Orb spawned", Orb) || !TestNotNull("Normal snap spawned", NormalSnap)
			|| !TestNotNull("Snap spawned", Snap) || !TestNotNull("Swap spawned", Swap)) return;
		Orb->InitVoidOrb(nullptr, nullptr, nullptr, 0.0f, 0, true);
		NormalSnap->InitSnap(nullptr, nullptr, nullptr, nullptr, 0, FVector::RightVector, false);
		Snap->InitSnap(nullptr, nullptr, nullptr, nullptr, 0, FVector::RightVector, true);
		Swap->InitSwapProjectile(nullptr, FVector::ZeroVector, FVector(1000.0f, 0.0f, 0.0f), 60.0f, 500.0f, true);
		UStaticMeshComponent* OrbMesh = BBTestHelpers::FindMesh(Orb, TEXT("ProjectileMesh"));
		UStaticMeshComponent* ConeMesh = BBTestHelpers::FindMesh(Snap, TEXT("ConeMesh"));
		UStaticMeshComponent* ZoneMesh = BBTestHelpers::FindMesh(Swap, TEXT("ZoneMesh"));
		if (!TestNotNull("Orb mesh valid", OrbMesh) || !TestNotNull("Cone mesh valid", ConeMesh) || !TestNotNull("Swap zone mesh valid", ZoneMesh)) return;
		TestTrue("Charged orb is enlarged", FMath::IsNearlyEqual(OrbMesh->GetRelativeScale3D().X, 0.24f));
		TestTrue("Empowered cone is enlarged", FMath::IsNearlyEqual(ConeMesh->GetRelativeScale3D().X, 1.9f));
		TestTrue("Cone tip faces travel direction",
			FVector::DotProduct(ConeMesh->GetUpVector().GetSafeNormal(), FVector::RightVector) > 0.99f);
		TestTrue("Normal cone accepts a forward point", NormalSnap->IsLocationInsideCone(FVector(0.0f, 300.0f, 0.0f)));
		TestFalse("Normal cone rejects a point behind it", NormalSnap->IsLocationInsideCone(FVector(0.0f, -50.0f, 0.0f)));
		TestFalse("Normal cone rejects a point beyond its tip", NormalSnap->IsLocationInsideCone(FVector(0.0f, 400.0f, 0.0f)));
		TestTrue("Empowered cone reaches the farther point", Snap->IsLocationInsideCone(FVector(0.0f, 400.0f, 0.0f)));
		TestTrue("Empowered cone has greater gameplay length", Snap->GetConeLength() > NormalSnap->GetConeLength());
		TestTrue("Empowered cone has greater gameplay radius", Snap->GetConeRadius() > NormalSnap->GetConeRadius());
		TestTrue("Empowered cone has greater configured speed", Snap->GetTravelSpeed() > NormalSnap->GetTravelSpeed());
		NormalSnap->Tick(0.1f);
		Snap->Tick(0.1f);
		TestTrue("Empowered cone travels faster", Snap->GetActorLocation().Y > NormalSnap->GetActorLocation().Y);
		TestTrue("Empowered swap radius is exact", FMath::IsNearlyEqual(ZoneMesh->GetRelativeScale3D().X, 10.0f));
	});

	It("keeps charged Void orbs wall-piercing but pawn-sensitive", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		ABBVoidOrbProjectile* Orb = TestWorld->SpawnActor<ABBVoidOrbProjectile>();
		if (!TestNotNull("Orb spawned", Orb)) return;
		Orb->InitVoidOrb(nullptr, nullptr, nullptr, 0.0f, 0, true);
		USphereComponent* Collision = Orb->FindComponentByClass<USphereComponent>();
		if (!TestNotNull("Orb collision valid", Collision)) return;
		TestEqual("Charged orb ignores world static", Collision->GetCollisionResponseToChannel(ECC_WorldStatic), ECR_Ignore);
		TestEqual("Charged orb ignores world dynamic", Collision->GetCollisionResponseToChannel(ECC_WorldDynamic), ECR_Ignore);
		TestEqual("Charged orb still overlaps pawns", Collision->GetCollisionResponseToChannel(ECC_Pawn), ECR_Overlap);
	});

	It("extends Crysta R lanes past close convergence into an X", [this]()
	{
		TArray<FVector> CloseStarts;
		TArray<FVector> CloseEnds;
		UGA_Crysta_R::CalculateLaneSegments(FVector::ZeroVector, FVector::ForwardVector,
			FVector(800.0f, 0.0f, 0.0f), 2300.0f, 190.0f, 115.0f, CloseStarts, CloseEnds);
		TestEqual("Close pattern has two lanes", CloseStarts.Num(), 2);
		TestEqual("Close pattern has two endpoints", CloseEnds.Num(), 2);
		if (CloseStarts.Num() != 2 || CloseEnds.Num() != 2) return;

		const FVector CloseConvergence(800.0f, 0.0f, 95.0f);
		for (int32 Lane = 0; Lane < 2; ++Lane)
		{
			const FVector LaneDirection = (CloseConvergence - CloseStarts[Lane]).GetSafeNormal2D();
			TestTrue("Lane passes through convergence",
				FMath::PointDistToLine(CloseConvergence, CloseEnds[Lane] - CloseStarts[Lane], CloseStarts[Lane]) < 0.1f);
			TestTrue("Lane extends by remaining range",
				FMath::IsNearlyEqual(FVector::Dist2D(CloseConvergence, CloseEnds[Lane]), 1500.0f, 0.1f));
			TestTrue("Endpoint remains beyond convergence",
				FVector::DotProduct(CloseEnds[Lane] - CloseConvergence, LaneDirection) > 0.0f);
			TestTrue("Close lane crosses to the opposite side",
				FMath::Sign(CloseStarts[Lane].Y) == -FMath::Sign(CloseEnds[Lane].Y));
		}

		TArray<FVector> FarStarts;
		TArray<FVector> FarEnds;
		UGA_Crysta_R::CalculateLaneSegments(FVector::ZeroVector, FVector::ForwardVector,
			FVector(2200.0f, 0.0f, 0.0f), 2300.0f, 190.0f, 115.0f, FarStarts, FarEnds);
		TestEqual("Far pattern has two lanes", FarEnds.Num(), 2);
		if (FarEnds.Num() != 2) return;
		const FVector FarConvergence(2200.0f, 0.0f, 95.0f);
		TestTrue("Far pattern has only the remaining 100-unit overrun",
			FMath::IsNearlyEqual(FVector::Dist2D(FarConvergence, FarEnds[0]), 100.0f, 0.1f));
		TestTrue("Close X crosses farther than far V",
			FMath::Abs(CloseEnds[0].Y) > FMath::Abs(FarEnds[0].Y));
	});

	It("keeps multi-charge dash cooldown queries independent", [this]()
	{
		auto VerifyIndependentTag = [this](const TCHAR* Label, const FGameplayTagContainer* Tags,
			const FGameplayTag& ExpectedTag)
		{
			if (!TestNotNull(Label, Tags)) return;
			TestTrue(FString::Printf(TEXT("%s has its own cooldown"), Label), Tags->HasTagExact(ExpectedTag));
			TestFalse(FString::Printf(TEXT("%s does not query shared dash cooldown"), Label),
				Tags->HasTagExact(BBGameplayTags::Cooldown_Dash));
		};

		VerifyIndependentTag(TEXT("Crysta primary"), GetDefault<UGA_Crysta_Shift_Primary>()->GetCooldownTags(),
			BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Primary);
		VerifyIndependentTag(TEXT("Crysta secondary"), GetDefault<UGA_Crysta_Shift_Secondary>()->GetCooldownTags(),
			BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Secondary);
		VerifyIndependentTag(TEXT("Eluna ground"), GetDefault<UGA_Eluna_GroundDash>()->GetCooldownTags(),
			BBGameplayTags::Cooldown_Hunter_Eluna_GroundDash);
		VerifyIndependentTag(TEXT("Eluna aerial"), GetDefault<UGA_Eluna_AerialDash>()->GetCooldownTags(),
			BBGameplayTags::Cooldown_Hunter_Eluna_AerialDash);
		VerifyIndependentTag(TEXT("Kingpin ground"), GetDefault<UGA_Kingpin_GroundDash>()->GetCooldownTags(),
			BBGameplayTags::Cooldown_Hunter_Kingpin_GroundDash);
		VerifyIndependentTag(TEXT("Kingpin aerial"), GetDefault<UGA_Kingpin_AerialDash>()->GetCooldownTags(),
			BBGameplayTags::Cooldown_Hunter_Kingpin_AerialDash);
	});

	It("reduces both Crysta Shift cooldowns independently", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		AActor* Owner = TestWorld->SpawnActor<AActor>();
		if (!TestNotNull("ASC owner spawned", Owner)) return;
		UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Owner, TEXT("CrystaCooldownASC"));
		Owner->AddInstanceComponent(ASC);
		ASC->RegisterComponent();
		ASC->InitAbilityActorInfo(Owner, Owner);

		auto ApplyCooldown = [ASC](const FGameplayTag& Tag)
		{
			FGameplayTagContainer GrantedTags;
			GrantedTags.AddTag(Tag);
			GrantedTags.AddTag(BBGameplayTags::Cooldown_Dash);
			FGameplayEffectSpecHandle Spec = UBBGameplayAbility::BuildBBCooldownSpec(ASC, 12.0f, GrantedTags);
			return Spec.IsValid()
				? ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get())
				: FActiveGameplayEffectHandle();
		};
		auto Remaining = [ASC](FActiveGameplayEffectHandle Handle)
		{
			float StartTime = 0.0f;
			float Duration = 0.0f;
			ASC->GetGameplayEffectStartTimeAndDuration(Handle, StartTime, Duration);
			return StartTime + Duration - ASC->GetWorld()->GetTimeSeconds();
		};

		const FActiveGameplayEffectHandle Primary = ApplyCooldown(BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Primary);
		const FActiveGameplayEffectHandle Secondary = ApplyCooldown(BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Secondary);
		TestTrue("Primary cooldown handle is valid", Primary.IsValid());
		TestTrue("Secondary cooldown handle is valid", Secondary.IsValid());
		TestTrue("Primary cooldown tag is granted",
			ASC->HasMatchingGameplayTag(BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Primary));
		TestTrue("Secondary cooldown tag is granted",
			ASC->HasMatchingGameplayTag(BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Secondary));
		TestTrue("Shared dash classification is granted", ASC->HasMatchingGameplayTag(BBGameplayTags::Cooldown_Dash));
		const float PrimaryBefore = Remaining(Primary);
		const float SecondaryBefore = Remaining(Secondary);
		TestTrue("Primary cooldown was reduced", UGA_Crysta_Passive::ReduceActiveCooldownTag(ASC,
			BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Primary, 1.5f));
		const float PrimaryAfter = Remaining(Primary);
		const float SecondaryAfterPrimary = Remaining(Secondary);
		TestTrue("Primary loses 1.5 seconds", FMath::IsNearlyEqual(PrimaryBefore - PrimaryAfter, 1.5f, 0.05f));
		TestTrue("Primary reduction leaves secondary unchanged",
			FMath::IsNearlyEqual(SecondaryBefore, SecondaryAfterPrimary, 0.05f));
		TestTrue("Secondary cooldown was reduced", UGA_Crysta_Passive::ReduceActiveCooldownTag(ASC,
			BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Secondary, 1.5f));
		TestTrue("Secondary loses 1.5 seconds",
			FMath::IsNearlyEqual(SecondaryAfterPrimary - Remaining(Secondary), 1.5f, 0.05f));
	});

	It("spawns empowered Crysta LMB shots front-to-back and detonates a mark", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		TArray<FVector> SpawnLocations;
		UGA_Crysta_LMB::CalculateShotSpawnLocations(FVector(100.0f, 200.0f, 300.0f), FVector::ForwardVector,
			true, 90.0f, 95.0f, SpawnLocations);
		TestEqual("Empowered shot count", SpawnLocations.Num(), 2);
		if (SpawnLocations.Num() != 2) return;
		TestTrue("First shot is in front", SpawnLocations[0].X > SpawnLocations[1].X);
		TestTrue("Shots are separated by configured spacing",
			FMath::IsNearlyEqual(FVector::Distance(SpawnLocations[0], SpawnLocations[1]), 95.0f, 0.01f));
		TestTrue("Shots share one line", FMath::IsNearlyZero(SpawnLocations[0].Y - SpawnLocations[1].Y)
			&& FMath::IsNearlyZero(SpawnLocations[0].Z - SpawnLocations[1].Z));

		AActor* Source = TestWorld->SpawnActor<AActor>();
		ABBTargetDummy* Target = TestWorld->SpawnActor<ABBTargetDummy>(FVector(500.0f, 0.0f, 100.0f), FRotator::ZeroRotator);
		if (!TestNotNull("Source spawned", Source) || !TestNotNull("Target spawned", Target)) return;
		UAbilitySystemComponent* SourceASC = NewObject<UAbilitySystemComponent>(Source, TEXT("CrystaSourceASC"));
		Source->AddInstanceComponent(SourceASC);
		SourceASC->RegisterComponent();
		SourceASC->InitAbilityActorInfo(Source, Source);
		UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
		if (!TestNotNull("Target ASC valid", TargetASC)) return;
		FGameplayEffectSpecHandle MarkSpec = SourceASC->MakeOutgoingSpec(UBBReverberationMarkEffect::StaticClass(),
			1.0f, SourceASC->MakeEffectContext());
		if (!TestTrue("Mark spec valid", MarkSpec.IsValid())) return;
		SourceASC->ApplyGameplayEffectSpecToTarget(*MarkSpec.Data.Get(), TargetASC);
		TestTrue("Target starts marked", TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Crysta_Reverberation));

		int32 DetonationEvents = 0;
		const FGameplayTagContainer EventTags(BBGameplayTags::Event_Crysta_ReverberationDetonated);
		const FDelegateHandle EventHandle = SourceASC->AddGameplayEventTagContainerDelegate(EventTags,
			FGameplayEventTagMulticastDelegate::FDelegate::CreateLambda(
				[&DetonationEvents](FGameplayTag, const FGameplayEventData*) { ++DetonationEvents; }));
		FActorSpawnParameters ProjectileParams;
		ProjectileParams.Owner = Source;
		ProjectileParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ABBCrystaLMBProjectile* Projectile = TestWorld->SpawnActor<ABBCrystaLMBProjectile>(
			ABBCrystaLMBProjectile::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, ProjectileParams);
		if (!TestNotNull("Projectile spawned", Projectile)) return;
		Projectile->InitCrystaProjectile(SourceASC, UBBDamageEffect::StaticClass(),
			UBBReverberationMarkEffect::StaticClass(), 38.0f, 35.0f, 0, true, false, true);
		Projectile->ProcessOverlapForAutomation(Target);
		TestFalse("LMB removes Reverberation", TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Crysta_Reverberation));
		TestEqual("LMB emits one detonation event", DetonationEvents, 1);
		SourceASC->RemoveGameplayEventTagContainerDelegate(EventTags, EventHandle);
	});

	It("steers and returns Crysta Flux Snare on timeout or release", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		AHunterCharacter* Hunter = TestWorld->SpawnActor<AHunterCharacter>(FVector(0.0f, 0.0f, 200.0f), FRotator::ZeroRotator);
		if (!TestNotNull("Hunter spawned", Hunter)) return;
		Hunter->ServerSetAimLocation(FVector(2000.0f, 1000.0f, 200.0f));
		ABBCrystaFluxSnareProjectile* Steered = TestWorld->SpawnActor<ABBCrystaFluxSnareProjectile>(
			ABBCrystaFluxSnareProjectile::StaticClass(), FVector(100.0f, 0.0f, 200.0f), FRotator::ZeroRotator);
		if (!TestNotNull("Steered projectile spawned", Steered)) return;
		Steered->InitFluxSnare(Hunter, nullptr, nullptr, nullptr, nullptr, 0, FVector::ForwardVector);
		Steered->Tick(0.1f);
		TestTrue("Held aim bends projectile toward flick", Steered->GetActorLocation().Y > 1.0f
			&& Steered->GetCurrentTravelDirection().Y > 0.0f);
		const float BeforeReturnDistance = FVector::Dist2D(Hunter->GetActorLocation(), Steered->GetActorLocation());
		Steered->StartReturn();
		Steered->Tick(0.05f);
		TestTrue("Release starts return", Steered->IsReturning());
		TestTrue("Returning projectile moves toward Crysta",
			FVector::Dist2D(Hunter->GetActorLocation(), Steered->GetActorLocation()) < BeforeReturnDistance);

		Hunter->ServerSetAimLocation(FVector(3000.0f, 0.0f, 200.0f));
		ABBCrystaFluxSnareProjectile* Automatic = TestWorld->SpawnActor<ABBCrystaFluxSnareProjectile>(
			ABBCrystaFluxSnareProjectile::StaticClass(), FVector(100.0f, 0.0f, 200.0f), FRotator::ZeroRotator);
		if (!TestNotNull("Automatic projectile spawned", Automatic)) return;
		Automatic->InitFluxSnare(Hunter, nullptr, nullptr, nullptr, nullptr, 0, FVector::ForwardVector);
		Automatic->Tick(0.6f);
		Automatic->Tick(0.01f);
		TestTrue("Tap path auto-returns at max range", Automatic->IsReturning());
	});

	It("makes Void-swappable props movable and replicated", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		const FTransform SpawnTransform(FVector(100.0f, 0.0f, 0.0f));
		AActor* Prop = TestWorld->SpawnActorDeferred<AActor>(AActor::StaticClass(), SpawnTransform);
		if (!TestNotNull("Deferred prop spawned", Prop)) return;

		USceneComponent* Root = NewObject<USceneComponent>(Prop, TEXT("SwappablePropRoot"));
		Prop->AddInstanceComponent(Root);
		Prop->SetRootComponent(Root);
		Root->SetMobility(EComponentMobility::Static);
		Root->RegisterComponent();

		UBBVoidSwappableComponent* Marker = NewObject<UBBVoidSwappableComponent>(Prop, TEXT("VoidSwappable"));
		Prop->AddInstanceComponent(Marker);
		Marker->RegisterComponent();
		Prop->FinishSpawning(SpawnTransform);
		Marker->PrepareOwnerForSwap();

		TestEqual("Marked prop root becomes movable", Root->Mobility, EComponentMobility::Movable);
		TestTrue("Marked prop replicates", Prop->GetIsReplicated());
		TestTrue("Marked prop movement replicates", Prop->IsReplicatingMovement());
	});

	It("recognizes Void Shift non-hunter categories and rejects unmarked props", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		ABBWispPawn* Wisp = TestWorld->SpawnActor<ABBWispPawn>();
		ABBTestAlly* TestAlly = TestWorld->SpawnActor<ABBTestAlly>();
		ABBChestActor* Chest = TestWorld->SpawnActor<ABBChestActor>();
		AActor* MarkedProp = TestWorld->SpawnActor<AActor>();
		AActor* UnmarkedProp = TestWorld->SpawnActor<AActor>();
		if (!TestNotNull("Wisp spawned", Wisp) || !TestNotNull("Test ally spawned", TestAlly) || !TestNotNull("Chest spawned", Chest)
			|| !TestNotNull("Marked prop spawned", MarkedProp) || !TestNotNull("Unmarked prop spawned", UnmarkedProp)) return;

		UBBVoidSwappableComponent* Marker = NewObject<UBBVoidSwappableComponent>(MarkedProp, TEXT("VoidSwappable"));
		MarkedProp->AddInstanceComponent(Marker);
		Marker->RegisterComponent();

		TestTrue("Wisp is eligible", ABBVoidSwapProjectile::IsActorEligibleForSwap(Wisp));
		TestTrue("Test ally is eligible", ABBVoidSwapProjectile::IsActorEligibleForSwap(TestAlly));
		TestTrue("Chest is eligible", ABBVoidSwapProjectile::IsActorEligibleForSwap(Chest));
		TestTrue("Marked prop is eligible", ABBVoidSwapProjectile::IsActorEligibleForSwap(MarkedProp));
		TestFalse("Unmarked prop is ineligible", ABBVoidSwapProjectile::IsActorEligibleForSwap(UnmarkedProp));
	});

	It("executes Void Shift for wisps, test allies, chests, and marked props", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		const FVector SourceCenter(0.0f, 0.0f, 200.0f);
		const FVector DestCenter(2000.0f, 0.0f, 200.0f);
		AHunterCharacter* SourceHunter = TestWorld->SpawnActor<AHunterCharacter>(SourceCenter, FRotator::ZeroRotator);
		ABBWispPawn* Wisp = TestWorld->SpawnActor<ABBWispPawn>(SourceCenter + FVector(0.0f, 60.0f, 0.0f), FRotator::ZeroRotator);
		ABBChestActor* Chest = TestWorld->SpawnActor<ABBChestActor>(SourceCenter + FVector(0.0f, -60.0f, 0.0f), FRotator::ZeroRotator);
		ABBTestAlly* TestAlly = TestWorld->SpawnActor<ABBTestAlly>(DestCenter + FVector(0.0f, 60.0f, 0.0f), FRotator::ZeroRotator);
		if (!TestNotNull("Source hunter spawned", SourceHunter) || !TestNotNull("Wisp spawned", Wisp)
			|| !TestNotNull("Chest spawned", Chest) || !TestNotNull("Test ally spawned", TestAlly)) return;

		auto SpawnProp = [this](const TCHAR* RootName, const FVector& Location, bool bMarked)
		{
			AActor* Prop = TestWorld->SpawnActorDeferred<AActor>(AActor::StaticClass(), FTransform(Location));
			USphereComponent* Root = NewObject<USphereComponent>(Prop, FName(RootName));
			Prop->AddInstanceComponent(Root);
			Prop->SetRootComponent(Root);
			Root->InitSphereRadius(35.0f);
			Root->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Root->SetCollisionObjectType(bMarked ? ECC_WorldDynamic : ECC_WorldStatic);
			Root->SetCollisionResponseToAllChannels(ECR_Overlap);
			Root->SetGenerateOverlapEvents(true);
			Root->SetMobility(EComponentMobility::Static);
			Root->SetWorldLocation(Location);
			Root->RegisterComponent();
			UBBVoidSwappableComponent* Marker = nullptr;
			if (bMarked)
			{
				Marker = NewObject<UBBVoidSwappableComponent>(Prop, TEXT("VoidSwappable"));
				Prop->AddInstanceComponent(Marker);
				Marker->RegisterComponent();
			}
			Prop->FinishSpawning(FTransform(Location));
			if (Marker)
			{
				Marker->PrepareOwnerForSwap();
			}
			Root->UpdateOverlaps();
			return Prop;
		};
		AActor* MarkedProp = SpawnProp(TEXT("MarkedRoot"), DestCenter + FVector(0.0f, -60.0f, 0.0f), true);
		AActor* UnmarkedProp = SpawnProp(TEXT("UnmarkedRoot"), SourceCenter + FVector(0.0f, 120.0f, 0.0f), false);
		if (!TestNotNull("Marked prop spawned", MarkedProp) || !TestNotNull("Unmarked prop spawned", UnmarkedProp)) return;
		TestTrue("Marked prop starts at destination", MarkedProp->GetActorLocation().X > 1500.0f);
		TestTrue("Marked prop is movable", MarkedProp->GetRootComponent()->Mobility == EComponentMobility::Movable);

		ABBVoidSwapProjectile* Swap = TestWorld->SpawnActor<ABBVoidSwapProjectile>();
		if (!TestNotNull("Swap projectile spawned", Swap)) return;
		Swap->InitSwapProjectile(SourceHunter, SourceCenter, DestCenter, 0.1f, 300.0f, false);
		Swap->Tick(0.11f);
		TestTrue("Wisp moved to destination", Wisp->GetActorLocation().X > 1500.0f);
		TestTrue("Chest moved to destination", Chest->GetActorLocation().X > 1500.0f);
		TestTrue("Test ally moved to source", TestAlly->GetActorLocation().X < 500.0f);
		TestTrue("Marked prop moved to source", MarkedProp->GetActorLocation().X < 500.0f);
		TestTrue("Unmarked static prop stayed at source", FMath::Abs(UnmarkedProp->GetActorLocation().X) < 500.0f);
	});

	It("transitions Void R from warning to empowered active radius", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		ABBVoidSingularityActor* Normal = TestWorld->SpawnActor<ABBVoidSingularityActor>(
			FVector::ZeroVector, FRotator::ZeroRotator);
		ABBVoidSingularityActor* Empowered = TestWorld->SpawnActor<ABBVoidSingularityActor>(
			FVector(2000.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
		if (!TestNotNull("Normal singularity spawned", Normal) || !TestNotNull("Empowered singularity spawned", Empowered)) return;
		Normal->InitSingularity(nullptr, nullptr, nullptr, 0, 620.0f, 0.1f, 1.0f, false);
		Empowered->InitSingularity(nullptr, nullptr, nullptr, 0, 620.0f, 0.1f, 1.0f, true);
		TestFalse("Normal starts in warning state", Normal->IsVortexActive());
		TestFalse("Empowered starts in warning state", Empowered->IsVortexActive());
		TestTrue("Normal radius is exact", FMath::IsNearlyEqual(Normal->GetEffectiveRadius(), 620.0f));
		TestTrue("Empowered radius increases by 28 percent",
			FMath::IsNearlyEqual(Empowered->GetEffectiveRadius(), 620.0f * 1.28f));
		USphereComponent* EmpoweredSphere = Empowered->FindComponentByClass<USphereComponent>();
		if (!TestNotNull("Empowered pull sphere valid", EmpoweredSphere)) return;
		TestTrue("Empowered collision radius matches gameplay radius",
			FMath::IsNearlyEqual(EmpoweredSphere->GetScaledSphereRadius(), Empowered->GetEffectiveRadius(), 0.1f));
		TestTrue("Normal lifecycle timers are scheduled", Normal->AreLifecycleTimersScheduledForAutomation());
		TestTrue("Empowered lifecycle timers are scheduled", Empowered->AreLifecycleTimersScheduledForAutomation());
		Normal->ActivateForAutomation();
		Empowered->ActivateForAutomation();
		TestTrue("Normal activates after warning", Normal->IsVortexActive());
		TestTrue("Empowered activates after warning", Empowered->IsVortexActive());
		Normal->FinishForAutomation();
		Empowered->FinishForAutomation();
		TestTrue("Normal cleans up after active duration", Normal->IsActorBeingDestroyed());
		TestTrue("Empowered cleans up after active duration", Empowered->IsActorBeingDestroyed());
	});

	It("allows Void Q to burst and destroy a live puddle before expiry", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		ABBVoidAnomalyPuddle* Puddle = TestWorld->SpawnActor<ABBVoidAnomalyPuddle>();
		if (!TestNotNull("Puddle spawned", Puddle)) return;
		Puddle->InitPuddle(nullptr, nullptr, nullptr, 0, 460.0f, 60.0f, 0.0f, 0.0f);
		TestFalse("Puddle starts active", Puddle->IsActorBeingDestroyed());
		Puddle->Burst();
		TestTrue("Immediate burst destroys the puddle", Puddle->IsActorBeingDestroyed());
	});

	It("shows empowered Crysta LMB state", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		ABBCrystaLMBProjectile* Projectile = TestWorld->SpawnActor<ABBCrystaLMBProjectile>();
		if (!TestNotNull("Crysta projectile spawned", Projectile)) return;
		Projectile->InitCrystaProjectile(nullptr, nullptr, nullptr, 0.0f, 0.0f, 0, false, false, true);
		UStaticMeshComponent* Mesh = BBTestHelpers::FindMesh(Projectile, TEXT("ProjectileMesh"));
		if (!TestNotNull("Projectile mesh valid", Mesh)) return;
		TestTrue("Empowered projectile is enlarged", FMath::IsNearlyEqual(Mesh->GetRelativeScale3D().X, 0.24f));
	});

	It("builds bounded replicated beam and wedge geometry", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		ABBPrimitiveBeamActor* Beam = TestWorld->SpawnActor<ABBPrimitiveBeamActor>();
		ABBPrimitiveWedgeActor* Wedge = TestWorld->SpawnActor<ABBPrimitiveWedgeActor>();
		if (!TestNotNull("Beam spawned", Beam) || !TestNotNull("Wedge spawned", Wedge)) return;
		Beam->InitBeam(FVector::ZeroVector, FVector(1000.0f, 0.0f, 0.0f), 10.0f, 1.0f, FLinearColor::White);
		Wedge->InitWedge(FVector::ZeroVector, FVector::ForwardVector, 600.0f, 35.0f, 6.0f, 1.0f, FLinearColor::White);
		UStaticMeshComponent* BeamMesh = BBTestHelpers::FindMesh(Beam, TEXT("BeamMesh"));
		if (!TestNotNull("Beam mesh valid", BeamMesh)) return;
		TestTrue("Beam length scale matches 1000 units", FMath::IsNearlyEqual(BeamMesh->GetRelativeScale3D().Z, 10.0f));
		int32 VisibleWedgeSegments = 0;
		TArray<UStaticMeshComponent*> WedgeMeshes;
		Wedge->GetComponents<UStaticMeshComponent>(WedgeMeshes);
		for (const UStaticMeshComponent* Mesh : WedgeMeshes)
		{
			VisibleWedgeSegments += Mesh && Mesh->IsVisible() ? 1 : 0;
		}
		TestEqual("Wedge renders two edges and eight arc segments", VisibleWedgeSegments, 10);
		TestTrue("Fallback actors replicate", Beam->GetIsReplicated() && Wedge->GetIsReplicated());
	});

	It("keeps replicated burst fallback hidden until initialized", [this]()
	{
		if (!TestNotNull("World valid", TestWorld)) return;
		ABBPrimitiveBurstActor* Burst = TestWorld->SpawnActor<ABBPrimitiveBurstActor>();
		if (!TestNotNull("Burst spawned", Burst)) return;
		UStaticMeshComponent* BurstMesh = BBTestHelpers::FindMesh(Burst, TEXT("BurstMesh"));
		if (!TestNotNull("Burst mesh valid", BurstMesh)) return;
		TestFalse("Uninitialized burst is hidden", BurstMesh->IsVisible());
		Burst->InitBurst(FVector(120.0f, 30.0f, 10.0f), 75.0f, 1.0f, FLinearColor::White, true);
		TestTrue("Initialized burst is visible", BurstMesh->IsVisible());
		TestTrue("Burst radius matches requested disc radius",
			FMath::IsNearlyEqual(BurstMesh->GetRelativeScale3D().X, 1.5f));
		TestTrue("Burst fallback replicates", Burst->GetIsReplicated());
	});
}

// ============================================================================
// 7. Wisp revive/decay priority
//    Enemy contest overrides ordinary allies and healing. Eluna carry is the
//    explicit exception and continues accelerated revival without HP drain.
// ============================================================================

BEGIN_DEFINE_SPEC(FBBWispRulesSpec, "Breachborne.GameSystems.Wisp.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FBBWispRulesSpec)

void FBBWispRulesSpec::Define()
{
	auto MakeInput = []()
	{
		FBBWispTickInput Input;
		Input.CurrentHP = 100.0f;
		Input.MaxHP = 100.0f;
		Input.CurrentRez = 0.5f;
		return Input;
	};

	It("applies natural HP and revive decay", [this, MakeInput]()
	{
		const FBBWispTickResult Result = ABBWispPawn::ResolveWispTick(MakeInput());
		TestTrue("HP drains at the base rate", FMath::IsNearlyEqual(Result.HP, 99.6f));
		TestTrue("Partial revive progress decays", FMath::IsNearlyEqual(Result.Rez, 0.467f));
		TestFalse("Natural decay is not paused", Result.bDecayPaused);
	});

	It("freezes decay and fills for an uncontested ally", [this, MakeInput]()
	{
		FBBWispTickInput Input = MakeInput();
		Input.bAllyNearby = true;
		const FBBWispTickResult Result = ABBWispPawn::ResolveWispTick(Input);
		TestTrue("HP is frozen", FMath::IsNearlyEqual(Result.HP, 100.0f));
		TestTrue("Revive fills at 1x", FMath::IsNearlyEqual(Result.Rez, 0.54f));
		TestTrue("Ally pauses decay", Result.bDecayPaused);
	});

	It("keeps a healing-started revive latched until enemy contest", [this]()
	{
		bool bLatched = ABBWispPawn::ResolveHealingReviveLatch(false, true, false);
		TestTrue("A heal starts the revive latch", bLatched);

		bLatched = ABBWispPawn::ResolveHealingReviveLatch(bLatched, false, false);
		TestTrue("The latch persists after healing expires", bLatched);

		bLatched = ABBWispPawn::ResolveHealingReviveLatch(bLatched, false, true);
		TestFalse("Enemy contest clears the latch", bLatched);

		bLatched = ABBWispPawn::ResolveHealingReviveLatch(bLatched, true, false);
		TestTrue("A later heal can start revival again", bLatched);
	});

	It("uses accelerated enemy stomp decay", [this, MakeInput]()
	{
		FBBWispTickInput Input = MakeInput();
		Input.bEnemyNearby = true;
		const FBBWispTickResult Result = ABBWispPawn::ResolveWispTick(Input);
		TestTrue("HP drains at 3x", FMath::IsNearlyEqual(Result.HP, 98.8f));
		TestTrue("Revive progress decays", FMath::IsNearlyEqual(Result.Rez, 0.467f));
		TestFalse("Enemy does not pause decay", Result.bDecayPaused);
	});

	It("gives enemy contest priority over an ally", [this, MakeInput]()
	{
		FBBWispTickInput Input = MakeInput();
		Input.bAllyNearby = true;
		Input.bEnemyNearby = true;
		const FBBWispTickResult Result = ABBWispPawn::ResolveWispTick(Input);
		TestTrue("Contested HP drains at 3x", FMath::IsNearlyEqual(Result.HP, 98.8f));
		TestTrue("Contested revive does not fill", Result.Rez < Input.CurrentRez);
		TestFalse("Contest does not pause decay", Result.bDecayPaused);
	});

	It("turns healing into accelerated revive and freezes decay", [this, MakeInput]()
	{
		FBBWispTickInput Input = MakeInput();
		Input.HealAmount = 20.0f;
		const FBBWispTickResult Result = ABBWispPawn::ResolveWispTick(Input);
		TestTrue("HP is frozen by healing", FMath::IsNearlyEqual(Result.HP, 100.0f));
		TestTrue("Healing is capped at 2x revive", FMath::IsNearlyEqual(Result.Rez, 0.58f));
		TestTrue("Healing pauses decay", Result.bDecayPaused);
	});

	It("keeps periodic healing active between pulses", [this, MakeInput]()
	{
		FBBWispTickInput Input = MakeInput();
		Input.bHealingActive = true;
		const FBBWispTickResult Result = ABBWispPawn::ResolveWispTick(Input);
		TestTrue("Active healing freezes HP between pulses", FMath::IsNearlyEqual(Result.HP, 100.0f));
		TestTrue("Active healing fills at the base revive rate between pulses", FMath::IsNearlyEqual(Result.Rez, 0.54f));
		TestTrue("Active healing pauses decay between pulses", Result.bDecayPaused);

		Input.bEnemyNearby = true;
		const FBBWispTickResult Contested = ABBWispPawn::ResolveWispTick(Input);
		TestTrue("Enemy still drains HP during active healing", FMath::IsNearlyEqual(Contested.HP, 98.8f));
		TestTrue("Enemy overrides active-healing revive", Contested.Rez < Input.CurrentRez);
		TestFalse("Enemy prevents active healing from pausing decay", Contested.bDecayPaused);
	});

	It("gives enemy contest priority over healing", [this, MakeInput]()
	{
		FBBWispTickInput Input = MakeInput();
		Input.HealAmount = 20.0f;
		Input.bEnemyNearby = true;
		const FBBWispTickResult Result = ABBWispPawn::ResolveWispTick(Input);
		TestTrue("Enemy still drains HP at 3x", FMath::IsNearlyEqual(Result.HP, 98.8f));
		TestTrue("Contested healing cannot fill revive", Result.Rez < Input.CurrentRez);
		TestFalse("Contested healing does not pause decay", Result.bDecayPaused);
	});

	It("keeps Eluna-carried revival protected from enemy contest", [this, MakeInput]()
	{
		FBBWispTickInput Input = MakeInput();
		Input.bAllyNearby = true;
		Input.bEnemyNearby = true;
		Input.bCarried = true;
		const FBBWispTickResult Result = ABBWispPawn::ResolveWispTick(Input);
		TestTrue("Carried HP is frozen", FMath::IsNearlyEqual(Result.HP, 100.0f));
		TestTrue("Carry fills revive at 2x", FMath::IsNearlyEqual(Result.Rez, 0.58f));
		TestTrue("Carry pauses decay", Result.bDecayPaused);
	});
}

#endif // WITH_AUTOMATION_TESTS && WITH_EDITOR
