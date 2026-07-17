#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BBAbilityVisualSet.generated.h"

class UAnimInstance;
class UAnimMontage;
class UMaterialInterface;
class UNiagaraSystem;
class UParticleSystem;
class USkeletalMesh;
class UStaticMesh;
class USoundBase;

UENUM(BlueprintType)
enum class EBBAbilityAnimationPhase : uint8
{
	Start,
	Loop,
	Fire,
	Toss,
	End,
	Impact,
	PassivePulse,
	Success,
	Cancel
};

/** Shared low-cost visual shapes. Cue assets should be instances of one of these templates. */
UENUM(BlueprintType)
enum class EBBVFXTemplateType : uint8
{
	None,
	ProjectileBody,
	ProjectileTrail,
	BeamTether,
	GroundTelegraph,
	PersistentGroundZone,
	ConeWedge,
	ImpactBurst,
	CharacterAura
};

USTRUCT(BlueprintType)
struct BREACHBORNE_API FBBVFXPalette
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Palette")
	FLinearColor PrimaryColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Palette")
	FLinearColor SecondaryColor = FLinearColor(0.65f, 0.70f, 0.78f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Palette")
	FLinearColor EmpoweredColor = FLinearColor(1.0f, 0.85f, 0.25f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Palette")
	FLinearColor FriendlyRelationColor = FLinearColor(0.30f, 0.79f, 0.94f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Palette")
	FLinearColor EnemyRelationColor = FLinearColor(1.0f, 0.30f, 0.35f, 1.0f);
};

/** Niagara/user-parameter defaults shared by every cue template. */
USTRUCT(BlueprintType)
struct BREACHBORNE_API FBBVFXParameterDefaults
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Parameters", meta = (ClampMin = "0.0"))
	float Radius = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Parameters", meta = (ClampMin = "0.0"))
	float Length = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Parameters", meta = (ClampMin = "0.0"))
	float Width = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Parameters")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Parameters", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Progress = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Parameters", meta = (ClampMin = "0.0"))
	float Intensity = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Parameters")
	bool bEmpowered = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Parameters")
	bool bEnemy = false;
};

/**
 * Low-spec budget metadata for a single visual cue.
 * Designers should treat these values as acceptance criteria for 1080p Low/Medium
 * on older PC hardware, not as optional notes.
 */
USTRUCT(BlueprintType)
struct BREACHBORNE_API FBBVisualBudget
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Budget", meta = (ClampMin = "1"))
	int32 MaxActiveInstances = 16;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Budget", meta = (ClampMin = "0.0"))
	float MaxDrawDistance = 3000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Budget")
	bool bCullWhenOffscreen = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Budget")
	bool bAllowLowQualityFallback = true;
};

/**
 * Asset references for one GameplayCue phase.
 * GameplayCue Blueprints own the actual spawn behavior; this data asset documents and
 * centralizes the intended low-cost assets/fallbacks for each cue.
 */
USTRUCT(BlueprintType)
struct BREACHBORNE_API FBBGameplayCueVisual
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Cue")
	FGameplayTag CueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Cue")
	bool bLooping = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Template")
	EBBVFXTemplateType TemplateType = EBBVFXTemplateType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Cue", meta = (ClampMin = "0.0"))
	float LifetimeSeconds = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Assets")
	TObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Assets")
	TObjectPtr<UParticleSystem> CascadeFallback = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Assets")
	TObjectPtr<UMaterialInterface> DecalMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Assets")
	TObjectPtr<UStaticMesh> MeshFallback = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Assets")
	TObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Budget")
	FBBVisualBudget Budget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Parameters")
	FBBVFXParameterDefaults ParameterDefaults;
};

USTRUCT(BlueprintType)
struct BREACHBORNE_API FBBAbilityMontageVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|Visuals|Animation")
	EBBAbilityAnimationPhase Phase = EBBAbilityAnimationPhase::Start;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|Visuals|Animation")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|Visuals|Animation", meta = (ClampMin = "0.01"))
	float PlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|Visuals|Animation")
	bool bLooping = false;
};

/** Per-input/ability visual bundle. Match by Ability.Hunter.* tag first, then InputTag.*. */
USTRUCT(BlueprintType)
struct BREACHBORNE_API FBBAbilitySlotVisuals
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|Visuals|Ability")
	FGameplayTag AbilityOrInputTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|Visuals|Animation")
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|Visuals|Animation")
	TArray<FBBAbilityMontageVisual> PhaseMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|Visuals|Sockets")
	FName PrimarySocketName = TEXT("Muzzle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|Visuals|Sockets")
	FName SecondarySocketName = TEXT("Effect");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|Visuals|Cue")
	TArray<FBBGameplayCueVisual> CueVisuals;
};

/**
 * Data-driven visual contract for a hunter or power.
 * This is intentionally content-facing: gameplay code reads tags/sockets/montages,
 * while GameplayCue assets decide the concrete, scalable effect implementation.
 */
UCLASS(BlueprintType)
class BREACHBORNE_API UBBAbilityVisualSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Palette")
	FBBVFXPalette Palette;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Character")
	TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Character")
	TSubclassOf<UAnimInstance> AnimInstanceClass;

	/** Every ability montage must use this AnimGraph slot. None disables slot validation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Animation")
	FName RequiredMontageSlotName = TEXT("DefaultSlot");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Character")
	FTransform MeshRelativeTransform = FTransform(FRotator(0.0f, -90.0f, 0.0f), FVector(0.0f, 0.0f, -90.0f), FVector(1.0f));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Character")
	TObjectPtr<UStaticMesh> DebugFallbackMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Performance")
	bool bPreferLowSpecDefaults = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Visuals|Abilities")
	TArray<FBBAbilitySlotVisuals> AbilityVisuals;

	UFUNCTION(BlueprintPure, Category = "Breachborne|Visuals")
	const FBBAbilitySlotVisuals& FindAbilityVisuals(FGameplayTag AbilityOrInputTag, bool& bFound) const;

	UFUNCTION(BlueprintPure, Category = "Breachborne|Visuals")
	UAnimMontage* FindMontage(FGameplayTag AbilityOrInputTag) const;

	UFUNCTION(BlueprintPure, Category = "Breachborne|Visuals")
	UAnimMontage* FindPhaseMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float& OutPlayRate, bool& bOutLooping) const;

	UAnimMontage* FindMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float& OutPlayRate, bool& bOutLooping) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
