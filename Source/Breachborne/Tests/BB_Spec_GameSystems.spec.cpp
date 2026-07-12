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
#include "Components/StaticMeshComponent.h"
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
#include "Breachborne/Combat/BBCrystaBurstZone.h"
#include "Breachborne/Combat/BBCrystaLMBProjectile.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Combat/BBPrimitiveWedgeActor.h"
#include "Breachborne/Combat/BBVoidAnomalyPuddle.h"
#include "Breachborne/Combat/BBVoidOrbProjectile.h"
#include "Breachborne/Combat/BBVoidSingularityActor.h"
#include "Breachborne/Combat/BBVoidSnapProjectile.h"
#include "Breachborne/Combat/BBVoidSwapProjectile.h"

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
		ABBVoidSnapProjectile* Snap = TestWorld->SpawnActor<ABBVoidSnapProjectile>();
		ABBVoidSwapProjectile* Swap = TestWorld->SpawnActor<ABBVoidSwapProjectile>();
		if (!TestNotNull("Orb spawned", Orb) || !TestNotNull("Snap spawned", Snap) || !TestNotNull("Swap spawned", Swap)) return;
		Orb->InitVoidOrb(nullptr, nullptr, nullptr, 0.0f, 0, true);
		Snap->InitSnap(nullptr, nullptr, nullptr, nullptr, 0, FVector::ForwardVector, true);
		Swap->InitSwapProjectile(nullptr, FVector::ZeroVector, FVector(1000.0f, 0.0f, 0.0f), 60.0f, 500.0f, true);
		UStaticMeshComponent* OrbMesh = BBTestHelpers::FindMesh(Orb, TEXT("ProjectileMesh"));
		UStaticMeshComponent* ConeMesh = BBTestHelpers::FindMesh(Snap, TEXT("ConeMesh"));
		UStaticMeshComponent* ZoneMesh = BBTestHelpers::FindMesh(Swap, TEXT("ZoneMesh"));
		if (!TestNotNull("Orb mesh valid", OrbMesh) || !TestNotNull("Cone mesh valid", ConeMesh) || !TestNotNull("Swap zone mesh valid", ZoneMesh)) return;
		TestTrue("Charged orb is enlarged", FMath::IsNearlyEqual(OrbMesh->GetRelativeScale3D().X, 0.24f));
		TestTrue("Empowered cone is enlarged", FMath::IsNearlyEqual(ConeMesh->GetRelativeScale3D().X, 1.9f));
		TestTrue("Empowered swap radius is exact", FMath::IsNearlyEqual(ZoneMesh->GetRelativeScale3D().X, 10.0f));
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

#endif // WITH_AUTOMATION_TESTS && WITH_EDITOR
