#include "GA_Kingpin_LMB.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBDamageExecution.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Combat/BBPrimitiveWedgeActor.h"
#include "Breachborne/Breachborne.h"

UGA_Kingpin_LMB::UGA_Kingpin_LMB()
{
	AbilityInputTag    = BBGameplayTags::InputTag_LMB;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, MeleeRange);
	bActivateOnInputHeld = true;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Kingpin_LMB);
	SetAssetTags(AssetTags);
}

void UGA_Kingpin_LMB::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_LMB, EBBAbilityAnimationPhase::Loop);
	// Immediate first strike, then repeat at FireInterval while input is held
	PerformStrike();

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(StrikeTimerHandle, this, &UGA_Kingpin_LMB::PerformStrike, FireInterval, true);
	}
}

void UGA_Kingpin_LMB::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StrikeTimerHandle);
	}
	StopVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_LMB, EBBAbilityAnimationPhase::Loop);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Kingpin_LMB::PerformStrike()
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

	const FVector Origin  = Hunter->GetActorLocation();
	const FVector Forward = Hunter->GetActorForwardVector().GetSafeNormal2D();
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_LMB_Fire, Origin, Forward);
	FActorSpawnParameters VisualParams;
	VisualParams.Owner = Hunter;
	VisualParams.Instigator = Hunter;
	VisualParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveWedgeActor* Swipe = Hunter->GetWorld()->SpawnActor<ABBPrimitiveWedgeActor>(
		ABBPrimitiveWedgeActor::StaticClass(), Origin, FRotator::ZeroRotator, VisualParams))
	{
		Swipe->InitWedge(Origin, Forward, MeleeRange, 72.5f, 7.0f, 0.18f,
			FLinearColor(0.90f, 0.23f, 0.18f, 1.0f));
	}

	// Overlap sphere to find enemies in melee range
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(KingpinMelee), false, Hunter);
	Hunter->GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(MeleeRange), Params);

	// Find the closest enemy in the front hemisphere
	AActor*  PrimaryTarget    = nullptr;
	float    ClosestDot       = 0.3f; // must be roughly facing
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == Hunter)
		{
			continue;
		}

		// Team check
		AHunterCharacter* HitHunter = Cast<AHunterCharacter>(Candidate);
		if (!HitHunter)
		{
			continue;
		}
		const ABreachbornePlayerState* TargetPS = HitHunter->GetPlayerState<ABreachbornePlayerState>();
		if (!TargetPS || TargetPS->GetTeamID() == KingpinPS->GetTeamID())
		{
			continue;
		}
		if (TargetPS->GetAbilitySystemComponent()->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
		{
			continue;
		}

		const FVector ToTarget = (Candidate->GetActorLocation() - Origin).GetSafeNormal2D();
		const float   Dot      = FVector::DotProduct(Forward, ToTarget);
		if (Dot > ClosestDot)
		{
			ClosestDot    = Dot;
			PrimaryTarget = Candidate;
		}
	}

	if (!PrimaryTarget)
	{
		return;
	}

	IAbilitySystemInterface* PrimaryASI = Cast<IAbilitySystemInterface>(PrimaryTarget);
	UAbilitySystemComponent* PrimaryASC = PrimaryASI ? PrimaryASI->GetAbilitySystemComponent() : nullptr;
	if (PrimaryASC)
	{
		ApplyDamageToTarget(SourceASC, PrimaryASC, BaseDamage);
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_LMB_Impact, PrimaryTarget->GetActorLocation(), (PrimaryTarget->GetActorLocation() - Origin).GetSafeNormal());
		if (ABBPrimitiveBurstActor* Impact = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
			ABBPrimitiveBurstActor::StaticClass(), PrimaryTarget->GetActorLocation(), FRotator::ZeroRotator, VisualParams))
		{
			Impact->InitBurst(PrimaryTarget->GetActorLocation(), 70.0f, 0.2f,
				FLinearColor(1.0f, 0.69f, 0.13f, 1.0f));
		}
		UE_LOG(LogBreachborne, Log, TEXT("Kingpin LMB: Strike %s for %.0f"), *PrimaryTarget->GetName(), BaseDamage);

		// Chain bounce: find second enemy within ChainRadius of the primary target
		const FVector PrimaryLoc = PrimaryTarget->GetActorLocation();
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* ChainCandidate = Overlap.GetActor();
			if (!ChainCandidate || ChainCandidate == Hunter || ChainCandidate == PrimaryTarget)
			{
				continue;
			}

			AHunterCharacter* ChainHunter = Cast<AHunterCharacter>(ChainCandidate);
			if (!ChainHunter)
			{
				continue;
			}
			const ABreachbornePlayerState* ChainPS = ChainHunter->GetPlayerState<ABreachbornePlayerState>();
			if (!ChainPS || ChainPS->GetTeamID() == KingpinPS->GetTeamID())
			{
				continue;
			}

			if (FVector::Dist(PrimaryLoc, ChainCandidate->GetActorLocation()) <= ChainRadius)
			{
				IAbilitySystemInterface* ChainASI = Cast<IAbilitySystemInterface>(ChainCandidate);
				UAbilitySystemComponent* ChainASC = ChainASI ? ChainASI->GetAbilitySystemComponent() : nullptr;
				if (ChainASC)
				{
					const float ChainDmg = BaseDamage * ChainDamageFraction;
					ApplyDamageToTarget(SourceASC, ChainASC, ChainDmg);
					if (ABBPrimitiveBeamActor* Chain = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
						ABBPrimitiveBeamActor::StaticClass(), PrimaryLoc, FRotator::ZeroRotator, VisualParams))
					{
						Chain->InitBeam(PrimaryLoc, ChainCandidate->GetActorLocation(), 6.0f, 0.2f,
							FLinearColor(1.0f, 0.69f, 0.13f, 1.0f));
					}
					UE_LOG(LogBreachborne, Log, TEXT("Kingpin LMB: Chain to %s for %.0f"), *ChainCandidate->GetName(), ChainDmg);
					break; // One chain bounce only
				}
			}
		}
	}
}

void UGA_Kingpin_LMB::ApplyDamageToTarget(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float Damage)
{
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	UGameplayEffect* DmgGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_Kingpin_Strike"));
	DmgGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UBBDamageExecution::StaticClass();
	DmgGE->Executions.Add(ExecDef);

	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddInstigator(SourceASC->GetOwnerActor(), SourceASC->GetOwnerActor());

	FGameplayEffectSpec Spec(DmgGE, Ctx, 1.0f);
	Spec.SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, Damage);

	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
}
