#include "GA_Eluna_RMB.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBStunTagEffect.h"
#include "Breachborne/Combat/BBElunaStickyProjectile.h"
#include "Breachborne/Combat/BBElunaRootZone.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Breachborne.h"

UGA_Eluna_RMB::UGA_Eluna_RMB()
{
	AbilityInputTag = BBGameplayTags::InputTag_RMB;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::TargetedArea, 2400.0f, RootRadius);
	bActivateOnInputHeld = false;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Eluna_RMB);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Eluna_RMB);

	DamageEffectClass = UBBDamageEffect::StaticClass();
	RootEffectClass = UBBStunTagEffect::StaticClass();
	ProjectileClass = ABBElunaStickyProjectile::StaticClass();
}

void UGA_Eluna_RMB::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	const FVector AimDir = GetAimDirection();
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Eluna_RMB, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Eluna_RMB_Fire, Hunter->GetActorLocation(), AimDir);

	if (Hunter->HasAuthority())
	{
		UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
		const ABreachbornePlayerState* ShooterPS = GetBBPlayerState();


		if (SourceASC && ShooterPS && ProjectileClass)
		{
			const FVector SpawnLocation = Hunter->GetActorLocation() + AimDir * SpawnForwardOffset;
			const FRotator SpawnRotation = AimDir.Rotation();

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Hunter;
			SpawnParams.Instigator = Hunter;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			ABBElunaStickyProjectile* Projectile = Hunter->GetWorld()->SpawnActor<ABBElunaStickyProjectile>(
				ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

			if (Projectile)
			{
				Projectile->InitProjectile(SourceASC, DamageEffectClass, BaseDamage, ShooterPS->GetTeamID());
				Projectile->FireInDirection(AimDir);
			}
			else
			{
			}
		}
	}
	else
	{
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Eluna_RMB::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Eluna_RMB::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
