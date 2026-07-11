#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBKnockupTagEffect.generated.h"

/**
 * Effect_Knockup tag GameplayEffect — applied by Kingpin's Iron Stomp (R) to enemies caught
 * in the shockwave radius. Briefly tags them as airborne so other systems (GliderComponent
 * spike detection, CC indicators) can query the state.
 *
 * Proper CDO-backed UGameplayEffect subclass (replication-safe across clients). See
 * BBHookTagEffect.h for the why behind avoiding NewObject<UGameplayEffect>() transients.
 *
 * Duration is set per-application via Spec.SetDuration() — the CDO default is a fallback.
 */
UCLASS()
class BREACHBORNE_API UBBKnockupTagEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBKnockupTagEffect();
};
