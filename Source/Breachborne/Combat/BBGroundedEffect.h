#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBGroundedEffect.generated.h"

/** Prevents glide/jump-style mobility through a tag for prototype Hudson wire readability. */
UCLASS()
class BREACHBORNE_API UBBGroundedEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBGroundedEffect();
};
