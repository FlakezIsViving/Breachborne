#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BBCooldownEffect.generated.h"

/**
 * Base cooldown GameplayEffect. Abilities override GetCooldownGameplayEffect() to return
 * instances of this class with the appropriate duration and cooldown tag.
 * Duration and granted tag are configured per-ability in the constructor or via subclass.
 */
UCLASS()
class BREACHBORNE_API UBBCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBBCooldownEffect();
};
