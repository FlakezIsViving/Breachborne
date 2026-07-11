#include "BBDebugHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Breachborne/Core/BreachbornePlayerController.h"
#include "Breachborne/PvE/BBBasecampActor.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Core/BreachborneGameState.h"
#include "Breachborne/Core/BreachborneTypes.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Characters/GliderComponent.h"
#include "Breachborne/Wisp/BBWispPawn.h"
#include "Breachborne/Storm/BBStormManager.h"
#include "Breachborne/Items/BBInventoryTypes.h"
#include "Breachborne/Items/BBItemTypes.h"
#include "Breachborne/Items/BBWorldItem.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "DrawDebugHelpers.h"

// ============================================================================
// Construction
// ============================================================================

ABBDebugHUD::ABBDebugHUD()
{
}

// ============================================================================
// Console Commands
// ============================================================================

void ABBDebugHUD::DebugHUD()
{
	bShowDebugOverlay = !bShowDebugOverlay;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
			FString::Printf(TEXT("Debug Overlay: %s"), bShowDebugOverlay ? TEXT("ON") : TEXT("OFF")));
	}
}

void ABBDebugHUD::ToggleHUD()
{
	bShowGameplayHUD = !bShowGameplayHUD;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
			FString::Printf(TEXT("Gameplay HUD: %s"), bShowGameplayHUD ? TEXT("ON") : TEXT("OFF")));
	}
}

void ABBDebugHUD::DebugShapes()
{
	bShowDebugShapes = !bShowDebugShapes;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
			FString::Printf(TEXT("Debug Shapes: %s"), bShowDebugShapes ? TEXT("ON") : TEXT("OFF")));
	}
}

// ============================================================================
// Main Draw
// ============================================================================

void ABBDebugHUD::DrawHUD()
{
	Super::DrawHUD();

	if (bShowDebugOverlay)
	{
		DrawDebugOverlay();
	}

	const ABreachborneGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABreachborneGameState>() : nullptr;
	const EMatchPhase MatchPhase = GS ? GS->GetMatchPhase() : EMatchPhase::WaitingForPlayers;
	const bool bInGameplayPhase = MatchPhase == EMatchPhase::Dropping || MatchPhase == EMatchPhase::Playing;

	if (bShowGameplayHUD && bInGameplayPhase)
	{
		DrawGameplayHUD();
	}

	if (bShowDebugShapes)
	{
		DrawDebugShapes3D();
	}
}

// ============================================================================
// Data Access Helpers
// ============================================================================

ABreachbornePlayerState* ABBDebugHUD::GetOwnerPlayerState() const
{
	APlayerController* PC = GetOwningPlayerController();
	return PC ? PC->GetPlayerState<ABreachbornePlayerState>() : nullptr;
}

AHunterCharacter* ABBDebugHUD::GetOwnerHunter() const
{
	APlayerController* PC = GetOwningPlayerController();
	return PC ? Cast<AHunterCharacter>(PC->GetPawn()) : nullptr;
}

ABBWispPawn* ABBDebugHUD::GetOwnerWisp() const
{
	APlayerController* PC = GetOwningPlayerController();
	return PC ? Cast<ABBWispPawn>(PC->GetPawn()) : nullptr;
}

ABBStormManager* ABBDebugHUD::FindStormManager() const
{
	if (ABBStormManager* Cached = CachedStormManager.Get())
	{
		return Cached;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ABBStormManager> It(World); It; ++It)
	{
		CachedStormManager = *It;
		return *It;
	}

	return nullptr;
}

// ============================================================================
// Canvas Drawing Helpers
// ============================================================================

void ABBDebugHUD::DrawPanel(float X, float Y, float W, float H, const FLinearColor& Color) const
{
	FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(W, H), Color);
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
}

float ABBDebugHUD::DrawTextLine(float X, float Y, const FString& Text, const FLinearColor& Color, float Scale) const
{
	FCanvasTextItem TextItem(FVector2D(X, Y), FText::FromString(Text), GEngine->GetSmallFont(), Color);
	TextItem.Scale = FVector2D(Scale, Scale);
	Canvas->DrawItem(TextItem);
	return 14.0f * Scale;
}

void ABBDebugHUD::DrawBar(float X, float Y, float W, float H, float FillPct, const FLinearColor& FillColor, const FLinearColor& BgColor) const
{
	FillPct = FMath::Clamp(FillPct, 0.0f, 1.0f);

	// Background
	DrawPanel(X, Y, W, H, BgColor);

	// Fill
	if (FillPct > 0.0f)
	{
		DrawPanel(X, Y, W * FillPct, H, FillColor);
	}

	// Border
	const FLinearColor BorderColor(0.5f, 0.5f, 0.5f, 0.8f);
	DrawPanel(X, Y, W, 1.0f, BorderColor);				// top
	DrawPanel(X, Y + H - 1.0f, W, 1.0f, BorderColor);		// bottom
	DrawPanel(X, Y, 1.0f, H, BorderColor);					// left
	DrawPanel(X + W - 1.0f, Y, 1.0f, H, BorderColor);		// right
}

// ============================================================================
// Debug Overlay (top-left panel)
// ============================================================================

