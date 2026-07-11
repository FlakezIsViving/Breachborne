#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBStunTagEffect.generated.h"

/**
 * State_Stunned tag GameplayEffect — applied by Kingpin's Headcracker (Q) to enemies caught
 * in the slam cone.
 *
 * Proper CDO-backed UGameplayEffect subclass (replication-safe across clients). See
 * BBHookTagEffect.h for the why behind avoiding NewObject<UGameplayEffect>() transients.
 *
 * Duration is set per-application via Spec.SetDuration() — the CDO default is a fallback.
 */
UCLASS()
class BREACHBORNE_API UBBStunTagEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBStunTagEffect();
};
