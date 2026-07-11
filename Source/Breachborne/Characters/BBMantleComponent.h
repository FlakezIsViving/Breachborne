#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BBMantleComponent.generated.h"

class UBBCharacterMovementComponent;
class UAbilitySystemComponent;
class UPrimitiveComponent;
struct FBBMantleTarget;

/**
 * Abyss mantle recovery.
 *
 * A falling hunter who contacts opt-in island terrain below the walkable top
 * is pulled along a swept diagonal path to a validated landing point.
 */
UCLASS(ClassGroup = "Breachborne", meta = (BlueprintSpawnableComponent))
class BREACHBORNE_API UBBMantleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBBMantleComponent();

	UFUNCTION(BlueprintCallable, Category = "Breachborne|Mantle")
	bool TryMantle();

	UFUNCTION(Server, Reliable)
	void ServerRequestMantle();

	UFUNCTION(BlueprintPure, Category = "Breachborne|Mantle")
	bool IsMantling() const;

	/** True when a recent wall contact can become a mantle; used to suppress glider auto-open. */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Mantle")
	bool HasRecentMantleCandidate() const;

	void NotifyMantleFinished();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	bool ValidateAndTriggerMantle(const FHitResult& Hit);
	bool BuildMantleTarget(const FHitResult& Hit, FBBMantleTarget& OutTarget, FName& OutRejectReason) const;
	bool IsRecoverableSurface(const FHitResult& Hit) const;
	bool IsMovementStateEligible() const;
	bool ValidateLanding(const FVector& LandingCenter, float CapsuleRadius, float CapsuleHalfHeight, FName& OutRejectReason) const;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	float MaxTopSearchHeight = 2200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	float MinMantleDepth = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	float MaxMantleDepth = 2200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	float LandingInset = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	float LandingSideRetryOffset = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	float LandingDeepRetryOffset = 140.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	float LandingVerticalClearance = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	float MantleDuration = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	float FailedAttemptCooldown = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_WorldStatic;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	TEnumAsByte<ECollisionChannel> CapsuleSweepChannel = ECC_Pawn;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	FName RecoverableActorTag = TEXT("MantleRecoverable");

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Mantle")
	FName RecoverableCollisionProfile = TEXT("MantleRecoverable");

private:
	UPROPERTY()
	TObjectPtr<UBBCharacterMovementComponent> BBMovement;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> ASC;

	void SetMantlingTag(bool bActive);
	void CancelGliderForMantle() const;
	void MarkFailedAttempt() const;

	mutable float LastFailedAttemptTime = -1000.0f;
	uint32 LastMantleTriggerFrame = 0;
};
