#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "BBAbilityVisualSet.h"
#include "BBGameplayTags.h"
#include "BBGameplayAbility.generated.h"

class AHunterCharacter;
class ABreachbornePlayerState;
class UBBAbilitySystemComponent;

UENUM(BlueprintType)
enum class EBBRangeIndicatorMode : uint8
{
	Disabled,
	Directional,
	TargetedArea,
	SelfCentered,
	Movement
};

USTRUCT(BlueprintType)
struct BREACHBORNE_API FBBAbilityRangeIndicatorInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range Indicator")
	EBBRangeIndicatorMode Mode = EBBRangeIndicatorMode::Disabled;

	/** Maximum distance from the caster to the target/center/landing point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range Indicator", meta = (ClampMin = "0.0"))
	float CastRange = 0.0f;

	/** Radius around the target point or caster. Zero means no area footprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Range Indicator", meta = (ClampMin = "0.0"))
	float EffectRadius = 0.0f;

	bool IsEnabled() const
	{
		return Mode != EBBRangeIndicatorMode::Disabled && (CastRange > 0.0f || EffectRadius > 0.0f);
	}
};

/**
 * Base class for all Breachborne abilities.
 * Provides input tag binding, blocking state tag checks, and helper accessors.
 * Instanced per actor so each player gets their own ability state.
 */
UCLASS(Abstract)
class BREACHBORNE_API UBBGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBBGameplayAbility();

	/** The Enhanced Input tag that activates this ability */
	FGameplayTag GetAbilityInputTag() const { return AbilityInputTag; }

	/** Whether this ability should stay active while input is held (e.g., LMB auto-fire) */
	bool ShouldActivateOnInputHeld() const { return bActivateOnInputHeld; }

	/** Expose the net execution policy for external checks (e.g., GameMode auto-activating passives) */
	EGameplayAbilityNetExecutionPolicy::Type GetBBNetExecutionPolicy() const { return NetExecutionPolicy; }

	/** Called by ASC when the input tag for this ability is released */
	virtual void OnInputReleased();

	/** Local-only HUD metadata. This does not participate in gameplay authority. */
	const FBBAbilityRangeIndicatorInfo& GetRangeIndicatorInfo() const { return RangeIndicatorInfo; }

	/** Build a network-safe cooldown spec with runtime duration and granted tags. */
	static FGameplayEffectSpecHandle BuildBBCooldownSpec(UAbilitySystemComponent* ASC, float Duration,
		const FGameplayTagContainer& GrantedTags, float AbilityLevel = 1.0f);

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// --- Helper Accessors ---

	/** Get the HunterCharacter avatar (the pawn). Returns nullptr if not a HunterCharacter. */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Ability")
	AHunterCharacter* GetHunterCharacter() const;

	/** Get the Breachborne PlayerState that owns this ability's ASC */
	ABreachbornePlayerState* GetBBPlayerState() const;

	/** Get the typed ASC */
	UBBAbilitySystemComponent* GetBBAbilitySystemComponent() const;

	/** Get the world-space aim location from the owning character's replicated aim point */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Ability")
	FVector GetAimLocation() const;

	/** Get the normalized aim direction from character position to aim location (2D, Z=0) */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Ability")
	FVector GetAimDirection() const;

	/** Apply a cooldown using a transient cooldown GE */
	void ApplyBBCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, float Duration) const;

	/** Apply a cooldown with explicit granted tags while keeping GetCooldownTags limited to activation checks. */
	void ApplyBBCooldownWithTags(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, float Duration, const FGameplayTagContainer& GrantedTags) const;

	/** Fire a one-shot GameplayCue through the owning ASC. */
	void ExecuteVisualCue(FGameplayTag CueTag, const FVector& Location = FVector::ZeroVector, const FVector& Normal = FVector::UpVector) const;

	/** Fire a one-shot beam cue. Location is the start, Normal is its direction, and RawMagnitude is its length. */
	void ExecuteBeamVisualCue(FGameplayTag CueTag, const FVector& StartLocation, const FVector& EndLocation) const;

	/** Start a looping GameplayCue through the owning ASC. Call RemoveVisualCue when the loop ends. */
	void AddVisualCue(FGameplayTag CueTag, const FVector& Location = FVector::ZeroVector, const FVector& Normal = FVector::UpVector) const;

	/** Stop a looping GameplayCue through the owning ASC. */
	void RemoveVisualCue(FGameplayTag CueTag) const;

	/** Play the configured montage from the character's visual set for an ability/input tag. */
	float PlayVisualMontage(FGameplayTag AbilityOrInputTag, float PlayRate = 1.0f) const;

	/** Play a specific animation phase from the character's visual set for an ability/input tag. */
	float PlayVisualMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float PlayRateOverride = 0.0f) const;

	/** Stop a specific animation phase from the character's visual set for an ability/input tag. */
	void StopVisualMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float BlendOutTime = 0.15f) const;

	/** Configure the shared hover/aim range preview for this ability. */
	void ConfigureRangeIndicator(EBBRangeIndicatorMode Mode, float CastRange, float EffectRadius = 0.0f);

	// --- Configuration ---

	/** The input tag that maps this ability to an Enhanced Input action */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input")
	FGameplayTag AbilityInputTag;

	/** If true, ability stays active while input is held (for auto-fire abilities like LMB) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Input")
	bool bActivateOnInputHeld = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|UI")
	FBBAbilityRangeIndicatorInfo RangeIndicatorInfo;
};
