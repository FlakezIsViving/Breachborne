#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBDynamicStatGE.generated.h"

/**
 * C++ UGameplayEffect subclass with SetByCaller modifiers for all 7 stat attributes.
 * Used by FBBEquipmentGEHelper to apply equipment/power stat bonuses.
 *
 * Unlike NewObject<UGameplayEffect>() instances, this class has a CDO that both
 * server and client can resolve via the net GUID cache, fixing replication of
 * Infinite-duration Active Gameplay Effects.
 *
 * Callers create a spec from this class via MakeOutgoingSpec and set magnitudes
 * via SetByCaller tags (SetByCaller.AttackPower, SetByCaller.MaxHealth, etc.).
 * Zero magnitudes produce no attribute change (additive +0).
 */
UCLASS()
class BREACHBORNE_API UBBDynamicStatGE : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBDynamicStatGE();
};
