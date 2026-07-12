#include "GA_Crysta_RMB.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBCrystaFluxSnareProjectile.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBGroundedEffect.h"
#include "Breachborne/Combat/BBReverberationMarkEffect.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

UGA_Crysta_RMB::UGA_Crysta_RMB()
{
	AbilityInputTag = BBGameplayTags::InputTag_RMB;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, 1350.0f);
	bActivateOnInputHeld = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Crysta_RMB);
	SetAssetTags(AssetTags);
	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Crysta_RMB);

	ProjectileClass = ABBCrystaFluxSnareProjectile::StaticClass();
	DamageEffectClass = UBBDamageEffect::StaticClass();
	MarkEffectClass = UBBReverberationMarkEffect::StaticClass();
	GroundedEffectClass = UBBGroundedEffect::StaticClass();
}

void UGA_Crysta_RMB::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	const ABreachbornePlayerState* PS = GetBBPlayerState();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Hunter || !PS || !ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector AimDir = GetAimDirection();
	ActivationWorldTime = Hunter->GetWorld()->GetTimeSeconds();
	bReleaseHandled = false;
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Crysta_RMB, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Crysta_RMB_Fire, Hunter->GetActorLocation(), AimDir);

	if (Hunter->HasAuthority() && ProjectileClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABBCrystaFluxSnareProjectile* Projectile = Hunter->GetWorld()->SpawnActor<ABBCrystaFluxSnareProjectile>(
			ProjectileClass, Hunter->GetActorLocation() + AimDir * SpawnForwardOffset, AimDir.Rotation(), Params);
		if (Projectile)
		{
			Projectile->InitFluxSnare(Hunter, ASC, DamageEffectClass, MarkEffectClass, GroundedEffectClass, PS->GetTeamID(), AimDir);
			ActiveProjectile = Projectile;
		}
	}

	if (UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true))
	{
		ReleaseTask->OnRelease.AddDynamic(this, &UGA_Crysta_RMB::HandleInputRelease);
		ReleaseTask->ReadyForActivation();
	}
}

void UGA_Crysta_RMB::OnInputReleased()
{
	const float HeldSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() - ActivationWorldTime : 0.0f;
	HandleInputRelease(HeldSeconds);
}

void UGA_Crysta_RMB::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ActiveProjectile.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Crysta_RMB::HandleInputRelease(float TimeHeld)
{
	if (bReleaseHandled || !IsActive())
	{
		return;
	}

	bReleaseHandled = true;
	const float HeldSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() - ActivationWorldTime : TimeHeld;
	if (HeldSeconds >= HoldReturnThreshold)
	{
		if (ABBCrystaFluxSnareProjectile* Projectile = ActiveProjectile.Get())
		{
			Projectile->StartReturn();
		}
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Crysta_RMB::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Crysta_RMB::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
