#include "GA_Void_Q.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBVoidAnomalyPuddle.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

UGA_Void_Q::UGA_Void_Q()
{
	AbilityInputTag = BBGameplayTags::InputTag_Q;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::TargetedArea, Range, EmpoweredRadius);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Void_Q);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Void_Q);
	PuddleClass = ABBVoidAnomalyPuddle::StaticClass();
	DamageEffectClass = UBBDamageEffect::StaticClass();
}

void UGA_Void_Q::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SpawnPuddle();
}

void UGA_Void_Q::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	const AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !Hunter->HasAuthority())
	{
		return;
	}

	if (ABBVoidAnomalyPuddle* Puddle = ActivePuddle.Get())
	{
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Void_Q_Detonate, Puddle->GetActorLocation(), FVector::UpVector);
		Puddle->Burst();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_Void_Q::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().ClearTimer(PuddleExpiryHandle);
	}

	ActivePuddle.Reset();
	RemoveVisualCue(BBGameplayTags::GameplayCue_Hunter_Void_Q_Active);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Void_Q::SpawnPuddle()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const ABreachbornePlayerState* PS = GetBBPlayerState();
	if (!Hunter || !ASC || !PS)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	const bool bEmpowered = ConsumeEmpowered();
	const FVector AimDir = GetAimDirection();
	const FVector HunterLocation = Hunter->GetActorLocation();
	const FVector AimLocation = GetAimLocation();
	const float AimDistance = FMath::Clamp(FVector::Dist2D(HunterLocation, AimLocation), 200.0f, Range);
	FVector TargetLocation = HunterLocation + AimDir * AimDistance;
	TargetLocation.Z = HunterLocation.Z - 80.0f;
	const float FinalRadius = bEmpowered ? EmpoweredRadius : Radius;

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Void_Q, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Void_Q_Cast, TargetLocation, AimDir);

	if (Hunter->HasAuthority() && PuddleClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABBVoidAnomalyPuddle* Puddle = Hunter->GetWorld()->SpawnActor<ABBVoidAnomalyPuddle>(
			PuddleClass, TargetLocation, FRotator::ZeroRotator, Params);
		if (Puddle)
		{
			Puddle->InitPuddle(ASC, Hunter, DamageEffectClass, PS->GetTeamID(), FinalRadius, Duration, TickDamage, BurstDamage);
			ActivePuddle = Puddle;
			AddVisualCue(BBGameplayTags::GameplayCue_Hunter_Void_Q_Active, TargetLocation);
		}
	}

	Hunter->GetWorldTimerManager().SetTimer(PuddleExpiryHandle, this, &UGA_Void_Q::OnPuddleExpired, Duration + 0.1f, false);
}

void UGA_Void_Q::OnPuddleExpired()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

bool UGA_Void_Q::ConsumeEmpowered() const
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

const FGameplayTagContainer* UGA_Void_Q::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Void_Q::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
