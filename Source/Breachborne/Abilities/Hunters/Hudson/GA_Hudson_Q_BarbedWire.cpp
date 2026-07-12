#include "GA_Hudson_Q_BarbedWire.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBHudsonBarbedWireActor.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

UGA_Hudson_Q_BarbedWire::UGA_Hudson_Q_BarbedWire()
{
	AbilityInputTag = BBGameplayTags::InputTag_Q;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::TargetedArea, Range, 360.0f);
	bActivateOnInputHeld = false;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Hudson_Q);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Hudson_Q);
	WireClass = ABBHudsonBarbedWireActor::StaticClass();
}

void UGA_Hudson_Q_BarbedWire::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	const ABreachbornePlayerState* PS = GetBBPlayerState();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!Hunter || !PS || !SourceASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector AimDir = GetAimDirection();
	const FVector Desired = Hunter->GetActorLocation() + AimDir * Range;
	FVector SpawnLocation = Desired;
	SpawnLocation.Z = Hunter->GetActorLocation().Z - 80.0f;

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_Q, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_Q_Cast, SpawnLocation, AimDir);

	if (Hunter->HasAuthority() && WireClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABBHudsonBarbedWireActor* Wire = Hunter->GetWorld()->SpawnActor<ABBHudsonBarbedWireActor>(
			WireClass,
			SpawnLocation,
			AimDir.Rotation(),
			Params);

		if (Wire)
		{
			Wire->InitWire(SourceASC, PS->GetTeamID(), Duration, DamagePerSecond);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Hudson_Q_BarbedWire::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Hudson_Q_BarbedWire::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
