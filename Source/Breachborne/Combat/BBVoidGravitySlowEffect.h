#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBVoidGravitySlowEffect.generated.h"

/** Light slow from Void's fully charged Gravity Orb. */
UCLASS()
class BREACHBORNE_API UBBVoidGravitySlowEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBVoidGravitySlowEffect();
};
