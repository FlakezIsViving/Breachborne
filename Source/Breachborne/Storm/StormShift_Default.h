#pragma once

#include "CoreMinimal.h"
#include "StormShiftBase.h"
#include "StormShift_Default.generated.h"

/**
 * Default storm shift: 4 escalating phases shrinking toward map center.
 * Phase 1: gentle shrink + low damage. Phase 4: full collapse + lethal damage.
 */
UCLASS()
class BREACHBORNE_API UStormShift_Default : public UStormShiftBase
{
	GENERATED_BODY()

public:
	virtual FName GetShiftName() const override { return FName(TEXT("Default")); }
	virtual TArray<FStormPhaseConfig> GetPhaseConfigs(float InitialRadius) const override;
	virtual FVector2D PickNextCenter(int32 PhaseIndex, const FVector2D& CurrentCenter, float NewRadius) const override;
};
