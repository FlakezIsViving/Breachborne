#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Breachborne/Abilities/BBAbilityVisualSet.h"
#include "GameplayEffectTypes.h"
#include "HunterCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UAbilitySystemComponent;
class UAnimInstance;
class USkeletalMesh;
class ABreachbornePlayerState;
class UGliderComponent;
class UBBMantleComponent;
class UBBCharacterMovementComponent;
class UBBHunterDefinition;

/** One cue in a server-authored cosmetic batch. Clients invoke these locally. */
USTRUCT()
struct BREACHBORNE_API FBBLocalGameplayCue
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag CueTag;

	UPROPERTY()
	FVector_NetQuantize Location = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantizeNormal Normal = FVector::UpVector;
};

/**
 * Base character class for all hunters. Top-down isometric with:
 * - Spring arm camera at ~60° pitch, 2000 arm length
 * - CMC configured for top-down (face cursor, not movement direction)
 * - Replicated aim location for other clients to see facing direction
 * - ASC binding from owning PlayerState (Phase 2)
 */
UCLASS()
class BREACHBORNE_API AHunterCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AHunterCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ IAbilitySystemInterface — delegates to PlayerState's ASC
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** Server RPC: client sends cursor world position at ~20Hz */
	UFUNCTION(Server, Unreliable)
	void ServerSetAimLocation(FVector NewAimLocation);

	/** Get the current replicated aim location */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Aim")
	FVector GetAimLocation() const { return ReplicatedAimLocation; }

	/** Get the GliderComponent */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Movement")
	UGliderComponent* GetGliderComponent() const { return GliderComponent; }

	/** Get the MantleComponent */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Movement")
	UBBMantleComponent* GetMantleComponent() const { return MantleComponent; }

	/** Get the custom CharacterMovementComponent */
	UBBCharacterMovementComponent* GetBBMovementComponent() const;

	/** Local presentation helper for long-range targeted powers. */
	void SetAerialStrikeViewActive(bool bActive, float TargetArmLength);

	/** Apply optional mesh/animation data from the selected hunter definition. */
	void ApplyHunterDefinitionVisuals(const UBBHunterDefinition* HunterDefinition);

	/** Current data-driven visual bundle, replicated from the selected hunter definition. */
	UFUNCTION(BlueprintPure, Category = "Breachborne|Visual")
	const UBBAbilityVisualSet* GetAbilityVisualSet() const { return ReplicatedHunterVisualSet; }

	/** Play an ability animation phase from the replicated visual set. */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Visual")
	float PlayAbilityMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase = EBBAbilityAnimationPhase::Start, float PlayRateOverride = 0.0f);

	/** Stop an ability animation phase from the replicated visual set. */
	UFUNCTION(BlueprintCallable, Category = "Breachborne|Visual")
	void StopAbilityMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase = EBBAbilityAnimationPhase::Start, float BlendOutTime = 0.15f);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayAbilityMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float PlayRateOverride);

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_StopAbilityMontage(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float BlendOutTime);

	/** Get the glider indicator mesh (for toggling visibility from GliderComponent) */
	UStaticMeshComponent* GetGliderIndicatorMesh() const { return GliderIndicatorMesh; }

	/** Batch high-frequency cosmetic cues into one network RPC, then execute them locally. */
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_InvokeLocalGameplayCueBatch(const TArray<FBBLocalGameplayCue>& Cues, bool bSkipFirstCueForRemoteOwner);

	/** Plays the code primitive paired with a locally predicted cue without another network RPC. */
	void PlayLocalPrimitiveCueFallback(const FBBLocalGameplayCue& Cue);

	void BeginHookPull(AActor* Target, float PullSpeed, float ImpactDistance, float MaxPullDuration, float PullTickInterval, UClass* DamageEffectClass, float Damage, const FGameplayEffectContextHandle& Context);

	void BeginGrapplePull(AActor* TargetActor, const FVector& TargetLocation, float InitialSpeed, float MaxSpeed, float StopDistance, float MaxPullDuration, float PullTickInterval);

