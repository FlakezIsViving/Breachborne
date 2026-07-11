#include "GA_Ghost_RMB.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Combat/BBGrenade.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Breachborne/Breachborne.h"

UGA_Ghost_RMB::UGA_Ghost_RMB()
{
	AbilityInputTag = BBGameplayTags::InputTag_RMB;
	bActivateOnInputHeld = false;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Ghost_RMB);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Ghost_RMB);

	DamageEffectClass = UBBDamageEffect::StaticClass();
	GrenadeClass = ABBGrenade::StaticClass();
}

void UGA_Ghost_RMB::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Ghost_RMB, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Ghost_RMB_Cast, Hunter->GetActorLocation(), AimDir);

	// Server: spawn the grenade projectile
	if (Hunter->HasAuthority())
	{
		UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
		const ABreachbornePlayerState* ShooterPS = GetBBPlayerState();

		if (SourceASC && ShooterPS && GrenadeClass)
		{
			const FVector SpawnLocation = Hunter->GetActorLocation() + AimDir * SpawnForwardOffset;
			const FRotator SpawnRotation = AimDir.Rotation();

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Hunter;
			SpawnParams.Instigator = Hunter;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			ABBGrenade* Grenade = Hunter->GetWorld()->SpawnActor<ABBGrenade>(
				GrenadeClass,
				SpawnLocation,
				SpawnRotation,
				SpawnParams
			);

			if (Grenade)
			{
				Grenade->InitProjectile(SourceASC, DamageEffectClass, GrenadeDamage, ShooterPS->GetTeamID());
				Grenade->FireInDirection(AimDir);

				UE_LOG(LogBreachborne, Log, TEXT("Ghost RMB: Spawned grenade at %s dir %s"),
					*SpawnLocation.ToString(), *AimDir.ToString());
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Ghost_RMB::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Ghost_RMB::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = NewObject<UGameplayEffect>(GetTransientPackage());
	CooldownGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	CooldownGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(CooldownDuration));
	UTargetTagsGameplayEffectComponent& TagComp = CooldownGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::Cooldown_Hunter_Ghost_RMB);
	TagComp.SetAndApplyTargetTagChanges(TagContainer);

	ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CooldownGE, GetAbilityLevel());
}