void ABBDebugHUD::DrawDebugOverlay()
{
	ABreachbornePlayerState* PS = GetOwnerPlayerState();
	if (!PS)
	{
		return;
	}

	const float PanelX = 10.0f;
	const float PanelY = 10.0f;
	const float PanelW = 320.0f;
	const float LineH = 14.0f;
	const float Pad = 6.0f;
	float CurY = PanelY + Pad;

	// Pre-draw background (estimate height — we'll overdraw slightly)
	DrawPanel(PanelX, PanelY, PanelW, 500.0f, FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));

	const float TextX = PanelX + Pad;

	// --- Header ---
	const FString LifeStateStr = UEnum::GetValueAsString(PS->GetLifeState());
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("=== DEBUG: %s ==="), *PS->GetPlayerName()), FLinearColor::Yellow);
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Team: %d  |  Level: %d  |  XP: %d  |  Kills: %d"),
		PS->GetTeamID(), PS->GetHunterLevel(), PS->GetXP(), PS->GetKills()));
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Life State: %s"), *LifeStateStr));
	if (const ABreachbornePlayerController* BBPC = Cast<ABreachbornePlayerController>(GetOwningPlayerController()))
	{
		const FString RecallState = BBPC->IsBasecampRecallActive()
			? FString::Printf(TEXT("Recall: %.0f%%"), BBPC->GetBasecampRecallProgress() * 100.0f)
			: TEXT("Recall: idle");
		CurY += DrawTextLine(TextX, CurY, RecallState, FLinearColor(0.4f, 0.9f, 1.0f));
	}
	CurY += 4.0f;

	// --- Health ---
	const FRepHealthData& HP = PS->GetHealthData();
	CurY += DrawTextLine(TextX, CurY, TEXT("--- Health ---"), FLinearColor(0.5f, 1.0f, 0.5f));
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("HP: %.0f / %.0f"), HP.Health, HP.MaxHealth));
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Shield: %.0f / %.0f"), HP.Shield, HP.MaxShield));
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Armor: %.0f"), HP.Armor));
	CurY += 4.0f;

	// --- Combat ---
	const FRepCombatData& Cmb = PS->GetCombatData();
	CurY += DrawTextLine(TextX, CurY, TEXT("--- Combat ---"), FLinearColor(1.0f, 0.5f, 0.5f));
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("ATK: %.0f  |  AP: %.0f"), Cmb.AttackPower, Cmb.AbilityPower));
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("CDR: %.0f%%  |  Crit: %.0f%% (x%.1f)"), Cmb.CooldownReduction * 100.0f, Cmb.CritChance * 100.0f, Cmb.CritMultiplier));
	CurY += 4.0f;

	// --- Movement ---
	const FRepMovementData& Mov = PS->GetMovementData();
	CurY += DrawTextLine(TextX, CurY, TEXT("--- Movement ---"), FLinearColor(0.5f, 0.7f, 1.0f));
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Speed: %.0f  |  Glide: %.0f  |  Dash: %.0f"), Mov.MoveSpeed, Mov.GlideSpeed, Mov.DashDistance));
	CurY += 4.0f;

	// --- Glider (only when alive as hunter) ---
	AHunterCharacter* Hunter = GetOwnerHunter();
	if (Hunter)
	{
		UGliderComponent* Glider = Hunter->GetGliderComponent();
		if (Glider)
		{
			const FString GliderStateStr = UEnum::GetValueAsString(Glider->GetGliderState());
			CurY += DrawTextLine(TextX, CurY, TEXT("--- Glider ---"), FLinearColor(1.0f, 0.8f, 0.3f));
			CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("State: %s  |  Fuel: %.0f%%"), *GliderStateStr, Glider->GetFuelLevel() * 100.0f));

			// Fuel bar inline
			const float BarX = TextX;
			const float BarW = PanelW - Pad * 2;
			const float FuelPct = Glider->GetFuelLevel();
			FLinearColor FuelColor = FuelPct > 0.5f
				? FMath::Lerp(FLinearColor(1.0f, 0.72f, 0.05f), FLinearColor(0.35f, 1.0f, 0.1f), (FuelPct - 0.5f) / 0.5f)
				: FMath::Lerp(FLinearColor(1.0f, 0.05f, 0.02f), FLinearColor(1.0f, 0.72f, 0.05f), FuelPct / 0.5f);
			DrawBar(BarX, CurY, BarW, 8.0f, FuelPct, FuelColor, FLinearColor(0.02f, 0.02f, 0.02f, 0.9f));
			CurY += 12.0f;
		}
	}

	// --- Wisp (only when in wisp state) ---
	if (PS->GetLifeState() == EPlayerLifeState::Wisp)
	{
		const FRepWispData& WD = PS->GetWispData();
		ABBWispPawn* Wisp = GetOwnerWisp();
		const FString WispStateStr = Wisp ? UEnum::GetValueAsString(Wisp->GetWispState()) : TEXT("N/A");
		const float RezPct = Wisp ? Wisp->GetRezBarProgress() : 0.0f;

		CurY += DrawTextLine(TextX, CurY, TEXT("--- Wisp ---"), FLinearColor(1.0f, 0.6f, 0.0f));
		CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("State: %s"), *WispStateStr));
		CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Wisp HP: %.0f / %.0f (%.0f%%)"),
			WD.WispHP, WD.MaxWispHP, WD.MaxWispHP > 0.0f ? (WD.WispHP / WD.MaxWispHP) * 100.0f : 0.0f));
		CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Rez Bar: %.0f%%"), RezPct * 100.0f));
		CurY += 4.0f;
	}

	// --- Inventory ---
	const FRepInventoryData& Inv = PS->GetInventoryData();
	CurY += DrawTextLine(TextX, CurY, TEXT("--- Inventory ---"), FLinearColor(0.9f, 0.8f, 0.2f));
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Gold: %d  |  Shards: %d"), Inv.Gold, Inv.UpgradeShards));

	// Equipment
	const FString WeaponStr = Inv.Weapon.IsEmpty() ? TEXT("[empty]") : FString::Printf(TEXT("%s T%d"), *Inv.Weapon.ItemID.ToString(), Inv.Weapon.UpgradeTier);
	const FString HelmetStr = Inv.Helmet.IsEmpty() ? TEXT("[empty]") : FString::Printf(TEXT("%s T%d"), *Inv.Helmet.ItemID.ToString(), Inv.Helmet.UpgradeTier);
	const FString BootsStr = Inv.Boots.IsEmpty() ? TEXT("[empty]") : FString::Printf(TEXT("%s T%d"), *Inv.Boots.ItemID.ToString(), Inv.Boots.UpgradeTier);
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Weapon: %s"), *WeaponStr));
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Helmet: %s"), *HelmetStr));
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Boots:  %s"), *BootsStr));

	// Armor
	const FString ArmorTierStr = UEnum::GetValueAsString(Inv.Armor.Tier);
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Armor: %s (%.0f HP)"), *ArmorTierStr, Inv.Armor.CurrentHP));

	// Powers
	const FString Pow1Str = Inv.Power1.IsEmpty() ? TEXT("[empty]") : Inv.Power1.PowerID.ToString();
	const FString Pow2Str = Inv.Power2.IsEmpty() ? TEXT("[empty]") : Inv.Power2.PowerID.ToString();
	CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Powers: %s | %s"), *Pow1Str, *Pow2Str));

	// Backpack
	FString BackpackStr = TEXT("Backpack: ");
	for (int32 i = 0; i < Inv.Backpack.Num(); ++i)
	{
		if (i > 0) BackpackStr += TEXT(", ");
		if (Inv.Backpack[i].IsEmpty())
		{
			BackpackStr += TEXT("[empty]");
		}
		else
		{
			BackpackStr += FString::Printf(TEXT("%s x%d"), *Inv.Backpack[i].ConsumableID.ToString(), Inv.Backpack[i].Count);
		}
	}
	CurY += DrawTextLine(TextX, CurY, BackpackStr);
	CurY += 4.0f;

	// --- Storm ---
	ABBStormManager* Storm = FindStormManager();
	if (Storm)
	{
		CurY += DrawTextLine(TextX, CurY, TEXT("--- Storm ---"), FLinearColor(0.7f, 0.3f, 1.0f));

		// Read replicated data directly from the storm actor.
		// StormManager's CurrentCenter/Radius/TargetCenter/TargetRadius/CurrentPhaseIndex/PhaseState
		// are replicated UPROPERTY — we access them via the class.
		// Since they are protected/private, we use a simple distance check from owned pawn instead.
		APlayerController* PC = GetOwningPlayerController();
		APawn* OwnedPawn = PC ? PC->GetPawn() : nullptr;
		if (OwnedPawn)
		{
			const FVector PawnLoc = OwnedPawn->GetActorLocation();
			const FVector StormLoc = Storm->GetActorLocation();
			const float DistToStorm = FVector::Dist2D(PawnLoc, StormLoc);
			CurY += DrawTextLine(TextX, CurY, FString::Printf(TEXT("Distance to storm center: %.0f"), DistToStorm));
		}
		else
		{
			CurY += DrawTextLine(TextX, CurY, TEXT("Storm: active"));
		}
	}

	// Redraw the background at the correct height
	// (We overdrew at 500 earlier — that's fine, the black panel just extends a bit further than needed)
}

// ============================================================================
// Gameplay HUD — four-corner layout
//
// Each corner is its own panel that pulls data live from PlayerState/GameState/ASC.
// We don't use UMG yet — primordial v0.1 uses immediate-mode Canvas drawing for fast
// iteration. Later, panels can move to UMG widgets without changing data sources.
// ============================================================================

namespace BBHUD
{
	// Local helpers — kept in a namespace so they don't clutter the class header.

	/** Pretty hunter name from HunterID. Replace with HunterDef lookup once HunterDef
	 *  refs are available client-side. */
	static FString GetHunterName(int32 HunterID)
	{
		switch (HunterID)
		{
		case 1:  return TEXT("Ghost");
		case 2:  return TEXT("Kingpin");
		default: return FString::Printf(TEXT("Hunter %d"), HunterID);
		}
	}

	/** Display label for an ability slot from its input tag. */
	static FString InputTagToSlotLabel(const FGameplayTag& InputTag)
	{
		if (InputTag == BBGameplayTags::InputTag_RMB)    return TEXT("RMB");
		if (InputTag == BBGameplayTags::InputTag_Shift)  return TEXT("Shift");
		if (InputTag == BBGameplayTags::InputTag_Q)      return TEXT("Q");
		if (InputTag == BBGameplayTags::InputTag_R)      return TEXT("R");
		if (InputTag == BBGameplayTags::InputTag_Power1) return TEXT("F");
		if (InputTag == BBGameplayTags::InputTag_Power2) return TEXT("G");
		return TEXT("?");
	}

	/** Disambiguator sub-label for the dash-stack case (Aerial / Ground). */
	static FString DashSubLabel(UClass* AbilityClass)
	{
		if (!AbilityClass) return FString();
		const FString N = AbilityClass->GetName();
		if (N.Contains(TEXT("Aerial"))) return TEXT("A");
		if (N.Contains(TEXT("Ground"))) return TEXT("G");
		return FString();
	}

