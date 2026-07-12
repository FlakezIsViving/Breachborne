#include "GA_Kingpin_GroundDash.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Breachborne/Breachborne.h"

UGA_Kingpin_GroundDash::UGA_Kingpin_GroundDash()
{
	AbilityInputTag = BBGameplayTags::InputTag_Shift;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Movement, 650.0f, 55.0f);
	bActivateOnInputHeld = false;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Kingpin_GroundDash);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Kingpin_GroundDash);
	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Dash); // tagged for takedown-reset query
}

void UGA_Kingpin_GroundDash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	// Direction: WASD input via CMC->Acceleration, cursor fallback when no input.
	// Same shape as Ghost Shift — see that file for the rationale.
	FVector DashDir = Hunter->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();
	if (DashDir.IsNearlyZero())
	{
		DashDir = Hunter->GetActorForwardVector().GetSafeNormal2D();
	}

	FVector LaunchVelocity = DashDir * DashSpeed;
	LaunchVelocity.Z = DashLift;
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_GroundDash, EBBAbilityAnimationPhase::Start);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_Shift_Start, Hunter->GetActorLocation(), DashDir);
	if (Hunter->HasAuthority())
	{
		const FVector Start = Hunter->GetActorLocation();
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBeamActor* Trail = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
			ABBPrimitiveBeamActor::StaticClass(), Start, FRotator::ZeroRotator, Params))
		{
			Trail->InitBeam(Start, Start + DashDir * 650.0f, 11.0f, 0.25f,
				FLinearColor(0.90f, 0.23f, 0.18f, 1.0f));
		}
	}
	Hunter->LaunchCharacter(LaunchVelocity, /*bXYOverride=*/true, /*bZOverride=*/true);

	UE_LOG(LogBreachborne, Log, TEXT("Kingpin Ground Dash on %s in dir (%.2f, %.2f)"),
		*Hunter->GetName(), DashDir.X, DashDir.Y);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Kingpin_GroundDash::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Kingpin_GroundDash::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
