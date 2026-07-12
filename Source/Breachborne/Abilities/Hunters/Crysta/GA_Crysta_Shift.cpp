#include "GA_Crysta_Shift.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Crysta_Shift_Primary::UGA_Crysta_Shift_Primary()
{
	AbilityInputTag = BBGameplayTags::InputTag_Shift;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Movement, 520.0f, 55.0f);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Crysta_Shift_Primary);
	SetAssetTags(AssetTags);
	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Primary);
	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Dash);
	DashTrailClass = ABBPrimitiveBeamActor::StaticClass();
}

void UGA_Crysta_Shift_Primary::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector DashDir = FVector::ZeroVector;
	if (const UCharacterMovementComponent* MoveComp = Hunter->GetCharacterMovement())
	{
		DashDir = MoveComp->GetCurrentAcceleration().GetSafeNormal2D();
	}
	if (DashDir.IsNearlyZero())
	{
		DashDir = GetAimDirection();
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(BBGameplayTags::State_Crysta_EmpoweredLMB);
	}

	const FGameplayTag VisualTag = IsA(UGA_Crysta_Shift_Secondary::StaticClass())
		? BBGameplayTags::Ability_Hunter_Crysta_Shift_Secondary
		: BBGameplayTags::Ability_Hunter_Crysta_Shift_Primary;
	PlayVisualMontage(VisualTag, EBBAbilityAnimationPhase::Start);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Crysta_Shift_Dash, Hunter->GetActorLocation(), DashDir);
	if (Hunter->HasAuthority() && DashTrailClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBeamActor* Trail = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(DashTrailClass, FTransform::Identity, Params))
		{
			const FVector Start = Hunter->GetActorLocation() + FVector(0.0f, 0.0f, 55.0f);
			const FVector End = Start + DashDir * 520.0f;
			Trail->InitBeam(Start, End, 18.0f, 0.32f, FLinearColor(1.0f, 0.95f, 0.15f, 1.0f));
		}
	}
	Hunter->LaunchCharacter(DashDir * DashSpeed + FVector(0.0f, 0.0f, DashLift), true, true);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Crysta_Shift_Primary::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Crysta_Shift_Primary::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}

UGA_Crysta_Shift_Secondary::UGA_Crysta_Shift_Secondary()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Crysta_Shift_Secondary);
	SetAssetTags(AssetTags);
	CooldownTagContainer.Reset();
	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Secondary);
	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Dash);
}