	/** Per-ability-slot color by InputTag. Helps the eye associate icons with keys. */
	static FLinearColor SlotColorForTag(const FGameplayTag& InputTag)
	{
		if (InputTag == BBGameplayTags::InputTag_RMB)    return FLinearColor(0.85f, 0.55f, 0.20f); // orange (RMB)
		if (InputTag == BBGameplayTags::InputTag_Shift)  return FLinearColor(0.30f, 0.80f, 0.95f); // cyan (Shift/dash)
		if (InputTag == BBGameplayTags::InputTag_Q)      return FLinearColor(0.95f, 0.85f, 0.30f); // yellow (Q)
		if (InputTag == BBGameplayTags::InputTag_R)      return FLinearColor(0.90f, 0.30f, 0.30f); // red (Ult)
		if (InputTag == BBGameplayTags::InputTag_Power1) return FLinearColor(0.70f, 0.50f, 0.95f); // purple (F power)
		if (InputTag == BBGameplayTags::InputTag_Power2) return FLinearColor(0.70f, 0.50f, 0.95f); // purple (G power)
		return FLinearColor::Gray;
	}

	static float ArmorMaxHP(EArmorTier Tier)
	{
		switch (Tier)
		{
		case EArmorTier::White:  return 50.0f;
		case EArmorTier::Green:  return 100.0f;
		case EArmorTier::Blue:   return 150.0f;
		case EArmorTier::Purple: return 200.0f;
		default: return 0.0f;
		}
	}

	static FLinearColor ArmorTierColor(EArmorTier Tier)
	{
		switch (Tier)
		{
		case EArmorTier::White:  return FLinearColor(0.88f, 0.88f, 0.88f, 0.95f);
		case EArmorTier::Green:  return FLinearColor(0.18f, 0.85f, 0.35f, 0.95f);
		case EArmorTier::Blue:   return FLinearColor(0.18f, 0.48f, 1.00f, 0.95f);
		case EArmorTier::Purple: return FLinearColor(0.70f, 0.32f, 1.00f, 0.95f);
		default: return FLinearColor(0.24f, 0.24f, 0.28f, 0.85f);
		}
	}

	static FString ArmorTierLabel(EArmorTier Tier)
	{
		switch (Tier)
		{
		case EArmorTier::White:  return TEXT("White");
		case EArmorTier::Green:  return TEXT("Green");
		case EArmorTier::Blue:   return TEXT("Blue");
		case EArmorTier::Purple: return TEXT("Purple");
		default: return TEXT("None");
		}
	}

	static FString ConsumableShortLabel(FName ConsumableID)
	{
		if (ConsumableID == FName(TEXT("ViveBean"))) return TEXT("Bean");
		if (ConsumableID == FName(TEXT("ViveBrew"))) return TEXT("Brew");
		const FString ID = ConsumableID.ToString();
		return ID.Len() > 5 ? ID.Left(5) : ID;
	}

	static FLinearColor ConsumableColor(FName ConsumableID)
	{
		if (ConsumableID == FName(TEXT("ViveBean"))) return FLinearColor(0.25f, 0.95f, 0.55f, 0.92f);
		if (ConsumableID == FName(TEXT("ViveBrew"))) return FLinearColor(0.05f, 0.90f, 1.00f, 0.92f);
		return FLinearColor(0.40f, 0.65f, 0.40f, 0.90f);
	}
}

void ABBDebugHUD::DrawGameplayHUD()
{
	if (!Canvas) return;

	DrawBasecampIndicators();
	DrawWorldItemPrompts();
	DrawBottomLeftPanel();
	DrawBottomRightPanel();
	DrawTopRightPanel();
	DrawTopLeftPanel();
}

void ABBDebugHUD::DrawWorldItemPrompts()
{
	UWorld* World = GetWorld();
	APlayerController* PC = GetOwningPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!World || !PC || !Pawn)
	{
		return;
	}

	for (TActorIterator<ABBWorldItem> It(World); It; ++It)
	{
		const ABBWorldItem* Item = *It;
		if (!Item || Item->IsPickedUp() || !Item->IsPowerPickup())
		{
			continue;
		}

		if (FVector::DistSquared(Pawn->GetActorLocation(), Item->GetActorLocation()) > FMath::Square(450.0f))
		{
			continue;
		}

		FVector2D ScreenPos;
		if (!PC->ProjectWorldLocationToScreen(Item->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f), ScreenPos))
		{
			continue;
		}

		const FString Label = FString::Printf(TEXT("E Pick Up Power: %s"), *Item->GetPickupItemID().ToString());
		const float W = FMath::Clamp(56.0f + Label.Len() * 6.5f, 150.0f, 260.0f);
		const float H = 24.0f;
		const float X = ScreenPos.X - W * 0.5f;
		const float Y = ScreenPos.Y;
		const FLinearColor Color(0.70f, 0.50f, 0.95f, 0.95f);
		DrawPanel(X, Y, W, H, FLinearColor(0.02f, 0.025f, 0.03f, 0.70f));
		DrawPanel(X, Y, W, 2.0f, Color);
		DrawTextLine(X + 7.0f, Y + 5.0f, Label, Color, 0.78f);
	}
}

