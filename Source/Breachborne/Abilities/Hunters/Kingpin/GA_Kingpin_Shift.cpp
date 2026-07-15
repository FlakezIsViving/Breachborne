#include "GA_Kingpin_Shift.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBDamageExecution.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Combat/BBPrimitiveWedgeActor.h"
#include "Breachborne/Breachborne.h"

UGA_Kingpin_Shift::UGA_Kingpin_Shift()
{
	AbilityInputTag    = BBGameplayTags::InputTag_Shift;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Movement, ChargeSpeed * ChargeDuration, 70.0f);
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
	ChargeStartLocation = Hunter->GetActorLocation();
	ChargeDirection = ChargeDir;
	PreviousChargeLocation = ChargeStartLocation;
	ChargeStartTimeSeconds = Hunter->GetWorld() ? Hunter->GetWorld()->GetTimeSeconds() : 0.0f;
	bChargePathBlocked = false;
	const float StartMontageDuration = PlayVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_Shift, EBBAbilityAnimationPhase::Start);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_Shift_Start, Hunter->GetActorLocation(), ChargeDir);
	AddVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_Shift_Trail, Hunter->GetActorLocation(), ChargeDir);
	if (Hunter->HasAuthority())
	{
		const FVector ChargeStart = Hunter->GetActorLocation();
		const float ChargeRange = ChargeSpeed * ChargeDuration;
		FActorSpawnParameters VisualParams;
		VisualParams.Owner = Hunter;
		VisualParams.Instigator = Hunter;
		VisualParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveWedgeActor* ChargeWedge = Hunter->GetWorld()->SpawnActor<ABBPrimitiveWedgeActor>(
			ABBPrimitiveWedgeActor::StaticClass(), ChargeStart, FRotator::ZeroRotator, VisualParams))
		{
			ChargeWedge->InitWedge(ChargeStart, ChargeDir, ChargeRange, 12.0f, 8.0f, ChargeDuration,
				FLinearColor(0.90f, 0.23f, 0.18f, 1.0f));
		}
		if (ABBPrimitiveBeamActor* ChargeTrail = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
			ABBPrimitiveBeamActor::StaticClass(), ChargeStart, FRotator::ZeroRotator, VisualParams))
		{
			ChargeTrail->InitBeam(ChargeStart, ChargeStart + ChargeDir * ChargeRange, 12.0f, ChargeDuration,
				FLinearColor(1.0f, 0.69f, 0.13f, 1.0f));
		}
	}
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

	// Advance an authoritative gameplay path independently of autonomous-proxy movement reconciliation.
	UWorld* World = Hunter->GetWorld();
	if (!World)
	{
		return;
	}
	const FVector SweepStart = PreviousChargeLocation;
	FVector Origin = SweepStart;
	if (!bChargePathBlocked)
	{
		const float ElapsedSeconds = FMath::Clamp(
			World->GetTimeSeconds() - ChargeStartTimeSeconds, 0.0f, ChargeDuration);
		const FVector DesiredEnd = ChargeStartLocation + ChargeDirection * ChargeSpeed * ElapsedSeconds;
		if (!DesiredEnd.Equals(SweepStart, 0.1f))
		{
			FCollisionObjectQueryParams WorldObjects;
			WorldObjects.AddObjectTypesToQuery(ECC_WorldStatic);
			WorldObjects.AddObjectTypesToQuery(ECC_WorldDynamic);
			FCollisionQueryParams WorldParams(SCENE_QUERY_STAT(KingpinChargeWorld), false, Hunter);
			const float WorldSweepRadius = Hunter->GetCapsuleComponent()
				? Hunter->GetCapsuleComponent()->GetScaledCapsuleRadius() : 42.0f;
			FHitResult BlockingHit;
			if (World->SweepSingleByObjectType(BlockingHit, SweepStart, DesiredEnd, FQuat::Identity,
				WorldObjects, FCollisionShape::MakeSphere(WorldSweepRadius), WorldParams))
			{
				Origin = BlockingHit.Location;
				bChargePathBlocked = true;
			}
			else
			{
				Origin = DesiredEnd;
			}
		}
	}

	TSet<AActor*> CandidateActors;
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(KingpinCharge), false, Hunter);
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(ChargeHitRadius), Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AActor* OverlapActor = Overlap.GetActor())
		{
			CandidateActors.Add(OverlapActor);
		}
	}

	TArray<FHitResult> SweepHits;
	World->SweepMultiByObjectType(SweepHits, SweepStart, Origin, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(ChargeHitRadius), Params);
	for (const FHitResult& SweepHit : SweepHits)
	{
		if (AActor* SweepActor = SweepHit.GetActor())
		{
			CandidateActors.Add(SweepActor);
		}
	}
	for (TActorIterator<AHunterCharacter> It(World); It; ++It)
	{
		AHunterCharacter* Candidate = *It;
		if (Candidate == Hunter)
		{
			continue;
		}

		const FVector ClosestPoint = FMath::ClosestPointOnSegment(
			Candidate->GetActorLocation(), SweepStart, Origin);
		const float CandidateDistance = FVector::Dist2D(Candidate->GetActorLocation(), ClosestPoint);
		if (CandidateDistance <= ChargeHitRadius)
		{
			CandidateActors.Add(Candidate);
		}
	}
	PreviousChargeLocation = Origin;

	for (AActor* HitActor : CandidateActors)
	{
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
		FActorSpawnParameters ImpactParams;
		ImpactParams.Owner = Hunter;
		ImpactParams.Instigator = Hunter;
		ImpactParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBurstActor* Impact = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
			ABBPrimitiveBurstActor::StaticClass(), HitHunter->GetActorLocation(), FRotator::ZeroRotator, ImpactParams))
		{
			Impact->InitBurst(HitHunter->GetActorLocation(), 100.0f, 0.22f,
				FLinearColor(1.0f, 0.69f, 0.13f, 1.0f));
		}

		UE_LOG(LogBreachborne, Log, TEXT("Kingpin Shift: Charged through %s for %.0f dmg"), *HitHunter->GetName(), ChargeDamage);
	}
}

void UGA_Kingpin_Shift::EndCharge()
{
	if (const AHunterCharacter* Hunter = GetHunterCharacter(); Hunter && Hunter->HasAuthority())
	{
		ChargeTick();
	}

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
		const AHunterCharacter* Hunter = GetHunterCharacter();
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo,
			Hunter && Hunter->HasAuthority(), false);
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
