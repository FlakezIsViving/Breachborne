#include "GA_Eluna_R.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Combat/BBHealEffect.h"
#include "Breachborne/Combat/BBElunaReviveBeam.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Wisp/BBWispPawn.h"
#include "Breachborne/Breachborne.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"

UGA_Eluna_R::UGA_Eluna_R()
{
	AbilityInputTag = BBGameplayTags::InputTag_R;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, MaxChannelRange);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Eluna_R);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Eluna_R);

	HealEffectClass = UBBHealEffect::StaticClass();
}

void UGA_Eluna_R::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	ABBWispPawn* WispPawn = Hunter ? nullptr : Cast<ABBWispPawn>(CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr);

	UE_LOG(LogBreachborne, Log, TEXT("Eluna R: Activate | Hunter=%s | WispPawn=%s"),
		Hunter ? *Hunter->GetName() : TEXT("NULL"),
		WispPawn ? *WispPawn->GetName() : TEXT("NULL"));

	// If the player is currently a wisp, self-revive immediately (old fallback)
	if (WispPawn)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Eluna R: Self-revive from wisp form | Wisp=%s"), *WispPawn->GetName());
		WispPawn->OnWispReviveReady.Broadcast(WispPawn);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Eluna R: CommitAbility failed — ending"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!Hunter)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Eluna R: No HunterCharacter — ending"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Find target
	AActor* Target = FindBestTarget();
	if (!Target)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Eluna R: No valid target in range %.0f — ending"), TargetSearchRange);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartChannel(Target);
}

AActor* UGA_Eluna_R::FindBestTarget() const
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		return nullptr;
	}

	const FVector SourceLoc = Hunter->GetActorLocation();
	const int32 SourceTeam = GetBBPlayerState() ? GetBBPlayerState()->GetTeamID() : -1;

	float BestWispDistSq = TargetSearchRange * TargetSearchRange;
	ABBWispPawn* BestWisp = nullptr;

	float BestAllyDistSq = TargetSearchRange * TargetSearchRange;
	AHunterCharacter* BestAlly = nullptr;

	// Search all actors in the world
	for (TActorIterator<AActor> It(Hunter->GetWorld()); It; ++It)
	{
		// Check for wisp pawns first (dead allies)
		if (ABBWispPawn* Wisp = Cast<ABBWispPawn>(*It))
		{
			if (Wisp->GetOwningPlayerState() && Wisp->GetOwningPlayerState()->GetTeamID() == SourceTeam)
			{
				float DistSq = FVector::DistSquared(SourceLoc, Wisp->GetActorLocation());
				if (DistSq < BestWispDistSq)
				{
					BestWispDistSq = DistSq;
					BestWisp = Wisp;
				}
			}
			continue;
		}

		// Check for alive hunters that need healing
		if (AHunterCharacter* Ally = Cast<AHunterCharacter>(*It))
		{
			if (Ally == Hunter)
			{
				continue; // skip self
			}

			ABreachbornePlayerState* AllyPS = Ally->GetPlayerState<ABreachbornePlayerState>();
			if (!AllyPS || !AllyPS->GetIsAlive() || AllyPS->GetTeamID() != SourceTeam)
			{
				continue;
			}

			// Only target allies below max health
			UBBHealthSet* HealthSet = AllyPS->GetHealthSet();
			if (HealthSet && HealthSet->GetHealth() < HealthSet->GetMaxHealth())
			{
				float DistSq = FVector::DistSquared(SourceLoc, Ally->GetActorLocation());
				if (DistSq < BestAllyDistSq)
				{
					BestAllyDistSq = DistSq;
					BestAlly = Ally;
				}
			}
		}
	}

	// Priority: wisp > alive ally needing heal
	if (BestWisp)
	{
		UE_LOG(LogBreachborne, Log, TEXT("Eluna R: Target found — WISP %s (dist=%.0f)"),
			*BestWisp->GetName(), FMath::Sqrt(BestWispDistSq));
		return BestWisp;
	}

	if (BestAlly)
	{
		UE_LOG(LogBreachborne, Log, TEXT("Eluna R: Target found — ALLY %s (dist=%.0f)"),
			*BestAlly->GetName(), FMath::Sqrt(BestAllyDistSq));
		return BestAlly;
	}

	return nullptr;
}