void ABBDebugHUD::DrawBasecampIndicators()
{
	UWorld* World = GetWorld();
	APlayerController* PC = GetOwningPlayerController();
	ABreachbornePlayerState* PS = GetOwnerPlayerState();
	if (!World || !PC || !PS)
	{
		return;
	}

	const int32 ViewerTeamID = PS->GetTeamID();

	for (TActorIterator<ABBBasecampActor> It(World); It; ++It)
	{
		ABBBasecampActor* Basecamp = *It;
		if (!Basecamp)
		{
			continue;
		}

		FVector2D ScreenPos;
		const FVector AnchorLocation = Basecamp->GetPlatformWorldLocation() + FVector(0.0f, 0.0f, 260.0f);
		if (!PC->ProjectWorldLocationToScreen(AnchorLocation, ScreenPos))
		{
			continue;
		}

		const bool bOwnedByViewer = Basecamp->OwningTeamID == ViewerTeamID && ViewerTeamID != -1;
		const bool bOwnedByEnemy = Basecamp->OwningTeamID != -1 && Basecamp->OwningTeamID != ViewerTeamID;
		const bool bActivelyCapturing = Basecamp->CaptureTeamID != -1 && Basecamp->CaptureProgress > 0.0f && Basecamp->OwningTeamID != Basecamp->CaptureTeamID;

		FLinearColor StateColor = FLinearColor(0.85f, 0.85f, 0.85f, 0.92f);
		FString StateLabel = TEXT("BASECAMP");
		if (Basecamp->bContested)
		{
			StateColor = FLinearColor(1.0f, 0.18f, 0.10f, 0.95f);
			StateLabel = TEXT("CONTESTED");
		}
		else if (bOwnedByViewer)
		{
			StateColor = FLinearColor(0.15f, 0.95f, 0.35f, 0.95f);
			StateLabel = TEXT("YOUR BASECAMP");
		}
		else if (bOwnedByEnemy)
		{
			StateColor = FLinearColor(1.0f, 0.35f, 0.20f, 0.95f);
			StateLabel = FString::Printf(TEXT("TEAM %d BASECAMP"), Basecamp->OwningTeamID);
		}
		else if (bActivelyCapturing)
		{
			StateColor = FLinearColor(1.0f, 0.82f, 0.20f, 0.95f);
			StateLabel = FString::Printf(TEXT("CAPTURING T%d"), Basecamp->CaptureTeamID);
		}
		else
		{
			StateLabel = TEXT("NEUTRAL BASECAMP");
		}

		const float PanelW = 180.0f;
		const float PanelH = 46.0f;
		const float PanelX = ScreenPos.X - PanelW * 0.5f;
		const float PanelY = ScreenPos.Y - 18.0f;
		DrawPanel(PanelX, PanelY, PanelW, PanelH, FLinearColor(0.02f, 0.025f, 0.03f, 0.68f));
		DrawPanel(PanelX, PanelY, PanelW, 2.0f, StateColor);
		DrawTextLine(PanelX + 8.0f, PanelY + 5.0f, StateLabel, StateColor, 0.95f);

		float ProgressPct = 0.0f;
		FLinearColor ProgressColor = StateColor;
		FString ProgressLabel;
		if (Basecamp->bContested)
		{
			ProgressPct = Basecamp->CaptureProgress;
			ProgressLabel = TEXT("paused");
			ProgressColor = FLinearColor(1.0f, 0.18f, 0.10f, 0.95f);
		}
		else if (bActivelyCapturing)
		{
			ProgressPct = Basecamp->CaptureProgress;
			ProgressLabel = FString::Printf(TEXT("%.0f%%"), ProgressPct * 100.0f);
			ProgressColor = FLinearColor(1.0f, 0.82f, 0.20f, 0.95f);
		}
		else if (bOwnedByViewer)
		{
			ProgressPct = 1.0f;
			ProgressLabel = TEXT("B: recall");
			ProgressColor = FLinearColor(0.15f, 0.95f, 0.35f, 0.95f);
		}
		else if (bOwnedByEnemy)
		{
			ProgressPct = 1.0f;
			ProgressLabel = TEXT("capture to claim");
		}
		else
		{
			ProgressPct = Basecamp->CaptureProgress;
			ProgressLabel = Basecamp->CaptureProgress > 0.0f
				? FString::Printf(TEXT("%.0f%%"), Basecamp->CaptureProgress * 100.0f)
				: TEXT("stand to capture");
		}

		DrawBar(PanelX + 8.0f, PanelY + 24.0f, PanelW - 16.0f, 8.0f, ProgressPct,
			ProgressColor, FLinearColor(0.08f, 0.08f, 0.09f, 0.9f));
		DrawTextLine(PanelX + 8.0f, PanelY + 33.0f, ProgressLabel, FLinearColor::White, 0.75f);

		const FRepInventoryData& Inv = PS->GetInventoryData();
		const auto GetArmorMaxHPForHUD = [](EArmorTier Tier) -> float
		{
			switch (Tier)
			{
			case EArmorTier::White:  return 50.0f;
			case EArmorTier::Green:  return 100.0f;
			case EArmorTier::Blue:   return 150.0f;
			case EArmorTier::Purple: return 200.0f;
			default: return 0.0f;
			}
		};
		const auto GetRepairCostForHUD = [](EArmorTier Tier) -> int32
		{
			switch (Tier)
			{
			case EArmorTier::White:  return 100;
			case EArmorTier::Green:  return 200;
			case EArmorTier::Blue:   return 500;
			case EArmorTier::Purple: return 1000;
			default: return 0;
			}
		};
		int32 ViveBeanCount = 0;
		for (const FConsumableStack& Stack : Inv.Backpack)
		{
			if (Stack.ConsumableID == FName(TEXT("ViveBean")))
			{
				ViveBeanCount += Stack.Count;
			}
		}

		FString AnvilLabel = TEXT("E  REPAIR");
		if (!bOwnedByViewer)
		{
			AnvilLabel = TEXT("ANVIL");
		}
		else if (!Inv.Armor.HasArmor())
		{
			AnvilLabel = TEXT("NO ARMOR");
		}
		else
		{
			const float ArmorMaxHP = GetArmorMaxHPForHUD(Inv.Armor.Tier);
			if (ArmorMaxHP > 0.0f && Inv.Armor.CurrentHP >= ArmorMaxHP)
			{
				AnvilLabel = TEXT("ARMOR FULL");
			}
			else
			{
				AnvilLabel = FString::Printf(TEXT("E  REPAIR %dg"), GetRepairCostForHUD(Inv.Armor.Tier));
			}
		}

		const FString CauldronLabel = bOwnedByViewer
			? FString::Printf(TEXT("E  BREW  BEANS:%d"), ViveBeanCount)
			: TEXT("CAULDRON");

		DrawBasecampStationPrompt(Basecamp, Basecamp->GetAnvilWorldLocation() + FVector(0.0f, 0.0f, 150.0f),
			AnvilLabel, FLinearColor(1.0f, 0.55f, 0.12f, 0.92f));
		DrawBasecampStationPrompt(Basecamp, Basecamp->GetCauldronWorldLocation() + FVector(0.0f, 0.0f, 150.0f),
			CauldronLabel, FLinearColor(0.05f, 0.90f, 1.0f, 0.92f));

		if (Basecamp->ActiveInteractionType != EBBBasecampInteractionType::None && Basecamp->StationProgress > 0.0f)
		{
			const bool bRepair = Basecamp->ActiveInteractionType == EBBBasecampInteractionType::RepairArmor;
			const FVector StationLocation = bRepair ? Basecamp->GetAnvilWorldLocation() : Basecamp->GetCauldronWorldLocation();
			FVector2D StationScreen;
			if (PC->ProjectWorldLocationToScreen(StationLocation + FVector(0.0f, 0.0f, 210.0f), StationScreen))
			{
				const float W = 130.0f;
				const float X = StationScreen.X - W * 0.5f;
				const float Y = StationScreen.Y + 22.0f;
				DrawTextLine(X, Y - 15.0f, bRepair ? TEXT("REPAIRING") : TEXT("BREWING"), FLinearColor::White, 0.8f);
				DrawBar(X, Y, W, 8.0f, Basecamp->StationProgress,
					bRepair ? FLinearColor(1.0f, 0.55f, 0.12f, 0.95f) : FLinearColor(0.05f, 0.90f, 1.0f, 0.95f),
					FLinearColor(0.02f, 0.02f, 0.03f, 0.85f));
				DrawTextLine(X, Y + 10.0f, TEXT("VULNERABLE"), FLinearColor(1.0f, 0.18f, 0.10f, 0.95f), 0.75f);
			}
		}
	}

	if (const ABreachbornePlayerController* BBPC = Cast<ABreachbornePlayerController>(PC))
	{
		if (BBPC->IsBasecampRecallActive())
		{
			const float W = 320.0f;
			const float X = Canvas->SizeX * 0.5f - W * 0.5f;
			const float Y = Canvas->SizeY * 0.5f + 120.0f;
			DrawPanel(X - 8.0f, Y - 22.0f, W + 16.0f, 48.0f, FLinearColor(0.02f, 0.025f, 0.03f, 0.72f));
			DrawTextLine(X, Y - 18.0f, TEXT("RECALLING TO BASECAMP"), FLinearColor(0.4f, 0.9f, 1.0f), 1.0f);
			DrawBar(X, Y + 2.0f, W, 10.0f, BBPC->GetBasecampRecallProgress(),
				FLinearColor(0.4f, 0.9f, 1.0f, 0.96f), FLinearColor(0.04f, 0.05f, 0.06f, 0.95f));
		}
	}
}

void ABBDebugHUD::DrawBasecampStationPrompt(const ABBBasecampActor* Basecamp, const FVector& WorldLocation,
	const FString& Label, const FLinearColor& Color) const
{
	if (!Basecamp)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	FVector2D ScreenPos;
	if (!PC->ProjectWorldLocationToScreen(WorldLocation, ScreenPos))
	{
		return;
	}

	const float W = FMath::Clamp(40.0f + Label.Len() * 7.0f, 86.0f, 180.0f);
	const float H = 24.0f;
	const float X = ScreenPos.X - W * 0.5f;
	const float Y = ScreenPos.Y;
	DrawPanel(X, Y, W, H, FLinearColor(0.02f, 0.025f, 0.03f, 0.65f));
	DrawPanel(X, Y, W, 2.0f, Color);
	DrawTextLine(X + 7.0f, Y + 5.0f, Label, Color, 0.78f);
}

// ----------------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------------

