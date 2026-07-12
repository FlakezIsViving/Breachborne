#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBReverberationMarkEffect.generated.h"

/** Crysta's Reverberation mark. Detonated by Crysta LMB/R. */
UCLASS()
class BREACHBORNE_API UBBReverberationMarkEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBReverberationMarkEffect();
};
