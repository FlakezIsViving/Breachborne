#include "GA_Ghost_LMB.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBProjectile.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Breachborne.h"
#include "DrawDebugHelpers.h"

UGA_Ghost_LMB::UGA_Ghost_LMB()
{
	AbilityInputTag = BBGameplayTags::InputTag_LMB;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, 1500.0f);
	bActivateOnInputHeld = true;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Ghost_LMB);
	SetAssetTags(AssetTags);

	DamageEffectClass = UBBDamageEffect::StaticClass();
	ProjectileClass = ABBProjectile::StaticClass();
}

void UGA_Ghost_LMB::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogBreachborne, Verbose, TEXT("Ghost LMB: ActivateAbility called"));

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Ghost LMB: CommitAbility FAILED"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogBreachborne, Verbose, TEXT("Ghost LMB: CommitAbility succeeded — starting fire loop"));

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Ghost_LMB, EBBAbilityAnimationPhase::Loop);

	// Fire first shot immediately
	FireShot();

	// Start repeating fire timer
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().SetTimer(
			FireTimerHandle,
			this,
			&UGA_Ghost_LMB::OnFireTimerTick,
			FireInterval,
			true // looping
		);
	}
}

void UGA_Ghost_LMB::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogBreachborne, Verbose, TEXT("Ghost LMB: EndAbility (Cancelled=%s)"), bWasCancelled ? TEXT("YES") : TEXT("NO"));

	// Clear the fire timer
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}
	StopVisualMontage(BBGameplayTags::Ability_Hunter_Ghost_LMB, EBBAbilityAnimationPhase::Loop);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Ghost_LMB::FireShot()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Ghost LMB FireShot: GetHunterCharacter() returned nullptr"));
		return;
	}

	const FVector AimDir = GetAimDirection();
	const bool bIsServer = Hunter->HasAuthority();
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Ghost_LMB_Fire, Hunter->GetActorLocation(), AimDir);

	// Client: draw debug line for muzzle flash direction (cosmetic only)
	if (!bIsServer)
	{
		const FVector StartLocation = Hunter->GetActorLocation();
		DrawDebugLine(Hunter->GetWorld(), StartLocation, StartLocation + AimDir * 200.0f, FColor::Yellow, false, 0.15f, 0, 2.0f);
	}

	// Server: spawn the projectile
	if (bIsServer)
	{
		UAbilitySystemComponent* SourceASCPtr = GetAbilitySystemComponentFromActorInfo();
		const ABreachbornePlayerState* ShooterPS = GetBBPlayerState();

		if (!SourceASCPtr || !ShooterPS)
		{
			UE_LOG(LogBreachborne, Warning, TEXT("Ghost LMB FireShot: SourceASC=%s ShooterPS=%s"),
				SourceASCPtr ? TEXT("valid") : TEXT("NULL"), ShooterPS ? TEXT("valid") : TEXT("NULL"));
			return;
		}

		if (!ProjectileClass)
		{
			UE_LOG(LogBreachborne, Warning, TEXT("Ghost LMB FireShot: ProjectileClass is null"));
			return;
		}

		const FVector SpawnLocation = Hunter->GetActorLocation() + AimDir * SpawnForwardOffset;
		const FRotator SpawnRotation = AimDir.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Hunter;
		SpawnParams.Instigator = Hunter;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABBProjectile* Projectile = Hunter->GetWorld()->SpawnActor<ABBProjectile>(
			ProjectileClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);

		if (Projectile)
		{
			Projectile->InitProjectile(SourceASCPtr, DamageEffectClass, BaseDamagePerShot, ShooterPS->GetTeamID());
			Projectile->SetImpactCueTag(BBGameplayTags::GameplayCue_Hunter_Ghost_LMB_Impact);
			Projectile->FireInDirection(AimDir);

			UE_LOG(LogBreachborne, Verbose, TEXT("Ghost LMB: Spawned projectile at %s dir %s"),
				*SpawnLocation.ToString(), *AimDir.ToString());
		}
	}
}

void UGA_Ghost_LMB::OnFireTimerTick()
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
