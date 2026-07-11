#include "GA_Power_EmergencyPlatform.h"

#include "BBPowerDeployableActor.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Breachborne.h"

UGA_Power_EmergencyPlatform::UGA_Power_EmergencyPlatform()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
}

void UGA_Power_EmergencyPlatform::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
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
	const FVector SpawnLocation = Hunter->GetActorLocation() + Direction * SpawnDistance + FVector(0.0f, 0.0f, -45.0f);
	FActorSpawnParameters Params;
	Params.Owner = Hunter;
	Params.Instigator = Hunter;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (Hunter->GetWorld()->SpawnActor<ABBPowerDeployableActor>(ABBPowerDeployableActor::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params))
	{
		ApplyPowerCooldown();
		UE_LOG(LogBreachborne, Log, TEXT("PowerAbility: %s spawned EmergencyPlatform"), *GetPowerID().ToString());
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
