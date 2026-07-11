#include "GA_Kingpin_Shift.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBDamageExecution.h"
#include "Breachborne/Breachborne.h"

UGA_Kingpin_Shift::UGA_Kingpin_Shift()
{
	AbilityInputTag    = BBGameplayTags::InputTag_Shift;
	bActivateOnInputHeld = false;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Kingpin_Shift);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Kingpin_Shift);
}

void UGA_Kingpin_Shift::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
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

	// Cache for EndCharge
	CachedHandle         = Handle;
	CachedActorInfo      = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	HitActors.Reset();

	// Apply State_Charging tag (Infinite — removed in EndCharge)
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (SourceASC)
	{
		SourceASC->AddLooseGameplayTag(BBGameplayTags::State_Charging);
	}

	// Charge follows movement input (WASD), not cursor — see GA_Ghost_Shift for the why.
	// Same fallback to actor forward when no input is held.
	FVector ChargeDir = Hunter->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();
	if (ChargeDir.IsNearlyZero())
	{
		ChargeDir = Hunter->GetActorForwardVector().GetSafeNormal2D();
	}
	const float StartMontageDuration = PlayVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_Shift, EBBAbilityAnimationPhase::Start);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_Shift_Start, Hunter->GetActorLocation(), ChargeDir);
	AddVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_Shift_Trail, Hunter->GetActorLocation(), ChargeDir);
	if (StartMontageDuration > 0.05f && StartMontageDuration < ChargeDuration)
	{
		if (UWorld* MontageWorld = GetWorld())
		{
			MontageWorld->GetTimerManager().SetTimer(ChargeLoopMontageHandle, this, &UGA_Kingpin_Shift::PlayChargeLoopMontage, StartMontageDuration, false);
		}
	}
	else
	{
		PlayChargeLoopMontage();
	}
	Hunter->LaunchCharacter(ChargeDir * ChargeSpeed, true, false);

	// Tick every 0.05s to check for mid-charge collisions
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(ChargeTickHandle, this, &UGA_Kingpin_Shift::ChargeTick, 0.05f, true);
		World->GetTimerManager().SetTimer(ChargeEndHandle,  this, &UGA_Kingpin_Shift::EndCharge, ChargeDuration, false);
	}
}

void UGA_Kingpin_Shift::ChargeTick()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !Hunter->HasAuthority())
	{
		return;
	}

	const ABreachbornePlayerState* KingpinPS = GetBBPlayerState();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!KingpinPS || !SourceASC)
	{
		return;
	}

	// Small sphere cast at character location for collision detection
	const FVector Origin = Hunter->GetActorLocation();
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(KingpinCharge), false, Hunter);
	Hunter->GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(80.0f), Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor || HitActors.Contains(HitActor))
		{
			continue;
		}

		AHunterCharacter* HitHunter = Cast<AHunterCharacter>(HitActor);
		if (!HitHunter)
		{
			continue;
		}

		const ABreachbornePlayerState* TargetPS = HitHunter->GetPlayerState<ABreachbornePlayerState>();
		if (!TargetPS || TargetPS->GetTeamID() == KingpinPS->GetTeamID())
		{
			continue;
		}

		HitActors.Add(HitActor);

		UAbilitySystemComponent* TargetASC = HitHunter->GetAbilitySystemComponent();
		if (!TargetASC)
		{
			continue;
		}

		// Knockback
		const FVector KBDir = (HitHunter->GetActorLocation() - Hunter->GetActorLocation()).GetSafeNormal();
		HitHunter->LaunchCharacter(KBDir * KnockbackForce + FVector(0.0f, 0.0f, 300.0f), true, true);

		// Damage
		UGameplayEffect* DmgGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_Kingpin_Charge_Damage"));
		DmgGE->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayEffectExecutionDefinition ExecDef;
		ExecDef.CalculationClass = UBBDamageExecution::StaticClass();
		DmgGE->Executions.Add(ExecDef);

		FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
		FGameplayEffectSpec Spec(DmgGE, Ctx, 1.0f);
		Spec.SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, ChargeDamage);
		TargetASC->ApplyGameplayEffectSpecToSelf(Spec);

		UE_LOG(LogBreachborne, Log, TEXT("Kingpin Shift: Charged through %s for %.0f dmg"), *HitHunter->GetName(), ChargeDamage);
	}
}

void UGA_Kingpin_Shift::EndCharge()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(ChargeTickHandle);
		World->GetTimerManager().ClearTimer(ChargeEndHandle);
		World->GetTimerManager().ClearTimer(ChargeLoopMontageHandle);
	}

	// Remove State_Charging
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (SourceASC)
	{
		SourceASC->RemoveLooseGameplayTag(BBGameplayTags::State_Charging);
	}

	if (CachedActorInfo)
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
	}
}

void UGA_Kingpin_Shift::PlayChargeLoopMontage()
{
	if (IsActive())
	{
		PlayVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_Shift, EBBAbilityAnimationPhase::Loop);
	}
}

void UGA_Kingpin_Shift::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChargeTickHandle);
		World->GetTimerManager().ClearTimer(ChargeEndHandle);
		World->GetTimerManager().ClearTimer(ChargeLoopMontageHandle);
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (SourceASC)
	{
		SourceASC->RemoveLooseGameplayTag(BBGameplayTags::State_Charging);
	}

	StopVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_Shift, EBBAbilityAnimationPhase::Loop);
	RemoveVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_Shift_Trail);
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_Shift, EBBAbilityAnimationPhase::End);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

const FGameplayTagContainer* UGA_Kingpin_Shift::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Kingpin_Shift::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
