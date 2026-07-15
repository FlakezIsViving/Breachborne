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

	FVector AimDir = GetAimDirection();
	if (AimDir.IsNearlyZero())
	{
		AimDir = Hunter->GetActorForwardVector().GetSafeNormal2D();
	}
	const FVector Desired = Hunter->GetActorLocation() + AimDir * Range;
	FVector SpawnLocation = Desired;
	FHitResult GroundHit;
	FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(HudsonBarbedWireGround), false, Hunter);
	const FVector TraceStart = Desired + FVector(0.0f, 0.0f, 1200.0f);
	const FVector TraceEnd = Desired - FVector(0.0f, 0.0f, 3000.0f);
	if (Hunter->GetWorld()->LineTraceSingleByChannel(
		GroundHit, TraceStart, TraceEnd, ECC_Visibility, GroundQuery))
	{
		// The wire plate is 92 units below its actor origin.
		SpawnLocation = GroundHit.ImpactPoint + FVector(0.0f, 0.0f, 92.0f);
	}
	else
	{
		SpawnLocation.Z = Hunter->GetActorLocation().Z - 80.0f;
	}

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
