#include "GA_Hudson_R_SalvageHook.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBAntiHealEffect.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBSlowEffect.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

UGA_Hudson_R_SalvageHook::UGA_Hudson_R_SalvageHook()
{
	AbilityInputTag = BBGameplayTags::InputTag_R;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, Range);
	bActivateOnInputHeld = false;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Hudson_R);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Hudson_R);
	DamageEffectClass = UBBDamageEffect::StaticClass();
	SlowEffectClass = UBBSlowEffect::StaticClass();
	AntiHealEffectClass = UBBAntiHealEffect::StaticClass();
}

void UGA_Hudson_R_SalvageHook::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	const ABreachbornePlayerState* SourcePS = GetBBPlayerState();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!Hunter || !SourcePS || !SourceASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	bExecuted = false;
	bReeling = false;
	ReelElapsedSeconds = 0.0f;
	ReelDurationSeconds = 0.0f;

	const FVector Start = Hunter->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
	const FVector AimDir = GetAimDirection();
	const FVector End = Start + AimDir * Range;
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_R, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_R_Fire, Start, AimDir);

	if (!Hunter->HasAuthority())
	{
		return;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HudsonSalvageHook), false, Hunter);
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);
	const bool bHit = Hunter->GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjParams, Params);
	const FVector HookEnd = bHit ? FVector(Hit.ImpactPoint) : End;
	FActorSpawnParameters VisualParams;
	VisualParams.Owner = Hunter;
	VisualParams.Instigator = Hunter;
	VisualParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveBeamActor* HookLine = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
		ABBPrimitiveBeamActor::StaticClass(), Start, FRotator::ZeroRotator, VisualParams))
	{
		HookLine->InitBeam(Start, HookEnd, 10.0f, 0.25f,
			FLinearColor(1.0f, 0.54f, 0.16f, 1.0f));
	}
	AHunterCharacter* TargetHunter = bHit ? Cast<AHunterCharacter>(Hit.GetActor()) : nullptr;
	const ABreachbornePlayerState* TargetPS = TargetHunter ? TargetHunter->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	UAbilitySystemComponent* TargetASC = TargetHunter ? TargetHunter->GetAbilitySystemComponent() : nullptr;

	if (!TargetHunter || !TargetPS || !TargetASC || TargetPS->GetTeamID() == SourcePS->GetTeamID())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(Hunter, Hunter);

	if (SlowEffectClass)
	{
		FGameplayEffectSpecHandle SlowSpec = SourceASC->MakeOutgoingSpec(SlowEffectClass, 1.0f, Context);
		if (SlowSpec.IsValid())
		{
			SlowSpec.Data->SetDuration(HookDuration, false);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SlowSpec.Data, TargetASC);
		}
	}
	if (AntiHealEffectClass)
	{
		FGameplayEffectSpecHandle AntiHealSpec = SourceASC->MakeOutgoingSpec(AntiHealEffectClass, 1.0f, Context);
		if (AntiHealSpec.IsValid())
		{
			AntiHealSpec.Data->SetDuration(HookDuration, false);
			SourceASC->ApplyGameplayEffectSpecToTarget(*AntiHealSpec.Data, TargetASC);
		}
	}

	TargetASC->AddLooseGameplayTag(BBGameplayTags::State_Hudson_Hooked);
	HookedTarget = TargetHunter;
	HookedTargetASC = TargetASC;
	if (ABBPrimitiveBeamActor* TetherLine = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
		ABBPrimitiveBeamActor::StaticClass(), Start, FRotator::ZeroRotator, VisualParams))
	{
		TetherLine->InitBeam(Start, TargetHunter->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f),
			12.0f, HookDuration, FLinearColor(0.31f, 0.64f, 0.78f, 1.0f));
	}
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_R_Impact, Hit.ImpactPoint, Hit.ImpactNormal);
	AddVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_R_Tether, Hit.ImpactPoint, AimDir);
	if (ABBPrimitiveBurstActor* Latch = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
		ABBPrimitiveBurstActor::StaticClass(), Hit.ImpactPoint, FRotator::ZeroRotator, VisualParams))
	{
		Latch->InitBurst(Hit.ImpactPoint, 55.0f, 0.24f, FLinearColor(1.0f, 0.82f, 0.4f, 1.0f));
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HookTickHandle, this, &UGA_Hudson_R_SalvageHook::HookTick, 0.1f, true, 0.0f);
		World->GetTimerManager().SetTimer(HookEndHandle, [this]()
		{
			if (CachedActorInfo)
			{
				EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
			}
		}, HookDuration, false);
	}
}