void UGA_Eluna_R::StartChannel(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	ChannelTarget = Target;
	TargetWisp = Cast<ABBWispPawn>(Target);
	bIsChanneling = true;
	ChannelElapsed = 0.0f;

	UE_LOG(LogBreachborne, Warning, TEXT("Eluna R: CHANNEL STARTED | Target=%s | Duration=%.1fs | Heal=%.0f/s | Range=%.0f"),
		*Target->GetName(), ChannelDuration, HealPerSecond, MaxChannelRange);

	const float StartMontageDuration = PlayVisualMontage(BBGameplayTags::Ability_Hunter_Eluna_R, EBBAbilityAnimationPhase::Start);
	if (StartMontageDuration > 0.05f && StartMontageDuration < ChannelDuration)
	{
		GetWorld()->GetTimerManager().SetTimer(
			ChannelLoopMontageHandle,
			this,
			&UGA_Eluna_R::PlayChannelLoopMontage,
			StartMontageDuration,
			false
		);
	}
	else
	{
		PlayChannelLoopMontage();
	}
	if (AHunterCharacter* Hunter = GetHunterCharacter())
	{
		AddVisualCue(BBGameplayTags::GameplayCue_Hunter_Eluna_R_Beam, Hunter->GetActorLocation(), FVector::UpVector);
	}

	// Spawn beam visual
	SpawnBeam();

	// Start channel tick timer (heal + range check)
	GetWorld()->GetTimerManager().SetTimer(
		ChannelTickHandle,
		this,
		&UGA_Eluna_R::OnChannelTick,
		ChannelTickInterval,
		true
	);

	// Start completion timer
	GetWorld()->GetTimerManager().SetTimer(
		ChannelCompleteHandle,
		this,
		&UGA_Eluna_R::OnChannelComplete,
		ChannelDuration,
		false
	);
}

void UGA_Eluna_R::OnChannelTick()
{
	if (!bIsChanneling)
	{
		return;
	}

	ChannelElapsed += ChannelTickInterval;

	// Check target validity
	if (!ChannelTarget.IsValid())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Eluna R: Channel broken — target destroyed"));
		BreakChannel();
		return;
	}

	// Check range
	if (GetDistanceToTarget() > MaxChannelRange)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Eluna R: Channel broken — target out of range (%.0f > %.0f)"),
			GetDistanceToTarget(), MaxChannelRange);
		BreakChannel();
		return;
	}

	// Apply heal tick
	ApplyHealTick(ChannelTickInterval);

	// Update beam visual
	UpdateBeam();

	// If target is a wisp and already filled, we could early-complete, but let's keep the full channel
	// for consistency (the 3s channel IS the revive time)
}

void UGA_Eluna_R::ApplyHealTick(float DeltaTime)
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !Hunter->HasAuthority())
	{
		return;
	}

	AActor* Target = ChannelTarget.Get();
	if (!Target)
	{
		return;
	}

	const float HealAmount = HealPerSecond * DeltaTime;

	// Heal wisp — converted to revive progress
	if (ABBWispPawn* Wisp = Cast<ABBWispPawn>(Target))
	{
		Wisp->ApplyHeal(HealAmount);
		return;
	}

	// Heal alive ally via gameplay effect
	if (AHunterCharacter* Ally = Cast<AHunterCharacter>(Target))
	{
		UBBAbilitySystemComponent* AllyASC = Cast<UBBAbilitySystemComponent>(Ally->GetAbilitySystemComponent());
		if (HealEffectClass && AllyASC)
		{
			FGameplayEffectSpecHandle SpecHandle = AllyASC->MakeOutgoingSpec(HealEffectClass, 1.0f, AllyASC->MakeEffectContext());
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_HealAmount.GetTag(), HealAmount);
				AllyASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}

