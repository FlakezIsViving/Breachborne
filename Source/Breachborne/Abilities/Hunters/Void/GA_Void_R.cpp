#include "GA_Void_R.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBStunTagEffect.h"
#include "Breachborne/Combat/BBVoidSingularityActor.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

UGA_Void_R::UGA_Void_R()
{
	AbilityInputTag = BBGameplayTags::InputTag_R;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::TargetedArea, Range, Radius);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Void_R);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Void_R);
	SingularityClass = ABBVoidSingularityActor::StaticClass();
	StunEffectClass = UBBStunTagEffect::StaticClass();
}

void UGA_Void_R::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const ABreachbornePlayerState* PS = GetBBPlayerState();
	if (!Hunter || !ASC || !PS)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const bool bEmpowered = ConsumeEmpowered();
	const FVector AimDir = GetAimDirection();
	const FVector HunterLocation = Hunter->GetActorLocation();
	const FVector AimLocation = GetAimLocation();
	const float AimDistance = FMath::Clamp(FVector::Dist2D(HunterLocation, AimLocation), 250.0f, Range);
	FVector TargetLocation = HunterLocation + AimDir * AimDistance;
	TargetLocation.Z = HunterLocation.Z - 65.0f;

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Void_R, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Void_R_Warning, TargetLocation, AimDir);

	if (Hunter->HasAuthority() && SingularityClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABBVoidSingularityActor* Singularity = Hunter->GetWorld()->SpawnActor<ABBVoidSingularityActor>(
			SingularityClass, TargetLocation, FRotator::ZeroRotator, Params);
		if (Singularity)
		{
			Singularity->InitSingularity(ASC, Hunter, StunEffectClass, PS->GetTeamID(), Radius, WarningDelay, ActiveDuration, bEmpowered);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UGA_Void_R::ConsumeEmpowered() const
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

const FGameplayTagContainer* UGA_Void_R::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Void_R::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
