#include "GA_Kingpin_RMB.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameFramework/Character.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBHookTagEffect.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Breachborne.h"

UGA_Kingpin_RMB::UGA_Kingpin_RMB()
{
	AbilityInputTag    = BBGameplayTags::InputTag_RMB;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, MaxRange);
	bActivateOnInputHeld = false;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Kingpin_RMB);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Kingpin_RMB);
}

void UGA_Kingpin_RMB::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
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

	const FVector StartLoc = Hunter->GetActorLocation();
	const FVector AimDir = GetAimDirection();
	const FVector EndLoc = StartLoc + AimDir * MaxRange;
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_RMB, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_RMB_Fire, StartLoc, AimDir);

	// Server-only: trace, apply hook tag, hand off the pull to the character.
	// Symmetric EndAbility at the bottom keeps LocalPredicted prediction happy.
	if (Hunter->HasAuthority())
	{
		const ABreachbornePlayerState* KingpinPS = GetBBPlayerState();
		UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(KingpinHook), false, Hunter);
		FCollisionObjectQueryParams ObjParams;
		ObjParams.AddObjectTypesToQuery(ECC_Pawn);

		const bool bHit = Hunter->GetWorld()->LineTraceSingleByObjectType(Hit, StartLoc, EndLoc, ObjParams, Params);
		const FVector HookEnd = bHit ? FVector(Hit.ImpactPoint) : EndLoc;
		FActorSpawnParameters VisualParams;
		VisualParams.Owner = Hunter;
		VisualParams.Instigator = Hunter;
		VisualParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBeamActor* HookLine = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
			ABBPrimitiveBeamActor::StaticClass(), StartLoc, FRotator::ZeroRotator, VisualParams))
		{
			HookLine->InitBeam(StartLoc, HookEnd, 9.0f, bHit ? MaxPullDuration : 0.28f,
				FLinearColor(0.90f, 0.23f, 0.18f, 1.0f));
		}

		AHunterCharacter* HitHunter = bHit ? Cast<AHunterCharacter>(Hit.GetActor()) : nullptr;
		bool bValidTarget = false;
		if (HitHunter)
		{
			const ABreachbornePlayerState* TargetPS = HitHunter->GetPlayerState<ABreachbornePlayerState>();
			bValidTarget = TargetPS
				&& KingpinPS
				&& TargetPS->GetTeamID() != KingpinPS->GetTeamID()
				&& !TargetPS->GetAbilitySystemComponent()->HasMatchingGameplayTag(BBGameplayTags::State_Dead);
		}

		if (bValidTarget && SourceASC)
		{
			UAbilitySystemComponent* TargetASC = HitHunter->GetAbilitySystemComponent();
			ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_RMB_Impact, Hit.ImpactPoint, Hit.ImpactNormal);
			if (ABBPrimitiveBurstActor* Latch = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
				ABBPrimitiveBurstActor::StaticClass(), Hit.ImpactPoint, FRotator::ZeroRotator, VisualParams))
			{
				Latch->InitBurst(Hit.ImpactPoint, 75.0f, 0.22f,
					FLinearColor(1.0f, 0.69f, 0.13f, 1.0f));
			}

			// 1. Hook tag GE (CDO-backed UBBHookTagEffect for replication safety, duration overridden per-spec)
			FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
			Context.AddInstigator(Hunter, Hunter);

			FGameplayEffectSpecHandle HookSpec = SourceASC->MakeOutgoingSpec(
				UBBHookTagEffect::StaticClass(), /*Level=*/1, Context);
			if (HookSpec.IsValid())
			{
				HookSpec.Data->SetDuration(HookTagDuration, /*bLockDuration=*/false);
				SourceASC->ApplyGameplayEffectSpecToTarget(*HookSpec.Data, TargetASC);
			}

			// 2. Notify passive (Iron Will)
			FGameplayEventData CCEvent;
			CCEvent.Instigator = Hunter;
			CCEvent.Target     = HitHunter;
			SourceASC->HandleGameplayEvent(BBGameplayTags::Event_CCApplied, &CCEvent);

			// 3. Hand the pull off to the character — see HunterCharacter.cpp comment for why.
			//    The ability ends below; the character's timer drives the pull, indicator,
			//    and impact damage. UBBDamageEffect is an instant CDO-backed Damage GE that
			//    routes through UBBDamageExecution, same as every other damage in the project.
			Hunter->BeginHookPull(
				HitHunter,
				PullSpeed,
				ImpactDistance,
				MaxPullDuration,
				PullTickInterval,
				UBBDamageEffect::StaticClass(),
				HookDamage,
				Context);

			UE_LOG(LogBreachborne, Log, TEXT("Kingpin RMB: Hooked %s — handed off to character pull"),
				*HitHunter->GetName());
		}
		else if (bHit)
		{
			UE_LOG(LogBreachborne, Log, TEXT("Kingpin RMB: hook hit %s but invalid target (friendly/dead)"),
				*Hit.GetActor()->GetName());
		}
		else
		{
			UE_LOG(LogBreachborne, Log, TEXT("Kingpin RMB: hook fired, no target in line trace"));
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Kingpin_RMB::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Kingpin_RMB::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