void UGA_Hudson_R_SalvageHook::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HookTickHandle);
		World->GetTimerManager().ClearTimer(HookEndHandle);
	}

	UAbilitySystemComponent* TargetASC = HookedTargetASC.Get();
	if (TargetASC)
	{
		TargetASC->SetLooseGameplayTagCount(BBGameplayTags::State_Hudson_Hooked, 0);
	}

	if (AHunterCharacter* Target = HookedTarget.Get())
	{
		const bool bTargetIsDead = TargetASC && TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead);
		if (!bTargetIsDead)
		{
			if (UCharacterMovementComponent* MoveComp = Target->GetCharacterMovement())
			{
				MoveComp->SetMovementMode(TargetPreviousMovementMode);
				MoveComp->Velocity = FVector::ZeroVector;
			}
		}
	}

	HookedTarget.Reset();
	HookedTargetASC.Reset();
	bReeling = false;
	RemoveVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_R_Tether);
	RemoveVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_R_Reel);
	StopVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_R, EBBAbilityAnimationPhase::Loop);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Hudson_R_SalvageHook::HookTick()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	AHunterCharacter* Target = HookedTarget.Get();
	UAbilitySystemComponent* TargetASC = HookedTargetASC.Get();
	if (!Hunter || !Target || !TargetASC)
	{
		if (CachedActorInfo)
		{
			EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, true);
		}
		return;
	}

	if (UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (SourceASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
		{
			if (CachedActorInfo)
			{
				EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, true);
			}
			return;
		}
	}

	if (bReeling)
	{
		ReelTarget();
		return;
	}

	if (const ABreachbornePlayerState* TargetPS = Target->GetPlayerState<ABreachbornePlayerState>())
	{
		if (const UBBHealthSet* HealthSet = TargetPS->GetHealthSet())
		{
			if (HealthSet->GetHealth() <= HealthSet->GetMaxHealth() * ExecuteHealthFraction)
			{
				StartReel();
			}
		}
	}
}

void UGA_Hudson_R_SalvageHook::StartReel()
{
	if (bReeling || bExecuted)
	{
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	AHunterCharacter* Target = HookedTarget.Get();
	if (!Hunter || !Target || !GetWorld())
	{
		return;
	}

	bReeling = true;
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_R, EBBAbilityAnimationPhase::Loop);
	AddVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_R_Reel, Target->GetActorLocation(), (Hunter->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D());
	ReelElapsedSeconds = 0.0f;
	ReelStartLocation = Target->GetActorLocation();

	const float Distance = FVector::Dist2D(ReelStartLocation, Hunter->GetActorLocation());
	ReelDurationSeconds = FMath::Clamp((Distance / FMath::Max(1.0f, Range)) * MaxReelSeconds, MinReelSeconds, MaxReelSeconds);
	FActorSpawnParameters VisualParams;
	VisualParams.Owner = Hunter;
	VisualParams.Instigator = Hunter;
	VisualParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector BeamStart = Hunter->GetActorLocation() + FVector(0.0f, 0.0f, 65.0f);
	const FVector BeamEnd = Target->GetActorLocation() + FVector(0.0f, 0.0f, 65.0f);
	if (ABBPrimitiveBeamActor* ReelLine = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
		ABBPrimitiveBeamActor::StaticClass(), BeamStart, FRotator::ZeroRotator, VisualParams))
	{
		ReelLine->InitBeam(BeamStart, BeamEnd, 14.0f, ReelDurationSeconds,
			FLinearColor(1.0f, 0.25f, 0.08f, 1.0f));
	}

	if (UCharacterMovementComponent* MoveComp = Target->GetCharacterMovement())
	{
		TargetPreviousMovementMode = MoveComp->MovementMode;
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	GetWorld()->GetTimerManager().ClearTimer(HookEndHandle);
	GetWorld()->GetTimerManager().ClearTimer(HookTickHandle);
	GetWorld()->GetTimerManager().SetTimer(HookTickHandle, this, &UGA_Hudson_R_SalvageHook::HookTick, ReelTickInterval, true, 0.0f);

}

