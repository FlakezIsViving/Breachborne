#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BBVisionConeComponent.generated.h"

/**
 * UBBVisionConeComponent — tracks each hunter's vision parameters.
 *
 * Prototype scope (per supervise-reference.md):
 * - Vision is CLIENT-SIDE rendering only — no server-side culling or fog-of-war
 * - No dedicated FogOfWarManager needed in prototype
 * - Component exposes FOV, range, and helper functions for Blueprint HUD rendering
 *
 * The component also drives the brush stealth check:
 * Server compares this cone against nearby brush volumes to determine whether
 * an enemy inside a brush is visible. This is the ONLY server-side computation:
 * a simple LOS/range check, not full FOW.
 *
 * Day-time: DayVisionRange
 * Night-time: NightVisionRange (reduced)
 * The component reads the GameState day/night sub-state each tick.
 */
UCLASS(ClassGroup = "Breachborne", meta = (BlueprintSpawnableComponent))
class BREACHBORNE_API UBBVisionConeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBBVisionConeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Returns true if WorldLocation is within this hunter's current vision cone */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Vision")
	bool IsInVisionCone(const FVector& WorldLocation) const;

	/** Current effective range (day or night, adjusted by State_InBrush) */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Vision")
	float GetEffectiveRange() const;

	/** Half-angle of the vision cone in degrees */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Vision")
	float GetHalfAngleDegrees() const { return VisionHalfAngle; }

protected:
	/** Vision range during daytime */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Vision")
	float DayVisionRange = 2000.0f;

	/** Vision range at night (reduced — gates level cap too) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Vision")
	float NightVisionRange = 1200.0f;

	/** Half-angle of the cone in degrees (60 = 120° total FOV) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Vision")
	float VisionHalfAngle = 60.0f;

	/** Range reduction when standing inside a brush (State_InBrush) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Vision")
	float BrushVisionMultiplier = 0.6f;

private:
	float CachedEffectiveRange = 2000.0f;
};
