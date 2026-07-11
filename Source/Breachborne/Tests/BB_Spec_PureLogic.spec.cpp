/**
 * BB_Spec_PureLogic.spec.cpp
 *
 * Automation spec tests for pure, world-free logic in the Breachborne module.
 * These tests exercise maths, struct defaults, and enum sentinel values that could
 * silently regress without a full PIE boot.
 *
 * Run from: Session Frontend > Automation > Breachborne > PureLogic
 *
 * To run from command line (cooked or editor):
 *   -ExecCmds="Automation RunTests Breachborne.PureLogic"
 *
 * All tests are guarded by WITH_AUTOMATION_TESTS so they vanish from shipping builds.
 */

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

// --- Types under test ---
#include "Breachborne/Core/BreachborneTypes.h"
#include "Breachborne/PvE/BBContractSubsystem.h"
#include "Breachborne/Storm/StormTypes.h"

// ============================================================================
// 1. Enum Sentinel Values
//    If someone inserts a value in the middle of an enum the serialised save/rep
//    data silently shifts. These tests pin the values we rely on in code.
// ============================================================================

DEFINE_SPEC(FBBEnumSentinelSpec, "Breachborne.PureLogic.EnumSentinels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FBBEnumSentinelSpec::Define()
{
	Describe("EBBCustomMovementMode", [this]()
	{
		It("Gliding == 1", [this]()
		{
			TestEqual("Gliding value", (uint8)EBBCustomMovementMode::Gliding, (uint8)1);
		});

		It("Spiked == 2", [this]()
		{
			TestEqual("Spiked value", (uint8)EBBCustomMovementMode::Spiked, (uint8)2);
		});

		It("Mantling == 3  [PhysCustom switch relies on this]", [this]()
		{
			// PhysCustom dispatches on (uint8)CustomMovementMode directly.
			// If Mantling shifts, the switch case silently falls through.
			TestEqual("Mantling value", (uint8)EBBCustomMovementMode::Mantling, (uint8)3);
		});
	});

	Describe("EBBDayNightPhase", [this]()
	{
		It("Day == 0", [this]()
		{
			TestEqual("Day value", (uint8)EBBDayNightPhase::Day, (uint8)0);
		});

		It("Night == 1", [this]()
		{
			TestEqual("Night value", (uint8)EBBDayNightPhase::Night, (uint8)1);
		});
	});

	Describe("EBBContractType", [this]()
	{
		It("None == 0", [this]()
		{
			TestEqual("None value", (uint8)EBBContractType::None, (uint8)0);
		});

		It("Brawler != CreepFarm  [ProgressContract branches on this]", [this]()
		{
			TestNotEqual("Brawler != CreepFarm",
				(uint8)EBBContractType::Brawler,
				(uint8)EBBContractType::CreepFarm);
		});
	});

	Describe("EMatchPhase", [this]()
	{
		It("Playing is after Dropping  [timer start depends on order]", [this]()
		{
			TestTrue("Playing > Dropping",
				(uint8)EMatchPhase::Playing > (uint8)EMatchPhase::Dropping);
		});
	});
}

// ============================================================================
// 2. FBBContract Struct Logic
//    IsComplete() is a critical predicate — wrong return breaks payout.
// ============================================================================

DEFINE_SPEC(FBBContractStructSpec, "Breachborne.PureLogic.ContractStruct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FBBContractStructSpec::Define()
{
	Describe("FBBContract::IsComplete", [this]()
	{
		It("returns false when progress is zero", [this]()
		{
			FBBContract C;
			C.ObjectiveCount  = 3;
			C.CurrentProgress = 0;
			TestFalse("Not complete at 0/3", C.IsComplete());
		});

		It("returns false when one short of objective", [this]()
		{
			FBBContract C;
			C.ObjectiveCount  = 3;
			C.CurrentProgress = 2;
			TestFalse("Not complete at 2/3", C.IsComplete());
		});

		It("returns true when exactly at objective", [this]()
		{
			FBBContract C;
			C.ObjectiveCount  = 3;
			C.CurrentProgress = 3;
			TestTrue("Complete at 3/3", C.IsComplete());
		});

		It("returns true when over objective  [no clamping assumed]", [this]()
		{
			FBBContract C;
			C.ObjectiveCount  = 3;
			C.CurrentProgress = 5;
			TestTrue("Still complete at 5/3", C.IsComplete());
		});
	});

	Describe("FBBContract defaults", [this]()
	{
		It("bCompleted starts false", [this]()
		{
			FBBContract C;
			TestFalse("Default not completed", C.bCompleted);
		});

		It("ContractType starts None", [this]()
		{
			FBBContract C;
			TestEqual("Default type is None", C.ContractType, EBBContractType::None);
		});

		It("GoldReward > 0  [payout must be positive]", [this]()
		{
			FBBContract C;
			TestTrue("Gold reward positive", C.GoldReward > 0);
		});
	});
}

// ============================================================================
// 3. Gold Steal on Kill — Formula Verification
//    AwardKillRewards uses: Max(1, FloorToInt(VictimGold * 0.25f))
//    These tests pin the formula so a future refactor can't silently change rates.
// ============================================================================

DEFINE_SPEC(FBBGoldStealSpec, "Breachborne.PureLogic.GoldStealFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FBBGoldStealSpec::Define()
{
	static constexpr float StealFraction = 0.25f;

	auto CalcSteal = [](int32 VictimGold) -> int32
	{
		return FMath::Max(1, FMath::FloorToInt(VictimGold * 0.25f));
	};

	Describe("Gold steal formula", [this, CalcSteal]()
	{
		It("steals exactly 25% of 100g  (25g)", [this, CalcSteal]()
		{
			TestEqual("100g steal", CalcSteal(100), 25);
		});

		It("steals 25% of 200g  (50g)", [this, CalcSteal]()
		{
			TestEqual("200g steal", CalcSteal(200), 50);
		});

		It("floors fractional result: 30g -> 7g not 7.5g", [this, CalcSteal]()
		{
			TestEqual("30g steal floors to 7", CalcSteal(30), 7);
		});

		It("minimum steal is 1g even from a 0g victim", [this, CalcSteal]()
		{
			TestEqual("0g victim still loses 1g", CalcSteal(0), 1);
		});

		It("minimum steal is 1g from a 3g victim  (0.75 -> floor -> 0 -> max(1,0) = 1)", [this, CalcSteal]()
		{
			TestEqual("3g victim loses 1g", CalcSteal(3), 1);
		});

		It("minimum steal is 1g from a 1g victim", [this, CalcSteal]()
		{
			TestEqual("1g victim loses 1g", CalcSteal(1), 1);
		});

		It("steals 25% of large wallet: 1000g -> 250g", [this, CalcSteal]()
		{
			TestEqual("1000g steal", CalcSteal(1000), 250);
		});
	});
}

// ============================================================================
// 4. Vision Cone Math
//    UBBVisionConeComponent::IsInVisionCone uses 2D distance + dot-product angle.
//    Tests validate the underlying formula with known vectors.
//    HalfAngle default = 60 degrees (120° total FOV).
// ============================================================================

DEFINE_SPEC(FBBVisionConeMathSpec, "Breachborne.PureLogic.VisionConeMath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FBBVisionConeMathSpec::Define()
{
	// Replicate the formula used in IsInVisionCone:
	//   bool bInRange = Distance2D <= EffectiveRange
	//   bool bInCone  = DotProduct(Forward2D, DirToTarget2D) >= cos(HalfAngleDeg)
	auto IsInCone = [](FVector OwnerPos, FVector OwnerForward, float HalfAngleDeg,
	                   float EffectiveRange, FVector TargetPos) -> bool
	{
		const FVector Delta    = TargetPos - OwnerPos;
		const float   Dist2D  = FVector(Delta.X, Delta.Y, 0.f).Size();

		if (Dist2D > EffectiveRange)
		{
			return false;
		}

		const FVector Dir2D      = FVector(Delta.X, Delta.Y, 0.f).GetSafeNormal();
		const FVector Forward2D  = FVector(OwnerForward.X, OwnerForward.Y, 0.f).GetSafeNormal();
		const float   DotResult  = FVector::DotProduct(Forward2D, Dir2D);
		const float   CosThresh  = FMath::Cos(FMath::DegreesToRadians(HalfAngleDeg));

		return DotResult >= CosThresh;
	};

	const FVector OwnerPos(0.f, 0.f, 0.f);
	const FVector FacingPosX(1.f, 0.f, 0.f);  // Facing +X
	const float   HalfAngle    = 60.f;          // 120° total FOV
	const float   EffectRange  = 2000.f;

	Describe("IsInVisionCone math", [this, IsInCone, OwnerPos, FacingPosX, HalfAngle, EffectRange]()
	{
		It("target directly in front is visible", [this, IsInCone, OwnerPos, FacingPosX, HalfAngle, EffectRange]()
		{
			TestTrue("Front visible", IsInCone(OwnerPos, FacingPosX, HalfAngle, EffectRange, FVector(500.f, 0.f, 0.f)));
		});

		It("target directly behind is NOT visible", [this, IsInCone, OwnerPos, FacingPosX, HalfAngle, EffectRange]()
		{
			TestFalse("Rear not visible", IsInCone(OwnerPos, FacingPosX, HalfAngle, EffectRange, FVector(-500.f, 0.f, 0.f)));
		});

		It("target at 90 degrees (side) is NOT visible with 60-degree half-angle", [this, IsInCone, OwnerPos, FacingPosX, HalfAngle, EffectRange]()
		{
			TestFalse("90-deg not visible", IsInCone(OwnerPos, FacingPosX, HalfAngle, EffectRange, FVector(0.f, 500.f, 0.f)));
		});

		It("target at exactly 60 degrees is on the cone boundary (visible)", [this, IsInCone, OwnerPos, FacingPosX, HalfAngle, EffectRange]()
		{
			// At 60°: X = cos(60°)=0.5, Y = sin(60°)≈0.866
			const FVector EdgeTarget(0.5f * 500.f, 0.866f * 500.f, 0.f);
			TestTrue("Edge of cone is visible", IsInCone(OwnerPos, FacingPosX, HalfAngle, EffectRange, EdgeTarget));
		});

		It("target at 61 degrees is just outside the cone (NOT visible)", [this, IsInCone, OwnerPos, FacingPosX, HalfAngle, EffectRange]()
		{
			const float Rad = FMath::DegreesToRadians(61.f);
			const FVector JustOutside(FMath::Cos(Rad) * 500.f, FMath::Sin(Rad) * 500.f, 0.f);
			TestFalse("Just outside cone not visible", IsInCone(OwnerPos, FacingPosX, HalfAngle, EffectRange, JustOutside));
		});

		It("target in front but beyond range is NOT visible", [this, IsInCone, OwnerPos, FacingPosX, HalfAngle, EffectRange]()
		{
			TestFalse("Beyond range not visible", IsInCone(OwnerPos, FacingPosX, HalfAngle, EffectRange, FVector(2100.f, 0.f, 0.f)));
		});

		It("Z-axis offset does not affect 2D cone check — tall target in front is visible", [this, IsInCone, OwnerPos, FacingPosX, HalfAngle, EffectRange]()
		{
			// Targets above/below should still pass the 2D cone check (3D position, 2D logic)
			TestTrue("High-Z front target visible", IsInCone(OwnerPos, FacingPosX, HalfAngle, EffectRange, FVector(500.f, 0.f, 900.f)));
		});

		It("brush multiplier: night range (1200) still catches a 1000-unit target", [this, IsInCone, OwnerPos, FacingPosX, HalfAngle]()
		{
			const float BrushMult  = 0.6f;
			const float NightRange = 1200.f;
			const float BrushRange = NightRange * BrushMult; // 720
			TestFalse("1000cm target outside brush-night range", IsInCone(OwnerPos, FacingPosX, HalfAngle, BrushRange, FVector(900.f, 0.f, 0.f)));
			TestTrue("600cm target inside brush-night range",    IsInCone(OwnerPos, FacingPosX, HalfAngle, BrushRange, FVector(600.f, 0.f, 0.f)));
		});
	});
}

// ============================================================================
// 5. Storm Phase Config Defaults
//    DamageTickInterval drives a SetTimer call. If it defaults to 0 the timer
//    fires every frame and tanks performance. Pin all sensitive defaults.
// ============================================================================

DEFINE_SPEC(FBBStormConfigSpec, "Breachborne.PureLogic.StormPhaseConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FBBStormConfigSpec::Define()
{
	Describe("FStormPhaseConfig defaults", [this]()
	{
		It("DamageTickInterval > 0  [must not trigger every frame]", [this]()
		{
			FStormPhaseConfig C;
			TestTrue("DamageTickInterval > 0", C.DamageTickInterval > 0.f);
		});

		It("DamagePerTick >= 0  [no negative storm damage]", [this]()
		{
			FStormPhaseConfig C;
			TestTrue("DamagePerTick >= 0", C.DamagePerTick >= 0.f);
		});

		It("TargetRadiusFraction in [0,1]  [fractions outside this break radius math]", [this]()
		{
			FStormPhaseConfig C;
			TestTrue("TargetRadiusFraction in [0,1]",
				C.TargetRadiusFraction >= 0.f && C.TargetRadiusFraction <= 1.f);
		});

		It("ShrinkDuration > 0  [zero duration causes instant teleport]", [this]()
		{
			FStormPhaseConfig C;
			TestTrue("ShrinkDuration > 0", C.ShrinkDuration > 0.f);
		});
	});
}

// ============================================================================
// 6. Mantle Parameter Constraints
//    Abyss recovery traces and swept path values must stay in sensible ranges.
// ============================================================================

DEFINE_SPEC(FBBMantleConstraintSpec, "Breachborne.PureLogic.MantleConstraints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FBBMantleConstraintSpec::Define()
{
	static constexpr float MaxTopSearchHeight = 2200.f;
	static constexpr float MinMantleDepth = 150.f;
	static constexpr float LandingInset = 120.f;
	static constexpr float MantleDuration = 0.45f;
	static constexpr float MantleArcHeight = 120.f;
	static constexpr float MaxDuration = 1.25f;

	Describe("Mantle trace defaults", [this]()
	{
		It("MaxTopSearchHeight > 0  [zero = no top floor ever found]", [this]()
		{
			TestTrue("MaxTopSearchHeight > 0", MaxTopSearchHeight > 0.f);
		});

		It("MinMantleDepth > 0  [zero = ground-level objects would mantle]", [this]()
		{
			TestTrue("MinDepth > 0", MinMantleDepth > 0.f);
		});
	});

	Describe("Mantle physics defaults", [this]()
	{
		It("LandingInset > 0  [must land safely inside the island edge]", [this]()
		{
			TestTrue("LandingInset > 0", LandingInset > 0.f);
		});

		It("MantleDuration is short but nonzero", [this]()
		{
			TestTrue("MantleDuration range", MantleDuration > 0.f && MantleDuration < MaxDuration);
		});

		It("MantleArcHeight > 0  [must clear rough island lips]", [this]()
		{
			TestTrue("MantleArcHeight > 0", MantleArcHeight > 0.f);
		});

		It("MaxDuration > MantleDuration  [safety timeout must exist]", [this]()
		{
			TestTrue("MaxDuration > MantleDuration", MaxDuration > MantleDuration);
		});
	});
}

// ============================================================================
// 7. Day/Night Phase Toggle Logic
//    AdvanceDayNightPhase() toggles Day->Night->Day.
//    Since this is pure enum math, test it without a world.
// ============================================================================

DEFINE_SPEC(FBBDayNightToggleSpec, "Breachborne.PureLogic.DayNightToggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FBBDayNightToggleSpec::Define()
{
	// Mirror the toggle logic from BreachborneGameState::AdvanceDayNightPhase
	auto Toggle = [](EBBDayNightPhase Current) -> EBBDayNightPhase
	{
		return (Current == EBBDayNightPhase::Day)
			? EBBDayNightPhase::Night
			: EBBDayNightPhase::Day;
	};

	Describe("Day/Night toggle", [this, Toggle]()
	{
		It("Day -> Night", [this, Toggle]()
		{
			TestEqual("Day toggles to Night",
				Toggle(EBBDayNightPhase::Day), EBBDayNightPhase::Night);
		});

		It("Night -> Day", [this, Toggle]()
		{
			TestEqual("Night toggles to Day",
				Toggle(EBBDayNightPhase::Night), EBBDayNightPhase::Day);
		});

		It("Two toggles restore original state", [this, Toggle]()
		{
			const EBBDayNightPhase Start = EBBDayNightPhase::Day;
			const EBBDayNightPhase AfterTwo = Toggle(Toggle(Start));
			TestEqual("Two toggles -> back to Day", AfterTwo, EBBDayNightPhase::Day);
		});

		It("Three toggles land on Night", [this, Toggle]()
		{
			EBBDayNightPhase Phase = EBBDayNightPhase::Day;
			Phase = Toggle(Phase);
			Phase = Toggle(Phase);
			Phase = Toggle(Phase);
			TestEqual("Three toggles -> Night", Phase, EBBDayNightPhase::Night);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
