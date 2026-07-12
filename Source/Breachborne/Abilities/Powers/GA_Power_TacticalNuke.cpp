#include "GA_Power_TacticalNuke.h"

#include "BBPowerNukeStrikeActor.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Core/BreachbornePlayerController.h"

UGA_Power_TacticalNuke::UGA_Power_TacticalNuke()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::TargetedArea, 5200.0f, Radius);
}

void UGA_Power_TacticalNuke::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (IsPowerOnCooldown())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("PowerAbility: %s rejected - cooldown active"), *GetPowerID().ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !Hunter->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ABreachbornePlayerController* PC = Cast<ABreachbornePlayerController>(Hunter->GetController());
	FVector TargetLocation = FVector::ZeroVector;
	if (!PC || !PC->ConsumePendingTargetedPowerActivation(GetPowerInputTag(), TargetLocation))
	{
		UE_LOG(LogBreachborne, Warning, TEXT("PowerAbility: %s rejected - missing confirmed target"), *GetPowerID().ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = Hunter;
	Params.Instigator = Hunter;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (ABBPowerNukeStrikeActor* Strike = Hunter->GetWorld()->SpawnActor<ABBPowerNukeStrikeActor>(
		ABBPowerNukeStrikeActor::StaticClass(), TargetLocation, FRotator::ZeroRotator, Params))
	{
		Strike->InitNukeStrike(GetAbilitySystemComponentFromActorInfo(), Hunter, UBBDamageEffect::StaticClass(),
			Radius, ImpactDelay, ImpactDamage, BurnDuration, BurnDamagePerTick, BurnTickInterval);
		ApplyPowerCooldown();
		UE_LOG(LogBreachborne, Log, TEXT("PowerAbility: %s launched TacticalNuke at %s"), *GetPowerID().ToString(), *TargetLocation.ToCompactString());
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
