#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Breachborne/Core/BreachborneTypes.h"
#include "BBHunterDefinition.generated.h"

class UBBGameplayAbility;
class UBBAbilityVisualSet;
class UAnimInstance;
class UAnimMontage;
class USkeletalMesh;

/**
 * Data asset defining a Hunter's identity: role, base stats, and granted abilities.
 * One per hunter (e.g., DA_Ghost, DA_Kingpin). Configured in the editor.
 * GameMode looks these up by HunterID and grants all listed abilities on PostLogin.
 */
UCLASS(BlueprintType)
class BREACHBORNE_API UBBHunterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Display name for this hunter */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter")
	FText HunterName;

	/** The hunter's role in team composition */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter")
	EHunterRole Role = EHunterRole::Fighter;

	/** Abilities granted to this hunter on match start (LMB, RMB, Shift, Q, R, Passive) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter")
	TArray<TSubclassOf<UBBGameplayAbility>> AbilitiesToGrant;

	// --- Optional Visuals ---

	/** Preferred data-driven visual bundle. Legacy fields below remain fallbacks for existing assets. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Visuals")
	TObjectPtr<UBBAbilityVisualSet> VisualSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Visuals")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Visuals")
	TSubclassOf<UAnimInstance> AnimInstanceClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Visuals")
	FTransform MeshRelativeTransform = FTransform(FRotator(0.0f, -90.0f, 0.0f), FVector(0.0f, 0.0f, -90.0f), FVector(1.0f));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Visuals")
	TObjectPtr<UAnimMontage> LMBMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Visuals")
	TObjectPtr<UAnimMontage> RMBMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Visuals")
	TObjectPtr<UAnimMontage> ShiftMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Visuals")
	TObjectPtr<UAnimMontage> QMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Visuals")
	TObjectPtr<UAnimMontage> RMontage;

	// --- Base Stats (applied via initial GE or direct attribute init) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Stats")
	float BaseHealth = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Stats")
	float BaseAttackPower = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Stats")
	float BaseAbilityPower = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Breachborne|Hunter|Stats")
	float BaseMoveSpeed = 600.0f;
};
