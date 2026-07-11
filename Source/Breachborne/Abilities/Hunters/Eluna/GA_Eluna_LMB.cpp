#include "GA_Eluna_LMB.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBElunaCrescentProjectile.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Breachborne.h"

UGA_Eluna_LMB::UGA_Eluna_LMB()
{
	AbilityInputTag = BBGameplayTags::InputTag_LMB;
	bActivateOnInputHeld = true;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Eluna_LMB);
	SetAssetTags(AssetTags);

	DamageEffectClass = UBBDamageEffect::StaticClass();
	ProjectileClass = ABBElunaCrescentProjectile::StaticClass();
}

void UGA_Eluna_LMB::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Eluna_LMB, EBBAbilityAnimationPhase::Loop);
	// Fire first shot immediately
	FireShot();

	// Start repeating fire timer
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().SetTimer(
			FireTimerHandle,
			this,
			&UGA_Eluna_LMB::OnFireTimerTick,
			FireInterval,
			true
		);
	}
}

void UGA_Eluna_LMB::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}

	CurrentHoldStacks = 0;
	StopVisualMontage(BBGameplayTags::Ability_Hunter_Eluna_LMB, EBBAbilityAnimationPhase::Loop);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Eluna_LMB::FireShot()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		return;
	}

	const FVector AimDir = GetAimDirection();
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Eluna_LMB_Fire, Hunter->GetActorLocation(), AimDir);

	// Server: spawn the projectile
	if (Hunter->HasAuthority())
	{
		UAbilitySystemComponent* SourceASCPtr = GetAbilitySystemComponentFromActorInfo();
		const ABreachbornePlayerState* ShooterPS = GetBBPlayerState();

		if (!SourceASCPtr || !ShooterPS || !ProjectileClass)
		{
			UE_LOG(LogBreachborne, Warning, TEXT("Eluna LMB FireShot: SourceASC=%s ShooterPS=%s ProjectileClass=%s"),
				SourceASCPtr ? TEXT("OK") : TEXT("NULL"), ShooterPS ? TEXT("OK") : TEXT("NULL"), ProjectileClass ? TEXT("OK") : TEXT("NULL"));
			return;
		}

		float DamageMultiplier = 1.0f + (CurrentHoldStacks * DamageBoostPerStack);
		float FinalDamage = BaseDamage * DamageMultiplier;

		const FVector SpawnLocation = Hunter->GetActorLocation() + AimDir * SpawnForwardOffset;
		const FRotator SpawnRotation = AimDir.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Hunter;
		SpawnParams.Instigator = Hunter;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABBProjectile* Projectile = Hunter->GetWorld()->SpawnActor<ABBProjectile>(
			ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

		if (Projectile)
		{
			Projectile->InitProjectile(SourceASCPtr, DamageEffectClass, FinalDamage, ShooterPS->GetTeamID());
			Projectile->FireInDirection(AimDir);
		}
		else
		{
		}
	}

	// Increment hold stacks (capped)
	CurrentHoldStacks = FMath::Min(CurrentHoldStacks + 1, MaxHoldStacks);
}

void UGA_Eluna_LMB::OnFireTimerTick()
{
	if (!IsActive())
	{
		if (const AHunterCharacter* Hunter = GetHunterCharacter())
		{
			Hunter->GetWorldTimerManager().ClearTimer(FireTimerHandle);
		}
		return;
	}

	FireShot();
}
