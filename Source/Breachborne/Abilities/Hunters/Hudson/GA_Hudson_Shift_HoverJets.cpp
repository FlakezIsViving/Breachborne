#include "GA_Hudson_Shift_HoverJets.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

UGA_Hudson_Shift_HoverJets::UGA_Hudson_Shift_HoverJets()
{
	AbilityInputTag = BBGameplayTags::InputTag_Shift;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Movement, HoverSpeed * MaxHoverDuration, HitRadius);
	bActivateOnInputHeld = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Hudson_Shift);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Hudson_Shift);
	DamageEffectClass = UBBDamageEffect::StaticClass();
}

void UGA_Hudson_Shift_HoverJets::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Hunter || !ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	HitActors.Reset();

	HoverDirection = Hunter->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();
	if (HoverDirection.IsNearlyZero())
	{
		HoverDirection = Hunter->GetActorForwardVector().GetSafeNormal2D();
	}

	ASC->AddLooseGameplayTag(BBGameplayTags::State_Hudson_Hovering);
	const float StartMontageDuration = PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_Shift, EBBAbilityAnimationPhase::Start);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_Shift_Start, Hunter->GetActorLocation(), HoverDirection);
	AddVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_Shift_Loop, Hunter->GetActorLocation(), HoverDirection);
	if (Hunter->HasAuthority())
	{
		const FVector Start = Hunter->GetActorLocation() + FVector(0.0f, 0.0f, 35.0f);
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBeamActor* Trail = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
			ABBPrimitiveBeamActor::StaticClass(), Start, FRotator::ZeroRotator, Params))
		{
			Trail->InitBeam(Start, Start + HoverDirection * HoverSpeed * MaxHoverDuration, 10.0f, MaxHoverDuration,
				FLinearColor(0.31f, 0.64f, 0.78f, 1.0f));
		}
	}
	if (StartMontageDuration > 0.05f && StartMontageDuration < MaxHoverDuration)
	{
		if (UWorld* MontageWorld = GetWorld())
		{
			MontageWorld->GetTimerManager().SetTimer(HoverLoopMontageHandle, this, &UGA_Hudson_Shift_HoverJets::PlayHoverLoopMontage, StartMontageDuration, false);
		}
	}
	else
	{
		PlayHoverLoopMontage();
	}
	Hunter->LaunchCharacter(HoverDirection * BurstSpeed + FVector(0.0f, 0.0f, 180.0f), true, true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HoverTickHandle, this, &UGA_Hudson_Shift_HoverJets::HoverTick, 0.05f, true, 0.0f);
		World->GetTimerManager().SetTimer(HoverEndHandle, [this]()
		{
			if (CachedActorInfo)
			{
				EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
			}
		}, MaxHoverDuration, false);
	}
}

void UGA_Hudson_Shift_HoverJets::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoverTickHandle);
		World->GetTimerManager().ClearTimer(HoverEndHandle);
		World->GetTimerManager().ClearTimer(HoverLoopMontageHandle);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(BBGameplayTags::State_Hudson_Hovering, 0);
	}

	RemoveVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_Shift_Loop);
	StopVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_Shift, EBBAbilityAnimationPhase::Loop);
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_Shift, EBBAbilityAnimationPhase::End);

	if (AHunterCharacter* Hunter = GetHunterCharacter())
	{
		if (UCharacterMovementComponent* MoveComp = Hunter->GetCharacterMovement())
		{
			MoveComp->Velocity.X = 0.0f;
			MoveComp->Velocity.Y = 0.0f;
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Hudson_Shift_HoverJets::PlayHoverLoopMontage()
{
	if (IsActive())
	{
		PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_Shift, EBBAbilityAnimationPhase::Loop);
	}
}

void UGA_Hudson_Shift_HoverJets::OnInputReleased()
{
	if (IsActive() && CachedActorInfo)
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
		return;
	}

	Super::OnInputReleased();
}

void UGA_Hudson_Shift_HoverJets::HoverTick()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		return;
	}

	const FVector Loc = Hunter->GetActorLocation();

	if (UCharacterMovementComponent* MoveComp = Hunter->GetCharacterMovement())
	{
		MoveComp->Velocity.X = HoverDirection.X * HoverSpeed;
		MoveComp->Velocity.Y = HoverDirection.Y * HoverSpeed;
	}

	if (!Hunter->HasAuthority())
	{
		return;
	}

	const ABreachbornePlayerState* SourcePS = GetBBPlayerState();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourcePS || !SourceASC || !DamageEffectClass)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HudsonHoverJets), false, Hunter);
	Hunter->GetWorld()->OverlapMultiByObjectType(Overlaps, Loc, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(HitRadius), Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AHunterCharacter* TargetHunter = Cast<AHunterCharacter>(Overlap.GetActor());
		if (!TargetHunter || TargetHunter == Hunter || HitActors.Contains(TargetHunter))
		{
			continue;
		}

		const ABreachbornePlayerState* TargetPS = TargetHunter->GetPlayerState<ABreachbornePlayerState>();
		UAbilitySystemComponent* TargetASC = TargetHunter->GetAbilitySystemComponent();
		if (!TargetPS || !TargetASC || TargetPS->GetTeamID() == SourcePS->GetTeamID())
		{
			continue;
		}

		HitActors.Add(TargetHunter);
		const FVector KnockDir = (TargetHunter->GetActorLocation() - Hunter->GetActorLocation()).GetSafeNormal2D();
		TargetHunter->LaunchCharacter(KnockDir * KnockbackForce + FVector(0.0f, 0.0f, 260.0f), true, true);
		PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_Shift, EBBAbilityAnimationPhase::Impact);
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_Shift_Impact, TargetHunter->GetActorLocation(), KnockDir);
		FActorSpawnParameters ImpactParams;
		ImpactParams.Owner = Hunter;
		ImpactParams.Instigator = Hunter;
		ImpactParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBurstActor* Impact = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
			ABBPrimitiveBurstActor::StaticClass(), TargetHunter->GetActorLocation(), FRotator::ZeroRotator, ImpactParams))
		{
			Impact->InitBurst(TargetHunter->GetActorLocation(), HitRadius, 0.24f,
				FLinearColor(1.0f, 0.54f, 0.16f, 1.0f));
		}

		FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, HitDamage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
		}

		ReduceOwnCooldown(CooldownDuration * CooldownRefundFractionPerHit);
	}
}

const FGameplayTagContainer* UGA_Hudson_Shift_HoverJets::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Hudson_Shift_HoverJets::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}

void UGA_Hudson_Shift_HoverJets::ReduceOwnCooldown(float Seconds)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || Seconds <= 0.0f)
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
		const float NewRemaining = FMath::Max(0.0f, Remaining - Seconds);
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
