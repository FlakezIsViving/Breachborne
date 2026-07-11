#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BBDebugHUD.generated.h"

class ABreachbornePlayerState;
class ABreachborneGameState;
class ABBStormManager;
class ABBBasecampActor;
class ABBWispPawn;
class AHunterCharacter;
class UBBGameplayAbility;
class UAbilitySystemComponent;
struct FRepHealthData;
struct FRepCombatData;
struct FRepMovementData;
struct FRepWispData;
struct FRepInventoryData;
struct FGameplayTag;

/**
 * Debug + gameplay HUD drawn via Canvas (no UMG required).
 * UE creates AHUD only on clients — safe for dedicated server.
 *
 * Console commands:
 *   DebugHUD    — toggle debug text overlay (top-left panel)
 *   ToggleHUD   — toggle gameplay health/economy bars (bottom-center)
 *   DebugShapes — toggle 3D debug circles around wisps + storm distance line
 */
UCLASS()
class BREACHBORNE_API ABBDebugHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABBDebugHUD();

	virtual void DrawHUD() override;

	/** Console command: toggle debug text overlay */
	UFUNCTION(Exec)
	void DebugHUD();

	/** Console command: toggle gameplay HUD bars */
	UFUNCTION(Exec)
	void ToggleHUD();

	/** Console command: toggle 3D debug shapes */
	UFUNCTION(Exec)
	void DebugShapes();

private:
	// --- Feature Toggles ---
	bool bShowDebugOverlay = false;
	bool bShowGameplayHUD = true;
	bool bShowDebugShapes = false;

	// --- Data Access Helpers ---
	ABreachbornePlayerState* GetOwnerPlayerState() const;
	AHunterCharacter* GetOwnerHunter() const;
	ABBWispPawn* GetOwnerWisp() const;
	ABBStormManager* FindStormManager() const;

	// --- Canvas Drawing Helpers ---

	/** Draw a semi-transparent background rect */
	void DrawPanel(float X, float Y, float W, float H, const FLinearColor& Color) const;

	/** Draw a text line at the given position. Returns the Y advance. */
	float DrawTextLine(float X, float Y, const FString& Text, const FLinearColor& Color = FLinearColor::White, float Scale = 1.0f) const;

	/** Draw a horizontal bar with fill amount (0-1) */
	void DrawBar(float X, float Y, float W, float H, float FillPct, const FLinearColor& FillColor, const FLinearColor& BgColor) const;

	// --- Drawing Systems ---

	/** Debug overlay: semi-transparent panel with all stats (top-left) */
	void DrawDebugOverlay();

	/** Gameplay HUD top-level: dispatches to the four corner panels. */
	void DrawGameplayHUD();

	// Four corner panels — each pulls its own data from PS/GS/ASC.
	void DrawBottomLeftPanel();   // Name, level, XP, HP, mana, abilities, F/G powers
	void DrawBottomRightPanel();  // Consumables, equipment, shards, gold
	void DrawTopRightPanel();     // Teams/players/souls + knocks/revives/assists/damage
	void DrawTopLeftPanel();      // Minimap placeholder, day/night, storm, team portraits
	void DrawBasecampIndicators();
	void DrawWorldItemPrompts();

	void DrawBasecampStationPrompt(const ABBBasecampActor* Basecamp, const FVector& WorldLocation,
		const FString& Label, const FLinearColor& Color) const;

	/** Draw a single ability icon-box with cooldown overlay. CornerLabel goes at top
	 *  (e.g. "RMB", "Shift", "Q"). SubLabel is a small tag in the corner (e.g. "A" / "G"
	 *  for Aerial/Ground dash). RemainingCD <= 0 means ability is ready (full color);
	 *  > 0 dims the box and overlays the remaining seconds. */
	void DrawAbilitySlot(float X, float Y, float Size, const FString& CornerLabel,
		const FString& SubLabel, float RemainingCD, float MaxCD,
		const FLinearColor& ReadyColor) const;

	/** HP rendered as 100-HP chunks left to right. Each chunk is one "block." */
	void DrawHPChunks(float X, float Y, float Width, float Height, float Health, float MaxHealth) const;

	/** Lookup the remaining cooldown time for the GE matching CooldownTag on ASC.
	 *  Returns 0 if no active cooldown. Out param OutDuration is the original total. */
	float GetCooldownRemaining(UAbilitySystemComponent* ASC, const FGameplayTag& CooldownTag, float& OutDuration) const;

	/** 3D debug shapes: wisp radii, storm distance */
	void DrawDebugShapes3D();

	// --- Cached Storm Manager ---
	mutable TWeakObjectPtr<ABBStormManager> CachedStormManager;
};
