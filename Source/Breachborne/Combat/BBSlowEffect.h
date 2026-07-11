#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBSlowEffect.generated.h"

/** Generic slow effect used by Hudson's readable prototype zones and hook. */
UCLASS()
class BREACHBORNE_API UBBSlowEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBSlowEffect();
};
