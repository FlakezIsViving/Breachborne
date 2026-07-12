#include "GA_Crysta_Q.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBCrystaBurstZone.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBReverberationMarkEffect.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

UGA_Crysta_Q::UGA_Crysta_Q()
{
	AbilityInputTag = BBGameplayTags::InputTag_Q;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::TargetedArea, Range, Radius);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Crysta_Q);
	SetAssetTags(AssetTags);
	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Crysta_Q);
	ZoneClass = ABBCrystaBurstZone::StaticClass();
	DamageEffectClass = UBBDamageEffect::StaticClass();
	MarkEffectClass = UBBReverberationMarkEffect::StaticClass();
}

void UGA_Crysta_Q::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	SpawnZone();
}

void UGA_Crysta_Q::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (ABBCrystaBurstZone* Zone = ActiveZone.Get())
	{
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Crysta_Q_Detonate, Zone->GetActorLocation(), FVector::UpVector);
		Zone->Detonate();
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_Crysta_Q::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().ClearTimer(ZoneExpiryHandle);
	}
	ActiveZone.Reset();
	RemoveVisualCue(BBGameplayTags::GameplayCue_Hunter_Crysta_Q_Active);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Crysta_Q::SpawnZone()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	const ABreachbornePlayerState* PS = GetBBPlayerState();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Hunter || !PS || !ASC)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	const FVector AimDir = GetAimDirection();
	FVector TargetLocation = Hunter->GetActorLocation() + AimDir * FMath::Min(Range, FVector::Dist2D(Hunter->GetActorLocation(), GetAimLocation()));
	if (TargetLocation.Equals(Hunter->GetActorLocation(), 1.0f))
	{
		TargetLocation = Hunter->GetActorLocation() + AimDir * Range;
	}
	TargetLocation.Z = Hunter->GetActorLocation().Z - 80.0f;
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Crysta_Q, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Crysta_Q_Cast, TargetLocation, AimDir);

	if (Hunter->HasAuthority() && ZoneClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABBCrystaBurstZone* Zone = Hunter->GetWorld()->SpawnActor<ABBCrystaBurstZone>(ZoneClass, TargetLocation, FRotator::ZeroRotator, Params);
		if (Zone)
		{
			Zone->InitBurstZone(Hunter, ASC, DamageEffectClass, MarkEffectClass, PS->GetTeamID(), Radius, Duration, BurstDamage);
			ActiveZone = Zone;
			AddVisualCue(BBGameplayTags::GameplayCue_Hunter_Crysta_Q_Active, TargetLocation);
		}
	}

	Hunter->GetWorldTimerManager().SetTimer(ZoneExpiryHandle, this, &UGA_Crysta_Q::OnZoneExpired, Duration + 0.1f, false);
}

void UGA_Crysta_Q::OnZoneExpired()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Crysta_Q::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Crysta_Q::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
