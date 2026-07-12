#include "GA_Void_RMB.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBStunTagEffect.h"
#include "Breachborne/Combat/BBVoidSnapProjectile.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

UGA_Void_RMB::UGA_Void_RMB()
{
	AbilityInputTag = BBGameplayTags::InputTag_RMB;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, 1725.0f);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Void_RMB);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Void_RMB);
	ProjectileClass = ABBVoidSnapProjectile::StaticClass();
	DamageEffectClass = UBBDamageEffect::StaticClass();
	StunEffectClass = UBBStunTagEffect::StaticClass();
}

void UGA_Void_RMB::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const ABreachbornePlayerState* PS = GetBBPlayerState();
	if (!Hunter || !ASC || !PS)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const bool bEmpowered = ConsumeEmpowered();
	const FVector AimDir = GetAimDirection();
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Void_RMB, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Void_RMB_Fire, Hunter->GetActorLocation(), AimDir);

	if (Hunter->HasAuthority() && ProjectileClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABBVoidSnapProjectile* Projectile = Hunter->GetWorld()->SpawnActor<ABBVoidSnapProjectile>(
			ProjectileClass, Hunter->GetActorLocation() + AimDir * SpawnForwardOffset, AimDir.Rotation(), Params);
		if (Projectile)
		{
			Projectile->InitSnap(Hunter, ASC, DamageEffectClass, StunEffectClass, PS->GetTeamID(), AimDir, bEmpowered);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UGA_Void_RMB::ConsumeEmpowered() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !ASC->HasMatchingGameplayTag(BBGameplayTags::State_Void_Empowered))
	{
		return false;
	}

	ASC->SetLooseGameplayTagCount(BBGameplayTags::State_Void_Empowered, 0);
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Void_Passive_Empowered, Hunter->GetActorLocation());
	}
	return true;
}

const FGameplayTagContainer* UGA_Void_RMB::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Void_RMB::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
