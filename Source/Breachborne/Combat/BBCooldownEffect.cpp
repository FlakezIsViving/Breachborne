#include "BBCooldownEffect.h"

UBBCooldownEffect::UBBCooldownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	// Duration and granted cooldown tag are set by subclasses or per-ability configuration
}
