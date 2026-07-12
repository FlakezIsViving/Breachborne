#include "GA_Power_GrapplingHook.h"

#include "BBGrapplingHookProjectile.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Characters/HunterCharacter.h"

UGA_Power_GrapplingHook::UGA_Power_GrapplingHook()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, Range);
}

void UGA_Power_GrapplingHook::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
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

	const FVector Direction = GetAimDirection();
	if (Direction.IsNearlyZero())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	const FVector SpawnLocation = Hunter->GetActorLocation() + Direction * 95.0f + FVector(0.0f, 0.0f, 65.0f);
	FActorSpawnParameters Params;
	Params.Owner = Hunter;
	Params.Instigator = Hunter;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABBGrapplingHookProjectile* Hook = Hunter->GetWorld()->SpawnActor<ABBGrapplingHookProjectile>(
		ABBGrapplingHookProjectile::StaticClass(),
		SpawnLocation,
		Direction.Rotation(),
		Params);

	if (Hook)
	{
		Hook->InitGrapple(Hunter, Direction, Range, InitialPullSpeed, MaxPullSpeed, StopDistance, MaxPullDuration, PullTickInterval);
		ApplyPowerCooldown();
		UE_LOG(LogBreachborne, Log, TEXT("PowerAbility: %s fired GrapplingHook"), *GetPowerID().ToString());
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
