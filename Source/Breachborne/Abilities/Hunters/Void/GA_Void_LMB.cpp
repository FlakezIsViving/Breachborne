#include "GA_Void_LMB.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBVoidGravitySlowEffect.h"
#include "Breachborne/Combat/BBVoidOrbProjectile.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

UGA_Void_LMB::UGA_Void_LMB()
{
	AbilityInputTag = BBGameplayTags::InputTag_LMB;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, 1800.0f);
	bActivateOnInputHeld = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Void_LMB);
	SetAssetTags(AssetTags);

	ProjectileClass = ABBVoidOrbProjectile::StaticClass();
	DamageEffectClass = UBBDamageEffect::StaticClass();
	SlowEffectClass = UBBVoidGravitySlowEffect::StaticClass();
}

void UGA_Void_LMB::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Void_LMB, EBBAbilityAnimationPhase::Loop);
	FireShot();

	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().SetTimer(FireTimerHandle, this, &UGA_Void_LMB::OnFireTimerTick, FireInterval, true);
	}
}

void UGA_Void_LMB::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}
	StopVisualMontage(BBGameplayTags::Ability_Hunter_Void_LMB, EBBAbilityAnimationPhase::Loop);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Void_LMB::FireShot()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const ABreachbornePlayerState* PS = GetBBPlayerState();
	UWorld* World = Hunter ? Hunter->GetWorld() : nullptr;
	if (!Hunter || !ASC || !PS || !World)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	const bool bCharged = Now - LastFireWorldTime >= ChargedIdleSeconds;
	LastFireWorldTime = Now;

	const FVector AimDir = GetAimDirection();
	ExecuteVisualCue(bCharged ? BBGameplayTags::GameplayCue_Hunter_Void_LMB_Charged : BBGameplayTags::GameplayCue_Hunter_Void_LMB_Fire,
		Hunter->GetActorLocation(), AimDir);

	if (!Hunter->HasAuthority() || !ProjectileClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = Hunter;
	Params.Instigator = Hunter;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLocation = Hunter->GetActorLocation() + AimDir * SpawnForwardOffset;
	ABBVoidOrbProjectile* Projectile = World->SpawnActor<ABBVoidOrbProjectile>(ProjectileClass, SpawnLocation, AimDir.Rotation(), Params);
	if (Projectile)
	{
		Projectile->InitVoidOrb(ASC, DamageEffectClass, SlowEffectClass, bCharged ? ChargedDamage : BaseDamage, PS->GetTeamID(), bCharged);
		Projectile->FireInDirection(AimDir);
	}
}

void UGA_Void_LMB::OnFireTimerTick()
{
	if (!IsActive())
	{
		if (const AHunterCharacter* Hunter = GetHunterCharacter())
		{
			Hunter->GetWorldTimerManager().ClearTimer(FireTimerHandle);
		}
		return;
	}

	FireShot();
}
