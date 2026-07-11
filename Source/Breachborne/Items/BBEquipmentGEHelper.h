#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"

struct FBBStatBonuses;
class UAbilitySystemComponent;
class UGameplayAbility;
struct FGameplayTag;

/**
 * Static helper functions for applying/removing equipment and power bonuses via GAS.
 * Equipment bonuses use programmatic Infinite-duration GEs with additive modifiers.
 * Powers grant abilities via ASC->GiveAbility().
 * All operations are SERVER ONLY — callers must check HasAuthority().
 */
class BREACHBORNE_API FBBEquipmentGEHelper
{
public:
	/**
	 * Create and apply a programmatic Infinite-duration GE with additive stat modifiers.
	 * Returns the active GE handle for later removal.
	 * @param ASC - The AbilitySystemComponent to apply to
	 * @param Stats - The stat bonuses to add
	 * @param EffectTag - A GameplayTag to identify this effect (e.g., Effect_Equipment_Weapon)
	 * @param DebugName - Name for the programmatic GE object (for ShowDebug)
	 */
	static FActiveGameplayEffectHandle ApplyStatBonuses(
		UAbilitySystemComponent* ASC,
		const FBBStatBonuses& Stats,
		const FGameplayTag& EffectTag,
		const FString& DebugName);

	/**
	 * Remove a previously applied equipment bonus GE.
	 * @param ASC - The AbilitySystemComponent to remove from
	 * @param Handle - The handle returned by ApplyStatBonuses
	 */
	static void RemoveStatBonuses(UAbilitySystemComponent* ASC, FActiveGameplayEffectHandle& Handle);

	/**
	 * Grant a power's ability to the ASC.
	 * @param ASC - The AbilitySystemComponent to grant to
	 * @param AbilityClass - The GameplayAbility class to grant
	 * @param SourceObject - The object that owns this grant (typically the PlayerState)
	 * @return The granted ability spec handle
	 */
	static FGameplayAbilitySpecHandle GrantPowerAbility(
		UAbilitySystemComponent* ASC,
		TSubclassOf<UGameplayAbility> AbilityClass,
		UObject* SourceObject,
		const FGameplayTag& InputTag);

	/**
	 * Remove a previously granted power ability.
	 * @param ASC - The AbilitySystemComponent to remove from
	 * @param Handle - The handle returned by GrantPowerAbility
	 */
	static void RemovePowerAbility(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle& Handle);
};