void UGA_Hudson_R_SalvageHook::ReelTarget()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	AHunterCharacter* Target = HookedTarget.Get();
	UAbilitySystemComponent* TargetASC = HookedTargetASC.Get();
	if (!Hunter || !Target || !TargetASC)
	{
		return;
	}

	if (TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
	{
		if (CachedActorInfo)
		{
			EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
		}
		return;
	}

	ReelElapsedSeconds += ReelTickInterval;

	const FVector HunterLoc = Hunter->GetActorLocation();
	const FVector ToStart = (ReelStartLocation - HunterLoc).GetSafeNormal2D();
	const FVector ReelEndLocation = HunterLoc + ToStart * ReelStopDistance;
	const float Alpha = FMath::Clamp(ReelElapsedSeconds / FMath::Max(0.01f, ReelDurationSeconds), 0.0f, 1.0f);
	const float SmoothAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
	const FVector NewLocation = FMath::Lerp(ReelStartLocation, ReelEndLocation, SmoothAlpha);

	Target->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* MoveComp = Target->GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->ForceReplicationUpdate();
	}
	Target->ForceNetUpdate();

	const float ActualDistance = FVector::Dist2D(Target->GetActorLocation(), HunterLoc);
	if (ActualDistance <= ReelStopDistance + 20.0f)
	{
		FinishExecute();
		return;
	}

	if (Alpha >= 1.0f)
	{
		const FVector SnapLocation = HunterLoc + ToStart * ReelStopDistance;
		Target->SetActorLocation(SnapLocation, false, nullptr, ETeleportType::TeleportPhysics);
		Target->ForceNetUpdate();

		if (FVector::Dist2D(Target->GetActorLocation(), HunterLoc) <= ReelStopDistance + 20.0f)
		{
			FinishExecute();
		}
	}
}

void UGA_Hudson_R_SalvageHook::FinishExecute()
{
	if (bExecuted)
	{
		return;
	}
	bExecuted = true;

	AHunterCharacter* Hunter = GetHunterCharacter();
	AHunterCharacter* Target = HookedTarget.Get();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = HookedTargetASC.Get();
	if (!Hunter || !Target || !SourceASC || !TargetASC || !DamageEffectClass)
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, ExecuteDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
	}

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_R, EBBAbilityAnimationPhase::Impact);
	const FVector ExecuteLocation = Target->GetActorLocation() + FVector(0.0f, 0.0f, 85.0f);
	FActorSpawnParameters ExecuteParams;
	ExecuteParams.Owner = Hunter;
	ExecuteParams.Instigator = Hunter;
	ExecuteParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveBurstActor* ExecuteBurst = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
		ABBPrimitiveBurstActor::StaticClass(), ExecuteLocation, FRotator::ZeroRotator, ExecuteParams))
	{
		ExecuteBurst->InitBurst(ExecuteLocation, 105.0f, 0.35f,
			FLinearColor(1.0f, 0.25f, 0.08f, 1.0f));
	}
	ReduceOwnCooldownByFraction(ExecuteCooldownRefundFraction);

	if (CachedActorInfo)
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
	}
}

const FGameplayTagContainer* UGA_Hudson_R_SalvageHook::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Hudson_R_SalvageHook::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}

void UGA_Hudson_R_SalvageHook::ReduceOwnCooldownByFraction(float Fraction)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || Fraction <= 0.0f)
	{
		return;
	}

	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTagContainer);
	for (const FActiveGameplayEffectHandle& Handle : ASC->GetActiveEffects(Query))
	{
		const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handle);
		if (!ActiveGE)
		{
			continue;
		}

		const float WorldTime = ASC->GetWorld()->GetTimeSeconds();
		const float Remaining = ActiveGE->GetDuration() - (WorldTime - ActiveGE->StartWorldTime);
		const float NewRemaining = FMath::Max(0.0f, Remaining * (1.0f - Fraction));
		if (NewRemaining <= 0.0f)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
		else
		{
			const_cast<FActiveGameplayEffect*>(ActiveGE)->StartWorldTime = WorldTime - (ActiveGE->GetDuration() - NewRemaining);
		}
	}
}
