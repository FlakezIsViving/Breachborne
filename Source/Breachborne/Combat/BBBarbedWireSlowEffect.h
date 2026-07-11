#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBBarbedWireSlowEffect.generated.h"

/** Hudson barbed-wire slow: 25% movement reduction with State.Slowed. */
UCLASS()
class BREACHBORNE_API UBBBarbedWireSlowEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBBarbedWireSlowEffect();
};