void ABBDebugHUD::DrawAbilitySlot(float X, float Y, float Size, const FString& CornerLabel,
	const FString& SubLabel, float RemainingCD, float MaxCD,
	const FLinearColor& ReadyColor) const
{
	const bool bOnCooldown = RemainingCD > 0.0f;

	// Box: dim when on cooldown, full color when ready.
	const FLinearColor FillColor = bOnCooldown
		? FLinearColor(ReadyColor.R * 0.30f, ReadyColor.G * 0.30f, ReadyColor.B * 0.30f, 0.85f)
		: FLinearColor(ReadyColor.R, ReadyColor.G, ReadyColor.B, 0.85f);
	DrawPanel(X, Y, Size, Size, FillColor);

	// Border (brighter when ready).
	const FLinearColor BorderColor = bOnCooldown
		? FLinearColor(0.5f, 0.5f, 0.5f, 0.9f)
		: FLinearColor(1.0f, 1.0f, 1.0f, 0.9f);
	DrawPanel(X, Y, Size, 2.0f, BorderColor);              // top
	DrawPanel(X, Y + Size - 2.0f, Size, 2.0f, BorderColor); // bottom
	DrawPanel(X, Y, 2.0f, Size, BorderColor);              // left
	DrawPanel(X + Size - 2.0f, Y, 2.0f, Size, BorderColor); // right

	// Cooldown fill from bottom (radial would need Canvas tricks; bottom-fill is fine).
	if (bOnCooldown && MaxCD > 0.0f)
	{
		const float FillFrac = FMath::Clamp(RemainingCD / MaxCD, 0.0f, 1.0f);
		const float FillH = Size * FillFrac;
		DrawPanel(X, Y + Size - FillH, Size, FillH, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
	}

	// Corner label (input key).
	DrawTextLine(X + 4.0f, Y + 2.0f, CornerLabel, FLinearColor::White, 1.0f);

	// Sub-label (e.g. "A"/"G" for dash stack).
	if (!SubLabel.IsEmpty())
	{
		DrawTextLine(X + Size - 12.0f, Y + 2.0f, SubLabel, FLinearColor::White, 1.0f);
	}

	// Cooldown remaining text (centered-ish), only when on cooldown.
	if (bOnCooldown)
	{
		const FString CDStr = FString::Printf(TEXT("%.1f"), RemainingCD);
		DrawTextLine(X + Size * 0.5f - 10.0f, Y + Size * 0.5f - 6.0f, CDStr, FLinearColor::White, 1.4f);
	}
}

void ABBDebugHUD::DrawHPChunks(float X, float Y, float Width, float Height, float Health, float MaxHealth) const
{
	// One chunk per 100 HP. Final chunk shows partial fill if MaxHealth isn't a multiple of 100.
	constexpr float HPPerChunk = 100.0f;
	const int32 NumChunks = FMath::Max(1, FMath::CeilToInt(MaxHealth / HPPerChunk));
	const float ChunkGap = 2.0f;
	const float ChunkW = (Width - ChunkGap * (NumChunks - 1)) / NumChunks;

	const FLinearColor BgColor(0.10f, 0.10f, 0.10f, 0.7f);
	const FLinearColor FullColor(0.20f, 0.95f, 0.30f, 0.95f);
	const FLinearColor LowColor(0.95f, 0.20f, 0.15f, 0.95f);
	const FLinearColor MidColor(1.00f, 0.85f, 0.10f, 0.95f);

	const float OverallPct = MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f;
	const FLinearColor TintColor = (OverallPct > 0.6f) ? FullColor : (OverallPct > 0.3f ? MidColor : LowColor);

	for (int32 i = 0; i < NumChunks; ++i)
	{
		const float ChunkX = X + i * (ChunkW + ChunkGap);

		// HP this chunk represents
		const float ChunkStartHP = i * HPPerChunk;
		const float ChunkEndHP   = FMath::Min(ChunkStartHP + HPPerChunk, MaxHealth);
		const float ChunkRange   = ChunkEndHP - ChunkStartHP;
		const float ChunkFillHP  = FMath::Clamp(Health - ChunkStartHP, 0.0f, ChunkRange);
		const float ChunkPct     = ChunkRange > 0.0f ? ChunkFillHP / ChunkRange : 0.0f;

		// Bg
		DrawPanel(ChunkX, Y, ChunkW, Height, BgColor);
		// Fill
		if (ChunkPct > 0.0f)
		{
			DrawPanel(ChunkX, Y, ChunkW * ChunkPct, Height, TintColor);
		}
		// Border
		const FLinearColor Border(0.5f, 0.5f, 0.5f, 0.8f);
		DrawPanel(ChunkX, Y, ChunkW, 1.0f, Border);
		DrawPanel(ChunkX, Y + Height - 1.0f, ChunkW, 1.0f, Border);
		DrawPanel(ChunkX, Y, 1.0f, Height, Border);
		DrawPanel(ChunkX + ChunkW - 1.0f, Y, 1.0f, Height, Border);
	}
}

float ABBDebugHUD::GetCooldownRemaining(UAbilitySystemComponent* ASC, const FGameplayTag& CooldownTag, float& OutDuration) const
{
	OutDuration = 0.0f;
	if (!ASC || !CooldownTag.IsValid()) return 0.0f;

	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
		FGameplayTagContainer(CooldownTag));
	const TArray<TPair<float, float>> RemAndDur = ASC->GetActiveEffectsTimeRemainingAndDuration(Query);
	if (RemAndDur.Num() > 0)
	{
		OutDuration = RemAndDur[0].Value;
		return RemAndDur[0].Key;
	}
	return 0.0f;
}

// ----------------------------------------------------------------------------
// Bottom-Left: name, level, XP/HP/mana, abilities, F/G powers
// ----------------------------------------------------------------------------

