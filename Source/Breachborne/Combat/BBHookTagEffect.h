#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBHookTagEffect.generated.h"

/**
 * State_Hooked tag GameplayEffect — applied by Kingpin's Iron Hook (RMB) to dragged targets.
 *
 * This is a proper UGameplayEffect subclass (not a NewObject<UGameplayEffect>() transient
 * instance) so the CDO has a stable net-GUID resolvable by every client. Without this,
 * cross-client replication of the Active GE would log
 *   "FNetGUIDCache::SupportsObject: GameplayEffect /Engine/Transient.GE_... NOT Supported"
 * and
 *   "FActiveGameplayEffect::PostReplicatedAdd Received ReplicatedGameplayEffect with no def".
 *
 * Duration is set per-application via Spec.SetDuration() — the CDO default below is just a
 * fallback; in practice the ability passes its own HookTagDuration.
 */
UCLASS()
class BREACHBORNE_API UBBHookTagEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBHookTagEffect();
};
