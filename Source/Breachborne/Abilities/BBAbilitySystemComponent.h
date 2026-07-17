#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "BBAbilitySystemComponent.generated.h"

/**
 * Custom AbilitySystemComponent for Breachborne.
 * Lives on APlayerState (survives pawn death/respawn).
 * Replication Mode: Mixed (full to owner, minimal tags+attributes to simulated proxies).
 * Provides input-tag-based ability activation for the Enhanced Input → GAS bridge.
 */
UCLASS()
class BREACHBORNE_API UBBAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UBBAbilitySystemComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Try to activate an ability that matches the given input tag.
	 * Called by PlayerController when an Enhanced Input action fires.
	 */
	bool TryActivateAbilityByInputTag(const FGameplayTag& InputTag);

	/** Route an active ability's second press to its authoritative instance. */
	UFUNCTION(Server, Reliable)
	void ServerInputTagPressed(FGameplayTag InputTag);

	/**
	 * Notify active abilities that the input associated with this tag was released.
	 * Used for held abilities (e.g., LMB auto-fire).
	 */
	void InputTagReleased(const FGameplayTag& InputTag);

	/**
	 * Cancel any active ability matching this input tag.
	 */
	void CancelAbilityByInputTag(const FGameplayTag& InputTag);

	/** Clear match-scoped effects, state/cooldown tags, and pressed input on this persistent ASC. */
	void ResetForNewMatch();

	/** Re-publish the authoritative ability list after a pawn/hunter grant transition. */
	void RefreshAbilitySpecReplication();

	/** Remove matching cooldown effects authoritatively and clear any predicted copy on the owner. */
	int32 RefundCooldownsByTags(const FGameplayTagContainer& CooldownTags);

private:
	UFUNCTION(Client, Reliable)
	void ClientRefundCooldownsByTags(FGameplayTagContainer CooldownTags);
};