void ABBDebugHUD::DrawBottomLeftPanel()
{
	ABreachbornePlayerState* PS = GetOwnerPlayerState();
	if (!PS) return;

	const float ScreenH = Canvas->SizeY;

	// --- Name + level header ---
	const FString HunterName = BBHUD::GetHunterName(PS->GetHunterID());
	const float NameX = 16.0f;
	const float NameY = ScreenH - 240.0f;

	DrawTextLine(NameX, NameY, FString::Printf(TEXT("%s"), *HunterName), FLinearColor(0.95f, 0.95f, 1.0f), 1.6f);
	DrawTextLine(NameX + 220.0f, NameY + 4.0f, FString::Printf(TEXT("LV %d"), PS->GetHunterLevel()),
		FLinearColor(1.0f, 0.85f, 0.30f), 1.4f);

	// --- XP bar (no XP system yet) ---
	const float BarX = NameX;
	const float BarW = 280.0f;
	float CurY = NameY + 26.0f;

	const float XPPct = 0.0f; // TODO: when XP curve exists, compute (CurXP - LevelMin) / (LevelMax - LevelMin)
	DrawBar(BarX, CurY, BarW, 6.0f, XPPct, FLinearColor(0.55f, 0.30f, 0.95f, 0.95f), FLinearColor(0.10f, 0.05f, 0.15f, 0.7f));
	DrawTextLine(BarX + BarW + 8.0f, CurY - 2.0f, TEXT("XP (TODO)"), FLinearColor(0.55f, 0.30f, 0.95f), 0.9f);
	CurY += 12.0f;

	// --- HP chunks (100 HP per square) ---
	const FRepHealthData& HP = PS->GetHealthData();
	DrawHPChunks(BarX, CurY, BarW, 14.0f, HP.Health, HP.MaxHealth);
	DrawTextLine(BarX + BarW + 8.0f, CurY - 1.0f,
		FString::Printf(TEXT("%.0f / %.0f"), HP.Health, HP.MaxHealth), FLinearColor::White, 1.0f);
	CurY += 18.0f;

	// --- Mana bar (no mana system yet) ---
	DrawBar(BarX, CurY, BarW, 8.0f, 0.0f, FLinearColor(0.20f, 0.50f, 0.95f, 0.95f),
		FLinearColor(0.05f, 0.10f, 0.15f, 0.7f));
	DrawTextLine(BarX + BarW + 8.0f, CurY - 2.0f, TEXT("MANA (TODO)"), FLinearColor(0.20f, 0.50f, 0.95f), 0.9f);
	CurY += 14.0f;

	// --- Ability slots: RMB, Shift(s), Q, R, then a small gap, then F, G ---
	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC) return;

	const float SlotSize = 44.0f;
	const float SlotGap = 6.0f;
	float SlotX = BarX;
	const float SlotY = CurY + 6.0f;

	// Build ordered slot list. We collect granted UBBGameplayAbility instances, group
	// by InputTag, and iterate the slot order so multiples of the same key (Kingpin
	// Shift = Aerial+Ground) render side-by-side.
	struct FAbilitySlot
	{
		FGameplayTag InputTag;
		const UBBGameplayAbility* AbilityCDO = nullptr;
	};
	TArray<FAbilitySlot> Slots;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const UBBGameplayAbility* BBAbility = Cast<UBBGameplayAbility>(Spec.Ability);
		if (!BBAbility) continue;
		if (Spec.DynamicAbilityTags.HasTagExact(BBGameplayTags::InputTag_Power1))
		{
			Slots.Add({ BBGameplayTags::InputTag_Power1, BBAbility });
		}
		else if (Spec.DynamicAbilityTags.HasTagExact(BBGameplayTags::InputTag_Power2))
		{
			Slots.Add({ BBGameplayTags::InputTag_Power2, BBAbility });
		}
		else if (BBAbility->GetAbilityInputTag().IsValid())
		{
			Slots.Add({ BBAbility->GetAbilityInputTag(), BBAbility });
		}
	}

	// Render in fixed order — same key always in the same screen position.
	// Pull native tags into FGameplayTag locals first; FNativeGameplayTag has a deleted
	// copy ctor, so a braced-init list of {BBGameplayTags::...} would deduce its element
	// type as FNativeGameplayTag and fail to copy them into the array. Locals trigger
	// the implicit FNativeGameplayTag→FGameplayTag conversion exactly once each.
	const FGameplayTag InputRMB   = BBGameplayTags::InputTag_RMB;
	const FGameplayTag InputShift = BBGameplayTags::InputTag_Shift;
	const FGameplayTag InputQ     = BBGameplayTags::InputTag_Q;
	const FGameplayTag InputR     = BBGameplayTags::InputTag_R;
	const TArray<FGameplayTag> SlotOrder = { InputRMB, InputShift, InputQ, InputR };

	for (const FGameplayTag& WantTag : SlotOrder)
	{
		// Find all abilities matching this input tag (usually 1, can be 2 for dashes).
		for (const FAbilitySlot& Slot : Slots)
		{
			if (!Slot.InputTag.MatchesTagExact(WantTag)) continue;

			// Look up cooldown remaining via the ability's CooldownTagContainer.
			float Remaining = 0.0f, Duration = 0.0f;
			if (Slot.AbilityCDO && Slot.AbilityCDO->GetCooldownTags())
			{
				for (const FGameplayTag& CDTag : *Slot.AbilityCDO->GetCooldownTags())
				{
					Remaining = GetCooldownRemaining(ASC, CDTag, Duration);
					if (Remaining > 0.0f) break;
				}
			}

			DrawAbilitySlot(SlotX, SlotY, SlotSize,
				BBHUD::InputTagToSlotLabel(Slot.InputTag),
				BBHUD::DashSubLabel(Slot.AbilityCDO ? Slot.AbilityCDO->GetClass() : nullptr),
				Remaining, Duration,
				BBHUD::SlotColorForTag(Slot.InputTag));

			SlotX += SlotSize + SlotGap;
		}
	}

	// Small visual gap then F / G powers (always shown, even if no power is equipped — empty box).
	SlotX += 14.0f;

	// Same FNativeGameplayTag/FGameplayTag conversion dance as above.
	const FGameplayTag InputF = BBGameplayTags::InputTag_Power1;
	const FGameplayTag InputG = BBGameplayTags::InputTag_Power2;
	const TArray<FGameplayTag> PowerSlots = { InputF, InputG };
	const FRepInventoryData& Inv = PS->GetInventoryData();
	for (const FGameplayTag& WantTag : PowerSlots)
	{
		const FAbilitySlot* Found = Slots.FindByPredicate(
			[&](const FAbilitySlot& S){ return S.InputTag.MatchesTagExact(WantTag); });

		float Remaining = 0.0f, Duration = 0.0f;
		const FGameplayTag PowerCooldownTag = WantTag.MatchesTagExact(InputG) ? BBGameplayTags::Cooldown_Power2 : BBGameplayTags::Cooldown_Power1;
		Remaining = GetCooldownRemaining(ASC, PowerCooldownTag, Duration);
		const FPowerSlotState& PowerState = WantTag.MatchesTagExact(InputG) ? Inv.Power2 : Inv.Power1;
		const bool bHasPower = !PowerState.IsEmpty();

		const FLinearColor PowerColor = bHasPower || Found
			? BBHUD::SlotColorForTag(WantTag)
			: FLinearColor(0.25f, 0.25f, 0.30f, 0.85f); // empty slot = dim grey
		DrawAbilitySlot(SlotX, SlotY, SlotSize,
			BBHUD::InputTagToSlotLabel(WantTag),
			bHasPower ? PowerState.PowerID.ToString().Left(5) : FString(),
			Remaining, Duration, PowerColor);

		SlotX += SlotSize + SlotGap;
	}
}

// ----------------------------------------------------------------------------
// Bottom-Right: consumables, equipment, shards, gold
// ----------------------------------------------------------------------------

