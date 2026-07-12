#include "GA_Eluna_Q.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBMoonlightZone.h"
#include "Breachborne/Breachborne.h"
#include "Engine/World.h"

UGA_Eluna_Q::UGA_Eluna_Q()
{
	AbilityInputTag = BBGameplayTags::InputTag_Q;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::TargetedArea, ThrowRange, HealRadius);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Eluna_Q);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Eluna_Q);

	ZoneClass = ABBMoonlightZone::StaticClass();
}

void UGA_Eluna_Q::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Eluna_Q, EBBAbilityAnimationPhase::Start);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Eluna_Q_Cast, Hunter->GetActorLocation(), FVector::UpVector);

	// Spawn zone at feet on first activation
	SpawnZone();
}

void UGA_Eluna_Q::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{

	ABBMoonlightZone* Zone = ActiveZone.Get();
	if (Zone && Zone->IsValidLowLevel() && !Zone->IsPendingKillPending())
	{
		if (!Zone->WasTossed())
		{
			TossZone();
		}
	}
	else
	{
		// Zone expired — end the ability so cooldown can run
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Eluna_Q::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().ClearTimer(ZoneExpiryTimerHandle);
	}

	ActiveZone = nullptr;
	RemoveVisualCue(BBGameplayTags::GameplayCue_Hunter_Eluna_Q_Active);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Eluna_Q::SpawnZone()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !ZoneClass)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}
	if (!Hunter->HasAuthority())
	{
		return;
	}

	const FVector SpawnLoc = Hunter->GetActorLocation();
	FActorSpawnParameters Params;
	Params.Owner = Hunter;
	Params.Instigator = Hunter;

	ABBMoonlightZone* Zone = Hunter->GetWorld()->SpawnActor<ABBMoonlightZone>(
		ZoneClass, SpawnLoc, FRotator::ZeroRotator, Params);

	if (Zone)
	{
		Zone->InitZone(Hunter, HealPerTick, HealTickInterval, HealRadius, ZoneDuration, SelfHealFraction, WispHealMultiplier, BurstHeal);
		ActiveZone = Zone;
		AddVisualCue(BBGameplayTags::GameplayCue_Hunter_Eluna_Q_Active, SpawnLoc, FVector::UpVector);

		// Set timer to end ability when zone expires
		Hunter->GetWorldTimerManager().SetTimer(
			ZoneExpiryTimerHandle,
			FTimerDelegate::CreateUObject(this, &UGA_Eluna_Q::OnZoneExpired),
			ZoneDuration,
			false
		);
	}
	else
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UGA_Eluna_Q::TossZone()
{
	ABBMoonlightZone* Zone = ActiveZone.Get();
	if (!Zone)
	{
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		return;
	}

	const FVector HunterLoc = Hunter->GetActorLocation();
	const FVector AimDir = GetAimDirection();
	FVector TargetLocation = HunterLoc + AimDir * ThrowRange;
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Eluna_Q, EBBAbilityAnimationPhase::Toss);

	// Trace to find valid ground position
	FHitResult HitResult;
	FVector TraceStart = TargetLocation + FVector(0, 0, 1000.0f);
	FVector TraceEnd = TargetLocation - FVector(0, 0, 5000.0f);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Hunter);
	QueryParams.AddIgnoredActor(Zone);

	const bool bHit = Hunter->GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	if (bHit)
	{
		TargetLocation = HitResult.ImpactPoint + FVector(0, 0, 10.0f);
	}

	UE_LOG(LogBreachborne, Warning, TEXT("[Q_TOSS] AimDir=%s | HunterLoc=%s | RawTarget=%s | TraceHit=%s | FinalTarget=%s | ZoneCurrent=%s"),
		*AimDir.ToCompactString(), *HunterLoc.ToCompactString(),
		*(HunterLoc + AimDir * ThrowRange).ToCompactString(),
		bHit ? TEXT("YES") : TEXT("NO"),
		*TargetLocation.ToCompactString(),
		*Zone->GetActorLocation().ToCompactString());

	Zone->TossToLocation(TargetLocation);
}

void UGA_Eluna_Q::OnZoneExpired()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Eluna_Q::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Eluna_Q::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
