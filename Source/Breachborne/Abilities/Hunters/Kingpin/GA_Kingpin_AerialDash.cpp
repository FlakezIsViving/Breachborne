#include "GA_Kingpin_AerialDash.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Breachborne/Breachborne.h"

UGA_Kingpin_AerialDash::UGA_Kingpin_AerialDash()
{
	AbilityInputTag = BBGameplayTags::InputTag_Shift;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Movement, 650.0f, 55.0f);
	bActivateOnInputHeld = false;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Kingpin_AerialDash);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Kingpin_AerialDash);
	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Dash); // tagged for takedown-reset query
}

bool UGA_Kingpin_AerialDash::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// Base checks first (cooldown, cost, ActivationBlockedTags) — gate on the cheap stuff
	// before the airborne query so we don't pay for IsMovingOnGround when something
	// already disqualifies us.
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return false;
	}

	const UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	if (!CMC)
	{
		return false;
	}

	// Airborne-only — IsMovingOnGround covers MOVE_Walking and MOVE_NavWalking; anything
	// else (MOVE_Falling, MOVE_Flying, our custom Gliding/Mantling) counts as airborne.
	return !CMC->IsMovingOnGround();
}

void UGA_Kingpin_AerialDash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
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

	FVector DashDir = Hunter->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();
	if (DashDir.IsNearlyZero())
	{
		DashDir = Hunter->GetActorForwardVector().GetSafeNormal2D();
	}

	FVector LaunchVelocity = DashDir * DashSpeed;
	LaunchVelocity.Z = DashLift;
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_AerialDash, EBBAbilityAnimationPhase::Start);
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
				FLinearColor(1.0f, 0.69f, 0.13f, 1.0f));
		}
	}
	Hunter->LaunchCharacter(LaunchVelocity, /*bXYOverride=*/true, /*bZOverride=*/true);

	UE_LOG(LogBreachborne, Log, TEXT("Kingpin Aerial Dash on %s in dir (%.2f, %.2f)"),
		*Hunter->GetName(), DashDir.X, DashDir.Y);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Kingpin_AerialDash::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Kingpin_AerialDash::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