void ABBDebugHUD::DrawBottomRightPanel()
{
	ABreachbornePlayerState* PS = GetOwnerPlayerState();
	if (!PS) return;

	const float ScreenW = Canvas->SizeX;
	const float ScreenH = Canvas->SizeY;

	const FRepInventoryData& Inv = PS->GetInventoryData();

	const float SlotSize = 36.0f;
	const float SlotGap = 6.0f;
	const float RightMargin = 16.0f;

	// Layout: row 1 = 4 consumables, row 2 = 4 equipment slots, then [Shards] [Gold] in the corner.
	const float RowsTotalW = 4 * SlotSize + 3 * SlotGap;
	const float StartX = ScreenW - RightMargin - RowsTotalW - 110.0f; // leave space for shards/gold to the right
	const float Row1Y  = ScreenH - 110.0f;
	const float Row2Y  = Row1Y + SlotSize + 6.0f;

	// --- Consumables (1-4) ---
	for (int32 i = 0; i < 4; ++i)
	{
		const float SlotX = StartX + i * (SlotSize + SlotGap);
		const bool bEmpty = (i >= Inv.Backpack.Num()) || Inv.Backpack[i].IsEmpty();
		const FLinearColor Color = bEmpty
			? FLinearColor(0.20f, 0.20f, 0.25f, 0.85f)
			: BBHUD::ConsumableColor(Inv.Backpack[i].ConsumableID);
		DrawPanel(SlotX, Row1Y, SlotSize, SlotSize, Color);
		// Border
		const FLinearColor Border(0.6f, 0.6f, 0.6f, 0.9f);
		DrawPanel(SlotX, Row1Y, SlotSize, 1.5f, Border);
		DrawPanel(SlotX, Row1Y + SlotSize - 1.5f, SlotSize, 1.5f, Border);
		DrawPanel(SlotX, Row1Y, 1.5f, SlotSize, Border);
		DrawPanel(SlotX + SlotSize - 1.5f, Row1Y, 1.5f, SlotSize, Border);
		// Number key label
		DrawTextLine(SlotX + 3.0f, Row1Y + 2.0f, FString::FromInt(i + 1), FLinearColor::White, 0.95f);
		if (!bEmpty)
		{
			DrawTextLine(SlotX + 4.0f, Row1Y + 14.0f,
				BBHUD::ConsumableShortLabel(Inv.Backpack[i].ConsumableID), FLinearColor::White, 0.72f);
			DrawTextLine(SlotX + 4.0f, Row1Y + SlotSize - 12.0f,
				FString::Printf(TEXT("x%d"), Inv.Backpack[i].Count), FLinearColor::White, 0.9f);
		}
	}

	// --- Equipment slots: Weapon, Helmet, Boots, Armor ---
	struct FEqDisplay { FString Label; bool bFilled; FLinearColor Color; };
	TArray<FEqDisplay> Eq;
	Eq.Add({ TEXT("Wpn"),  !Inv.Weapon.IsEmpty(), FLinearColor(0.85f, 0.55f, 0.20f, 0.90f) });
	Eq.Add({ TEXT("Hlm"),  !Inv.Helmet.IsEmpty(), FLinearColor(0.55f, 0.55f, 0.85f, 0.90f) });
	Eq.Add({ TEXT("Bts"),  !Inv.Boots.IsEmpty(),  FLinearColor(0.40f, 0.75f, 0.40f, 0.90f) });
	Eq.Add({ TEXT("Arm"),  Inv.Armor.HasArmor(),  BBHUD::ArmorTierColor(Inv.Armor.Tier) });

	for (int32 i = 0; i < 4; ++i)
	{
		const float SlotX = StartX + i * (SlotSize + SlotGap);
		const FLinearColor Color = Eq[i].bFilled ? Eq[i].Color : FLinearColor(0.20f, 0.20f, 0.25f, 0.85f);
		DrawPanel(SlotX, Row2Y, SlotSize, SlotSize, Color);
		const FLinearColor Border(0.6f, 0.6f, 0.6f, 0.9f);
		DrawPanel(SlotX, Row2Y, SlotSize, 1.5f, Border);
		DrawPanel(SlotX, Row2Y + SlotSize - 1.5f, SlotSize, 1.5f, Border);
		DrawPanel(SlotX, Row2Y, 1.5f, SlotSize, Border);
		DrawPanel(SlotX + SlotSize - 1.5f, Row2Y, 1.5f, SlotSize, Border);

		DrawTextLine(SlotX + 3.0f, Row2Y + 2.0f, Eq[i].Label, FLinearColor::White, 0.95f);

		if (i == 3 && Inv.Armor.HasArmor())
		{
			const float ArmorMax = BBHUD::ArmorMaxHP(Inv.Armor.Tier);
			const float ArmorPct = ArmorMax > 0.0f ? FMath::Clamp(Inv.Armor.CurrentHP / ArmorMax, 0.0f, 1.0f) : 0.0f;
			DrawTextLine(SlotX + 4.0f, Row2Y + 14.0f,
				ArmorPct <= 0.0f ? TEXT("BROK") : BBHUD::ArmorTierLabel(Inv.Armor.Tier).Left(4),
				FLinearColor::White, 0.72f);
			DrawBar(SlotX + 4.0f, Row2Y + SlotSize - 8.0f, SlotSize - 8.0f, 4.0f, ArmorPct,
				BBHUD::ArmorTierColor(Inv.Armor.Tier), FLinearColor(0.05f, 0.05f, 0.06f, 0.90f));
		}
	}

	if (Inv.Armor.HasArmor())
	{
		const float ArmorMax = BBHUD::ArmorMaxHP(Inv.Armor.Tier);
		const float ArmorPct = ArmorMax > 0.0f ? FMath::Clamp(Inv.Armor.CurrentHP / ArmorMax, 0.0f, 1.0f) : 0.0f;
		const float ArmorBarX = StartX;
		const float ArmorBarY = Row2Y + SlotSize + 6.0f;
		DrawTextLine(ArmorBarX, ArmorBarY - 2.0f,
			FString::Printf(TEXT("%s Armor %.0f/%.0f"), *BBHUD::ArmorTierLabel(Inv.Armor.Tier), Inv.Armor.CurrentHP, ArmorMax),
			ArmorPct <= 0.0f ? FLinearColor(1.0f, 0.18f, 0.10f, 0.95f) : BBHUD::ArmorTierColor(Inv.Armor.Tier),
			0.85f);
		DrawBar(ArmorBarX, ArmorBarY + 14.0f, RowsTotalW, 6.0f, ArmorPct,
			BBHUD::ArmorTierColor(Inv.Armor.Tier), FLinearColor(0.05f, 0.05f, 0.06f, 0.90f));
	}

	// --- Shards + Gold (right of the equipment grid, stacked) ---
	const float SideX = StartX + RowsTotalW + 14.0f;
	DrawTextLine(SideX, Row1Y + 4.0f, FString::Printf(TEXT("⚡ %d"), Inv.UpgradeShards),
		FLinearColor(0.50f, 0.85f, 1.00f), 1.4f);  // ⚡ unicode lightning
	DrawTextLine(SideX, Row1Y + 26.0f, FString::Printf(TEXT("Shards")),
		FLinearColor(0.50f, 0.85f, 1.00f), 0.85f);

	DrawTextLine(SideX, Row2Y + 4.0f, FString::Printf(TEXT("● %d"), Inv.Gold),
		FLinearColor(1.00f, 0.85f, 0.20f), 1.4f);  // ● gold coin (filled circle)
	DrawTextLine(SideX, Row2Y + 26.0f, FString::Printf(TEXT("Gold")),
		FLinearColor(1.00f, 0.85f, 0.20f), 0.85f);
}

// ----------------------------------------------------------------------------
// Top-Right: match status — teams/players/souls + knocks/revives/assists/damage
// ----------------------------------------------------------------------------

void ABBDebugHUD::DrawTopRightPanel()
{
	ABreachbornePlayerState* PS = GetOwnerPlayerState();
	if (!PS) return;

	ABreachborneGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABreachborneGameState>() : nullptr;

	const float ScreenW = Canvas->SizeX;
	const float PanelW = 220.0f;
	const float PanelX = ScreenW - PanelW - 16.0f;
	float PanelY = 16.0f;
	const float PanelPad = 8.0f;

	// --- Panel 1: teams alive / players alive / meteor souls ---
	{
		const int32 TeamsAlive = GS ? GS->GetAliveTeamCount() : 0;
		// Players alive: count PlayerStates whose LifeState is Alive. Cheap enough; cache later if needed.
		int32 PlayersAlive = 0;
		if (UWorld* W = GetWorld())
		{
			for (TActorIterator<ABreachbornePlayerState> It(W); It; ++It)
			{
				if (*It && (*It)->GetIsAlive()) ++PlayersAlive;
			}
		}

		const float Panel1H = 56.0f;
		DrawPanel(PanelX, PanelY, PanelW, Panel1H, FLinearColor(0.05f, 0.05f, 0.08f, 0.65f));

		float TextX = PanelX + PanelPad;
		DrawTextLine(TextX, PanelY + 6.0f, FString::Printf(TEXT("Teams: %d"), TeamsAlive), FLinearColor::White, 1.1f);
		DrawTextLine(TextX + 100.0f, PanelY + 6.0f, FString::Printf(TEXT("Alive: %d"), PlayersAlive), FLinearColor::White, 1.1f);

		// Meteor souls — placeholder: 2 empty circles (drawn as ring rectangles for now).
		DrawTextLine(TextX, PanelY + 26.0f, TEXT("Souls"), FLinearColor(0.85f, 0.85f, 0.85f), 0.95f);
		const float SoulX = TextX + 60.0f;
		const float SoulY = PanelY + 28.0f;
		const float SoulSize = 14.0f;
		const FLinearColor SoulRing(0.5f, 0.5f, 0.6f, 0.9f);
		// Slot 1
		DrawPanel(SoulX,       SoulY, SoulSize, 1.5f, SoulRing);
		DrawPanel(SoulX,       SoulY + SoulSize - 1.5f, SoulSize, 1.5f, SoulRing);
		DrawPanel(SoulX,       SoulY, 1.5f, SoulSize, SoulRing);
		DrawPanel(SoulX + SoulSize - 1.5f, SoulY, 1.5f, SoulSize, SoulRing);
		// Slot 2
		DrawPanel(SoulX + SoulSize + 6.0f, SoulY, SoulSize, 1.5f, SoulRing);
		DrawPanel(SoulX + SoulSize + 6.0f, SoulY + SoulSize - 1.5f, SoulSize, 1.5f, SoulRing);
		DrawPanel(SoulX + SoulSize + 6.0f, SoulY, 1.5f, SoulSize, SoulRing);
		DrawPanel(SoulX + SoulSize + 6.0f + SoulSize - 1.5f, SoulY, 1.5f, SoulSize, SoulRing);

		PanelY += Panel1H + 6.0f;
	}

	// --- Panel 2: knocks / revives / assists / damage ---
	{
		// Revives, Assists, Damage are not tracked yet — show dashes / 0 as TODO.
		const int32 Knocks  = PS->GetKnocks();
		const int32 Revives = 0;  // TODO
		const int32 Assists = 0;  // TODO
		const int32 Damage  = 0;  // TODO

		const float Panel2H = 60.0f;
		DrawPanel(PanelX, PanelY, PanelW, Panel2H, FLinearColor(0.05f, 0.05f, 0.08f, 0.65f));

		const float TextX = PanelX + PanelPad;
		DrawTextLine(TextX,        PanelY + 4.0f, FString::Printf(TEXT("KO %d"),  Knocks),  FLinearColor(1.0f, 0.6f, 0.3f), 1.05f);
		DrawTextLine(TextX + 60.0f, PanelY + 4.0f, FString::Printf(TEXT("REV %d"), Revives), FLinearColor(0.4f, 0.95f, 0.5f), 1.05f);
		DrawTextLine(TextX + 130.0f, PanelY + 4.0f, FString::Printf(TEXT("AST %d"), Assists), FLinearColor(0.7f, 0.7f, 0.95f), 1.05f);

		DrawTextLine(TextX, PanelY + 26.0f, TEXT("Damage dealt:"), FLinearColor(0.85f, 0.85f, 0.85f), 0.95f);
		DrawTextLine(TextX + 110.0f, PanelY + 26.0f, FString::Printf(TEXT("%d"), Damage),
			FLinearColor(1.0f, 0.4f, 0.3f), 1.2f);
		DrawTextLine(TextX, PanelY + 44.0f, TEXT("(TODO: track DMG/REV/AST)"),
			FLinearColor(0.55f, 0.55f, 0.55f), 0.78f);
	}
}

