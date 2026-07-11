#include "GA_Eluna_Shift.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"

// ------------------------------------------------------------------
// Ground Dash
// ------------------------------------------------------------------

UGA_Eluna_GroundDash::UGA_Eluna_GroundDash()
{
	AbilityInputTag = BBGameplayTags::InputTag_Shift;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Eluna_GroundDash);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Eluna_GroundDash);
	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Dash);
}

bool UGA_Eluna_GroundDash::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	return bResult;
}

void UGA_Eluna_GroundDash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UCharacterMovementComponent* CMC = Hunter->GetCharacterMovement();
	if (!CMC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector DashDir = CMC->GetCurrentAcceleration().GetSafeNormal2D();
	if (DashDir.IsNearlyZero())
	{
		DashDir = GetAimDirection();
	}


	FVector LaunchVelocity = DashDir * DashSpeed;
	LaunchVelocity.Z = DashLift;
	const FGameplayTag VisualAbilityTag = IsA(UGA_Eluna_AerialDash::StaticClass())
		? BBGameplayTags::Ability_Hunter_Eluna_AerialDash
		: BBGameplayTags::Ability_Hunter_Eluna_GroundDash;
	PlayVisualMontage(VisualAbilityTag, EBBAbilityAnimationPhase::Start);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Eluna_Shift_Trail, Hunter->GetActorLocation(), DashDir);
	Hunter->LaunchCharacter(LaunchVelocity, true, true);
	CheckAllyPassThrough();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_Eluna_GroundDash::CheckAllyPassThrough()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		return;
	}

	ABreachbornePlayerState* SourcePS = GetBBPlayerState();
	if (!SourcePS)
	{
		return;
	}

	const FVector SourceLoc = Hunter->GetActorLocation();
	const int32 SourceTeam = SourcePS->GetTeamID();

	// Sphere sweep for allies within detection radius
	TArray<AActor*> OverlappingActors;
	UKismetSystemLibrary::SphereOverlapActors(
		Hunter->GetWorld(),
		SourceLoc,
		AllyDetectionRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>{ UEngineTypes::ConvertToObjectType(ECC_Pawn) },
		AHunterCharacter::StaticClass(),
		TArray<AActor*>{ Hunter },
		OverlappingActors
	);

	bool bPassedThroughAlly = false;
	for (AActor* Actor : OverlappingActors)
	{
		AHunterCharacter* Ally = Cast<AHunterCharacter>(Actor);
		if (!Ally || Ally == Hunter)
		{
			continue;
		}

		ABreachbornePlayerState* AllyPS = Ally->GetPlayerState<ABreachbornePlayerState>();
		if (AllyPS && AllyPS->GetTeamID() == SourceTeam)
		{
			bPassedThroughAlly = true;
			break;
		}
	}

	if (!bPassedThroughAlly)
	{
		return;
	}

	// Refund 50% of this dash's cooldown
	UBBAbilitySystemComponent* ASC = GetBBAbilitySystemComponent();
	if (ASC)
	{
		// Remove active cooldown effect for this dash
		FGameplayTagContainer DashCooldownTags;
		DashCooldownTags.AddTag(BBGameplayTags::Cooldown_Hunter_Eluna_GroundDash);
		ASC->RemoveActiveEffectsWithTags(DashCooldownTags);

		// Re-apply cooldown at reduced duration
		const float RefundedDuration = CooldownDuration * (1.0f - AllyCooldownRefundFraction);
		if (RefundedDuration > 0.1f)
		{
			ApplyBBCooldown(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), RefundedDuration);
		}

		// Reduce Q cooldown by 1 second
		FGameplayTagContainer QCooldownTag;
		QCooldownTag.AddTag(BBGameplayTags::Cooldown_Hunter_Eluna_Q);
		// Reduce Q cooldown by removing the existing cooldown effect and re-applying with reduced duration
		TArray<FActiveGameplayEffectHandle> ActiveEffects = ASC->GetActiveEffectsWithAllTags(QCooldownTag);
		for (const FActiveGameplayEffectHandle& EffectHandle : ActiveEffects)
		{
			const FActiveGameplayEffect* ActiveEffect = ASC->GetActiveGameplayEffect(EffectHandle);
			if (ActiveEffect)
			{
				const float NewRemaining = FMath::Max(0.0f, ActiveEffect->GetTimeRemaining(ASC->GetWorld()->GetTimeSeconds()) - QCooldownReduction);
				ASC->RemoveActiveGameplayEffect(EffectHandle);
				if (NewRemaining > 0.01f)
				{
					// Re-apply cooldown with reduced duration
					ApplyBBCooldown(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), NewRemaining);
				}
			}
		}
	}


}

const FGameplayTagContainer* UGA_Eluna_GroundDash::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Eluna_GroundDash::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}

// ------------------------------------------------------------------
// Aerial Dash
// ------------------------------------------------------------------

UGA_Eluna_AerialDash::UGA_Eluna_AerialDash()
{
	AbilityInputTag = BBGameplayTags::InputTag_Shift;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Eluna_AerialDash);
	SetAssetTags(AssetTags);

	// Aerial dash goes farther
	DashDistance = 900.0f;
	CooldownDuration = 6.0f;
}

bool UGA_Eluna_AerialDash::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const bool bSuperOk = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	bool bAirborne = false;
	if (const AHunterCharacter* Hunter = Cast<AHunterCharacter>(ActorInfo->AvatarActor.Get()))
	{
		if (UCharacterMovementComponent* CMC = Hunter->GetCharacterMovement())
		{
			bAirborne = !CMC->IsMovingOnGround();
		}
	}
	return bSuperOk && bAirborne;
}

const FGameplayTagContainer* UGA_Eluna_AerialDash::GetCooldownTags() const
{
	static FGameplayTagContainer AerialCooldownTags;
	AerialCooldownTags.AddTag(BBGameplayTags::Cooldown_Hunter_Eluna_AerialDash);
	AerialCooldownTags.AddTag(BBGameplayTags::Cooldown_Dash);
	return &AerialCooldownTags;
}