void UGA_Eluna_R::OnChannelComplete()
{
	if (!bIsChanneling)
	{
		return;
	}

	UE_LOG(LogBreachborne, Warning, TEXT("Eluna R: CHANNEL COMPLETE | Target=%s | Elapsed=%.1fs"),
		ChannelTarget.IsValid() ? *ChannelTarget->GetName() : TEXT("NULL"), ChannelElapsed);

	// Revive if target is a wisp
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (Hunter && Hunter->HasAuthority() && TargetWisp.IsValid())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Eluna R: REVIVING wisp of %s"),
			TargetWisp->GetOwningPlayerState() ? *TargetWisp->GetOwningPlayerState()->GetPlayerName() : TEXT("Unknown"));
		TargetWisp->OnWispReviveReady.Broadcast(TargetWisp.Get());
	}
	else if (ChannelTarget.IsValid())
	{
		UE_LOG(LogBreachborne, Log, TEXT("Eluna R: Heal channel complete on %s"), *ChannelTarget->GetName());
	}

	if (Hunter && Hunter->HasAuthority() && ChannelTarget.IsValid())
	{
		const FVector TargetLocation = ChannelTarget->GetActorLocation();
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBurstActor* Burst = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
			ABBPrimitiveBurstActor::StaticClass(), TargetLocation, FRotator::ZeroRotator, Params))
		{
			Burst->InitBurst(TargetLocation, 140.0f, 0.35f, FLinearColor::White);
		}
	}

	EndChannel(false);
}

void UGA_Eluna_R::BreakChannel()
{
	UE_LOG(LogBreachborne, Warning, TEXT("Eluna R: Channel BROKEN after %.1fs"), ChannelElapsed);
	EndChannel(true);
}

void UGA_Eluna_R::EndChannel(bool bWasCancelled)
{
	bIsChanneling = false;
	ChannelElapsed = 0.0f;

	GetWorld()->GetTimerManager().ClearTimer(ChannelTickHandle);
	GetWorld()->GetTimerManager().ClearTimer(ChannelCompleteHandle);
	GetWorld()->GetTimerManager().ClearTimer(ChannelLoopMontageHandle);

	StopVisualMontage(BBGameplayTags::Ability_Hunter_Eluna_R, EBBAbilityAnimationPhase::Loop);
	RemoveVisualCue(BBGameplayTags::GameplayCue_Hunter_Eluna_R_Beam);
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Eluna_R, bWasCancelled ? EBBAbilityAnimationPhase::Cancel : EBBAbilityAnimationPhase::Success);
	DestroyBeam();

	ChannelTarget = nullptr;
	TargetWisp = nullptr;

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, bWasCancelled);
}

void UGA_Eluna_R::PlayChannelLoopMontage()
{
	if (bIsChanneling)
	{
		PlayVisualMontage(BBGameplayTags::Ability_Hunter_Eluna_R, EBBAbilityAnimationPhase::Loop);
	}
}

void UGA_Eluna_R::SpawnBeam()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !Hunter->HasAuthority() || !ChannelTarget.IsValid())
	{
		return;
	}

	if (UClass* BeamClass = ABBElunaReviveBeam::StaticClass())
	{
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		ABBElunaReviveBeam* Beam = Hunter->GetWorld()->SpawnActor<ABBElunaReviveBeam>(BeamClass, Hunter->GetActorLocation(), FRotator::ZeroRotator, Params);
		if (Beam)
		{
			ActiveBeam = Beam;
			Beam->SetBeamSource(Hunter->GetActorLocation());
			Beam->SetBeamTarget(ChannelTarget->GetActorLocation());
		}
	}
}

void UGA_Eluna_R::UpdateBeam()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !ActiveBeam.IsValid() || !ChannelTarget.IsValid())
	{
		return;
	}

	ActiveBeam->SetBeamSource(Hunter->GetActorLocation());
	ActiveBeam->SetBeamTarget(ChannelTarget->GetActorLocation());

	// Color gradient: green (close/safe) -> yellow (mid) -> red (near break)
	const float Dist = GetDistanceToTarget();
	const float Ratio = FMath::Clamp(Dist / MaxChannelRange, 0.0f, 1.0f);
	ActiveBeam->SetDistanceRatio(Ratio);
}

void UGA_Eluna_R::DestroyBeam()
{
	if (ActiveBeam.IsValid())
	{
		ActiveBeam->Destroy();
		ActiveBeam = nullptr;
	}
}

float UGA_Eluna_R::GetDistanceToTarget() const
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	AActor* Target = ChannelTarget.Get();
	if (!Hunter || !Target)
	{
		return MAX_FLT;
	}
	return FVector::Dist(Hunter->GetActorLocation(), Target->GetActorLocation());
}

void UGA_Eluna_R::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bIsChanneling)
	{
		EndChannel(bWasCancelled);
		return;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

const FGameplayTagContainer* UGA_Eluna_R::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Eluna_R::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