// ----------------------------------------------------------------------------
// Top-Left: minimap placeholder, day/night, storm, team portraits placeholder
// ----------------------------------------------------------------------------

void ABBDebugHUD::DrawTopLeftPanel()
{
	ABreachborneGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABreachborneGameState>() : nullptr;

	const float PanelX = 16.0f;
	float PanelY = 16.0f;

	// --- Minimap placeholder ---
	const float MinimapSize = 140.0f;
	DrawPanel(PanelX, PanelY, MinimapSize, MinimapSize, FLinearColor(0.05f, 0.05f, 0.08f, 0.75f));
	const FLinearColor Border(0.5f, 0.5f, 0.6f, 0.9f);
	DrawPanel(PanelX, PanelY, MinimapSize, 1.5f, Border);
	DrawPanel(PanelX, PanelY + MinimapSize - 1.5f, MinimapSize, 1.5f, Border);
	DrawPanel(PanelX, PanelY, 1.5f, MinimapSize, Border);
	DrawPanel(PanelX + MinimapSize - 1.5f, PanelY, 1.5f, MinimapSize, Border);
	DrawTextLine(PanelX + MinimapSize * 0.5f - 28.0f, PanelY + MinimapSize * 0.5f - 6.0f,
		TEXT("MINIMAP"), FLinearColor(0.55f, 0.55f, 0.55f), 1.0f);
	DrawTextLine(PanelX + MinimapSize * 0.5f - 18.0f, PanelY + MinimapSize * 0.5f + 8.0f,
		TEXT("(TODO)"), FLinearColor(0.45f, 0.45f, 0.45f), 0.85f);
	PanelY += MinimapSize + 6.0f;

	// --- Day/Night row ---
	{
		const int32 Cycle = GS ? GS->GetDayNightCycle() : 0;
		const bool bIsNight = GS ? GS->IsNight() : false;
		const FString PhaseLabel = bIsNight ? TEXT("NIGHT") : TEXT("DAY");
		const FLinearColor PhaseColor = bIsNight ? FLinearColor(0.40f, 0.40f, 0.95f) : FLinearColor(1.00f, 0.85f, 0.30f);

		// "Day N" + phase tag
		DrawTextLine(PanelX, PanelY, FString::Printf(TEXT("Day %d"), Cycle + 1),
			FLinearColor::White, 1.2f);
		DrawTextLine(PanelX + 70.0f, PanelY, PhaseLabel, PhaseColor, 1.2f);

		// Phase progress bar — empty until GameState exposes per-phase elapsed time.
		// For now it's a static placeholder with a TODO label so you can see where it'll go.
		const float PhaseBarY = PanelY + 18.0f;
		const float PhaseBarW = MinimapSize;
		DrawBar(PanelX, PhaseBarY, PhaseBarW, 6.0f, 0.0f, PhaseColor,
			FLinearColor(0.10f, 0.10f, 0.12f, 0.7f));
		DrawTextLine(PanelX, PhaseBarY + 8.0f, TEXT("phase timer (TODO)"),
			FLinearColor(0.55f, 0.55f, 0.55f), 0.78f);
		PanelY = PhaseBarY + 22.0f;
	}

	// --- Storm phase ---
	{
		ABBStormManager* Storm = FindStormManager();
		const FString StormText = Storm
			? FString::Printf(TEXT("Storm: phase active"))
			: TEXT("Storm: idle");
		DrawTextLine(PanelX, PanelY, StormText, FLinearColor(0.75f, 0.45f, 1.00f), 1.0f);
		PanelY += 18.0f;
	}

	// --- Team portraits placeholder ---
	{
		// Up to 4 portraits stacked. Show name + HP percentage when we can identify squadmates.
		// For now, just show this player's own entry as a starting point — extend when squad
		// roster is queryable client-side.
		ABreachbornePlayerState* OwnPS = GetOwnerPlayerState();
		if (!OwnPS) return;

		const float PortraitH = 28.0f;
		const float PortraitW = MinimapSize;

		DrawPanel(PanelX, PanelY, PortraitW, PortraitH, FLinearColor(0.05f, 0.05f, 0.08f, 0.65f));
		// Mini HP bar
		const FRepHealthData& HP = OwnPS->GetHealthData();
		const float HPPct = HP.MaxHealth > 0.0f ? FMath::Clamp(HP.Health / HP.MaxHealth, 0.0f, 1.0f) : 0.0f;
		DrawBar(PanelX + 4.0f, PanelY + PortraitH - 8.0f, PortraitW - 8.0f, 4.0f, HPPct,
			FLinearColor(0.25f, 0.95f, 0.30f), FLinearColor(0.10f, 0.10f, 0.10f, 0.7f));
		DrawTextLine(PanelX + 4.0f, PanelY + 4.0f, OwnPS->GetPlayerName(), FLinearColor::White, 0.95f);
		DrawTextLine(PanelX + PortraitW - 36.0f, PanelY + 4.0f,
			FString::Printf(TEXT("LV%d"), OwnPS->GetHunterLevel()),
			FLinearColor(1.0f, 0.85f, 0.30f), 0.95f);
		PanelY += PortraitH + 4.0f;

		DrawTextLine(PanelX, PanelY, TEXT("(squadmates: TODO)"), FLinearColor(0.50f, 0.50f, 0.50f), 0.78f);
	}
}

// ============================================================================
// 3D Debug Shapes
// ============================================================================

void ABBDebugHUD::DrawDebugShapes3D()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// --- Wisp Radii ---
	for (TActorIterator<ABBWispPawn> It(World); It; ++It)
	{
		ABBWispPawn* Wisp = *It;
		if (!Wisp)
		{
			continue;
		}

		const FVector WispLoc = Wisp->GetActorLocation();

		// Proximity radius (green)
		DrawDebugCircle(World, WispLoc, Wisp->GetProximityRadius(), 48,
			FColor::Green, false, -1.0f, 0, 2.0f, FVector::RightVector, FVector::ForwardVector, false);

		// Execute radius (red)
		DrawDebugCircle(World, WispLoc, Wisp->GetExecuteRadius(), 32,
			FColor::Red, false, -1.0f, 0, 2.0f, FVector::RightVector, FVector::ForwardVector, false);
	}

	// --- Storm Distance Line ---
	ABBStormManager* Storm = FindStormManager();
	APlayerController* PC = GetOwningPlayerController();
	APawn* OwnedPawn = PC ? PC->GetPawn() : nullptr;
	if (Storm && OwnedPawn)
	{
		const FVector PawnLoc = OwnedPawn->GetActorLocation();
		const FVector StormCenter = Storm->GetActorLocation();
		const FVector StormCenter2D = FVector(StormCenter.X, StormCenter.Y, PawnLoc.Z);
		const float Dist2D = FVector::Dist2D(PawnLoc, StormCenter2D);

		// Green line if close to storm center (inside), red if far (outside)
		// We don't have direct access to radius from here, so use distance as a rough indicator
		const FColor LineColor = Dist2D < 5000.0f ? FColor::Green : FColor::Red;
		DrawDebugLine(World, PawnLoc, StormCenter2D, LineColor, false, -1.0f, 0, 2.0f);
	}
}
