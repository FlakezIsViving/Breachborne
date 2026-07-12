#include "GA_Void_Shift.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBVoidSwapProjectile.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"

UGA_Void_Shift::UGA_Void_Shift()
{
	AbilityInputTag = BBGameplayTags::InputTag_Shift;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::TargetedArea, Range, EmpoweredRadius);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Void_Shift);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Void_Shift);
	ProjectileClass = ABBVoidSwapProjectile::StaticClass();
}

void UGA_Void_Shift::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
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

	const bool bEmpowered = ConsumeEmpowered();
	const FVector AimDir = GetAimDirection();
	const FVector HunterLocation = Hunter->GetActorLocation();
	const FVector AimLocation = GetAimLocation();
	const float AimDistance = FMath::Clamp(FVector::Dist2D(HunterLocation, AimLocation), 250.0f, Range);
	FVector TargetLocation = HunterLocation + AimDir * AimDistance;
	TargetLocation.Z = HunterLocation.Z;
	const FVector LaunchLocation = HunterLocation + AimDir * SpawnForwardOffset + FVector(0.0f, 0.0f, 70.0f);

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Void_Shift, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Void_Shift_Cast, TargetLocation, AimDir);

	if (Hunter->HasAuthority() && ProjectileClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const float FinalRadius = bEmpowered ? EmpoweredRadius : Radius;
		const float FinalTravelDuration = bEmpowered ? EmpoweredTravelDuration : TravelDuration;
		FVector SourceZoneLocation = HunterLocation;
		SourceZoneLocation.Z -= 80.0f;
		if (ABBPrimitiveBurstActor* SourceZone = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
			ABBPrimitiveBurstActor::StaticClass(), SourceZoneLocation, FRotator::ZeroRotator, Params))
		{
			SourceZone->InitBurst(SourceZoneLocation, FinalRadius, FinalTravelDuration,
				bEmpowered ? FLinearColor(1.0f, 0.31f, 0.85f, 1.0f) : FLinearColor(0.35f, 0.94f, 0.56f, 1.0f), true);
		}

		ABBVoidSwapProjectile* Projectile = Hunter->GetWorld()->SpawnActor<ABBVoidSwapProjectile>(
			ProjectileClass, LaunchLocation, AimDir.Rotation(), Params);
		if (Projectile)
		{
			Projectile->InitSwapProjectile(Hunter, LaunchLocation, TargetLocation,
				FinalTravelDuration,
				FinalRadius,
				bEmpowered);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UGA_Void_Shift::ConsumeEmpowered() const
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

const FGameplayTagContainer* UGA_Void_Shift::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Void_Shift::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
