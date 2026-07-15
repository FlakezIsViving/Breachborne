#include "GA_Crysta_LMB.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBCrystaLMBProjectile.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBReverberationMarkEffect.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

UGA_Crysta_LMB::UGA_Crysta_LMB()
{
	AbilityInputTag = BBGameplayTags::InputTag_LMB;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, 1800.0f);
	bActivateOnInputHeld = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Crysta_LMB);
	SetAssetTags(AssetTags);

	ProjectileClass = ABBCrystaLMBProjectile::StaticClass();
	DamageEffectClass = UBBDamageEffect::StaticClass();
	MarkEffectClass = UBBReverberationMarkEffect::StaticClass();
}

void UGA_Crysta_LMB::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Crysta_LMB, EBBAbilityAnimationPhase::Loop);
	FireShot();
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().SetTimer(FireTimerHandle, this, &UGA_Crysta_LMB::FireShot, FireInterval, true);
	}
}

void UGA_Crysta_LMB::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}
	StopVisualMontage(BBGameplayTags::Ability_Hunter_Crysta_LMB, EBBAbilityAnimationPhase::Loop);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Crysta_LMB::FireShot()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Hunter || !ASC)
	{
		return;
	}

	const FVector AimDir = GetAimDirection();
	const bool bEmpowered = ASC->HasMatchingGameplayTag(BBGameplayTags::State_Crysta_EmpoweredLMB);
	ExecuteVisualCue(bEmpowered ? BBGameplayTags::GameplayCue_Hunter_Crysta_LMB_Empowered : BBGameplayTags::GameplayCue_Hunter_Crysta_LMB_Fire,
		Hunter->GetActorLocation(), AimDir);

	if (bEmpowered)
	{
		ASC->SetLooseGameplayTagCount(BBGameplayTags::State_Crysta_EmpoweredLMB, 0);
	}

	TArray<FVector> SpawnLocations;
	CalculateShotSpawnLocations(Hunter->GetActorLocation(), AimDir, bEmpowered,
		SpawnForwardOffset, EmpoweredShotSpacing, SpawnLocations);
	for (const FVector& SpawnLocation : SpawnLocations)
	{
		SpawnProjectile(AimDir, bEmpowered, SpawnLocation);
	}
}

void UGA_Crysta_LMB::CalculateShotSpawnLocations(const FVector& Origin, const FVector& Direction, bool bEmpowered,
	float ForwardOffset, float ShotSpacing, TArray<FVector>& OutLocations)
{
	OutLocations.Reset();
	const FVector ShotDirection = Direction.GetSafeNormal();
	if (ShotDirection.IsNearlyZero())
	{
		return;
	}

	if (!bEmpowered)
	{
		OutLocations.Add(Origin + ShotDirection * ForwardOffset);
		return;
	}

	const float HalfSpacing = FMath::Max(0.0f, ShotSpacing) * 0.5f;
	OutLocations.Add(Origin + ShotDirection * (ForwardOffset + HalfSpacing));
	OutLocations.Add(Origin + ShotDirection * FMath::Max(20.0f, ForwardOffset - HalfSpacing));
}

void UGA_Crysta_LMB::SpawnProjectile(const FVector& Direction, bool bEmpoweredShot, const FVector& SpawnLocation)
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	const ABreachbornePlayerState* PS = GetBBPlayerState();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Hunter || !Hunter->HasAuthority() || !PS || !ASC || !ProjectileClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = Hunter;
	Params.Instigator = Hunter;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABBCrystaLMBProjectile* Projectile = Hunter->GetWorld()->SpawnActor<ABBCrystaLMBProjectile>(
		ProjectileClass, SpawnLocation, Direction.Rotation(), Params);
	if (Projectile)
	{
		Projectile->InitCrystaProjectile(ASC, DamageEffectClass, MarkEffectClass, BaseDamage, DetonationDamage,
			PS->GetTeamID(), true, false, bEmpoweredShot);
		Projectile->FireInDirection(Direction);
	}
}