protected:
	virtual void BeginPlay() override;

	/** Called on server when possessed by a controller */
	virtual void PossessedBy(AController* NewController) override;

	/** Called on client when PlayerState replicates */
	virtual void OnRep_PlayerState() override;

	// --- Components ---

	/** Glider state machine — manages gliding, heat, and spiking */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Movement")
	TObjectPtr<UGliderComponent> GliderComponent;

	/** Mantle component — fall-recovery wall-climb */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Movement")
	TObjectPtr<UBBMantleComponent> MantleComponent;

	/** Temporary visible mesh for testing — remove when real art is added */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Visual")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	/** Arrow/cone showing which direction the character is facing — debug visual */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Visual")
	TObjectPtr<UStaticMeshComponent> FacingIndicator;

	/** Flat plane hovering above the character — visible only while gliding */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Visual")
	TObjectPtr<UStaticMeshComponent> GliderIndicatorMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|UI")
	TObjectPtr<UWidgetComponent> GliderFuelWidgetComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	float DefaultCameraBoomArmLength = 2000.0f;
	float TargetCameraBoomArmLength = 2000.0f;
	float CameraBoomZoomInterpSpeed = 6.0f;

	// --- Replicated State ---

	/** World-space cursor position, updated from owning client at ~20Hz */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Breachborne|Aim")
	FVector_NetQuantize ReplicatedAimLocation;

	UPROPERTY(ReplicatedUsing = OnRep_HunterVisuals, BlueprintReadOnly, Category = "Breachborne|Visual")
	TObjectPtr<USkeletalMesh> ReplicatedHunterMesh;

	UPROPERTY(ReplicatedUsing = OnRep_HunterVisuals, BlueprintReadOnly, Category = "Breachborne|Visual")
	TSubclassOf<UAnimInstance> ReplicatedHunterAnimInstanceClass;

	UPROPERTY(ReplicatedUsing = OnRep_HunterVisuals, BlueprintReadOnly, Category = "Breachborne|Visual")
	FTransform ReplicatedHunterMeshRelativeTransform = FTransform(FRotator(0.0f, -90.0f, 0.0f), FVector(0.0f, 0.0f, -90.0f), FVector(1.0f));

	UPROPERTY(ReplicatedUsing = OnRep_HunterVisuals, BlueprintReadOnly, Category = "Breachborne|Visual")
	TObjectPtr<UBBAbilityVisualSet> ReplicatedHunterVisualSet;

private:
	/** Bind to the ASC on the owning PlayerState. Called from PossessedBy (server) and OnRep_PlayerState (client). */
	void BindASCFromPlayerState();

	UFUNCTION()
	void OnRep_HunterVisuals();

	void ApplyReplicatedHunterVisuals();
	float PlayAbilityMontageLocal(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float PlayRateOverride);
	void StopAbilityMontageLocal(FGameplayTag AbilityOrInputTag, EBBAbilityAnimationPhase Phase, float BlendOutTime);

	/** Timer handle for retrying ASC binding on clients when PlayerState hasn't replicated yet */
	FTimerHandle ASCBindRetryHandle;

	void HookPullTick();
	void EndHookPull(bool bApplyImpactDamage);
	void ApplyHookPullImpactDamage();

	FTimerHandle HookPullTimerHandle;

	UPROPERTY()
	TWeakObjectPtr<AActor> HookPullTargetActor;

	UPROPERTY()
	TObjectPtr<UClass> HookPullDamageEffectClass = nullptr;

	FGameplayEffectContextHandle HookPullEffectContext;
	float HookPullElapsedSeconds = 0.0f;
	float HookPullSpeed = 1800.0f;
	float HookPullImpactDistance = 150.0f;
	float HookPullMaxDuration = 1.0f;
	float HookPullTickInterval = 0.02f;
	float HookPullDamage = 0.0f;

	void GrapplePullTick();
	void EndGrapplePull();

	FTimerHandle GrapplePullTimerHandle;

	UPROPERTY()
	TWeakObjectPtr<AActor> GrappleTargetActor;

	FVector GrappleTargetLocation = FVector::ZeroVector;
	float GrappleInitialDistance = 0.0f;
	float GrappleElapsedSeconds = 0.0f;
	float GrappleInitialSpeed = 1800.0f;
	float GrappleMaxSpeed = 4200.0f;
	float GrappleStopDistance = 140.0f;
	float GrappleMaxDuration = 1.0f;
	float GrappleTickInterval = 0.02f;
};
