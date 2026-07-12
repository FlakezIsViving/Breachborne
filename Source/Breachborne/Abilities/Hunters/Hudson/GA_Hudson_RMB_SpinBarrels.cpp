#include "GA_Hudson_RMB_SpinBarrels.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBPrimitiveWedgeActor.h"

UGA_Hudson_RMB_SpinBarrels::UGA_Hudson_RMB_SpinBarrels()
{
	AbilityInputTag = BBGameplayTags::InputTag_RMB;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, 1300.0f);
	bActivateOnInputHeld = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Hudson_RMB);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Hudson_RMB);
}

void UGA_Hudson_RMB_SpinBarrels::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
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

	ASC->AddLooseGameplayTag(BBGameplayTags::State_Hudson_Spinning);
	const float StartMontageDuration = PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_RMB, EBBAbilityAnimationPhase::Start);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_RMB_Start, Hunter->GetActorLocation());
	AddVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_RMB_Loop, Hunter->GetActorLocation());
	if (StartMontageDuration > 0.05f && StartMontageDuration < SpinUpSeconds)
	{
		if (UWorld* MontageWorld = GetWorld())
		{
			MontageWorld->GetTimerManager().SetTimer(SpinLoopMontageHandle, this, &UGA_Hudson_RMB_SpinBarrels::PlaySpinLoopMontage, StartMontageDuration, false);
		}
	}
	else
	{
		PlaySpinLoopMontage();
	}
	SpinStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bReachedSpunUp = false;
	LastVisualStep = 0;
	bShouldApplyWindDownCooldown = true;
	if (Hunter->HasAuthority())
	{
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveWedgeActor* Ring = Hunter->GetWorld()->SpawnActor<ABBPrimitiveWedgeActor>(
			ABBPrimitiveWedgeActor::StaticClass(), Hunter->GetActorLocation(), FRotator::ZeroRotator, Params))
		{
			Ring->AttachToActor(Hunter, FAttachmentTransformRules::KeepWorldTransform);
			Ring->InitWedge(Hunter->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f), FVector::ForwardVector,
				90.0f, 179.0f, 4.0f, 60.0f, FLinearColor(1.0f, 0.54f, 0.16f, 1.0f));
			ActiveSpinVisual = Ring;
		}
	}

	if (UCharacterMovementComponent* MoveComp = Hunter->GetCharacterMovement())
	{
		OriginalYawRate = MoveComp->RotationRate.Yaw;
		MoveComp->RotationRate.Yaw = OriginalYawRate * TurnRateMultiplier;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SpinTickHandle, this, &UGA_Hudson_RMB_SpinBarrels::SpinTick, VisualTickInterval, true, 0.0f);
	}
}

void UGA_Hudson_RMB_SpinBarrels::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpinTickHandle);
		World->GetTimerManager().ClearTimer(SpinLoopMontageHandle);
	}

	if (AHunterCharacter* Hunter = GetHunterCharacter())
	{
		if (UCharacterMovementComponent* MoveComp = Hunter->GetCharacterMovement())
		{
			MoveComp->RotationRate.Yaw = OriginalYawRate;
		}
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(BBGameplayTags::State_Hudson_Spinning, 0);
		ASC->SetLooseGameplayTagCount(BBGameplayTags::State_Hudson_SpunUp, 0);
	}
	if (ActiveSpinVisual.IsValid())
	{
		ActiveSpinVisual->Destroy();
		ActiveSpinVisual.Reset();
	}

	RemoveVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_RMB_Loop);
	StopVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_RMB, EBBAbilityAnimationPhase::Loop);
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_RMB, EBBAbilityAnimationPhase::End);
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_RMB_End, Hunter->GetActorLocation());
	}

	if (bShouldApplyWindDownCooldown)
	{
		ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, WindDownCooldownSeconds);
		bShouldApplyWindDownCooldown = false;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Hudson_RMB_SpinBarrels::PlaySpinLoopMontage()
{
	if (IsActive())
	{
		PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_RMB, EBBAbilityAnimationPhase::Loop);
	}
}

void UGA_Hudson_RMB_SpinBarrels::SpinTick()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Hunter || !ASC || !GetWorld())
	{
		return;
	}

	const float Elapsed = GetWorld()->GetTimeSeconds() - SpinStartTime;
	const float Alpha = FMath::Clamp(Elapsed / FMath::Max(0.01f, SpinUpSeconds), 0.0f, 1.0f);
	if (Alpha >= 1.0f && !bReachedSpunUp)
	{
		bReachedSpunUp = true;
		ASC->AddLooseGameplayTag(BBGameplayTags::State_Hudson_SpunUp);
	}

	if (Hunter->HasAuthority() && ActiveSpinVisual.IsValid())
	{
		const uint8 VisualStep = static_cast<uint8>(FMath::FloorToInt(Alpha * 10.0f));
		if (VisualStep != LastVisualStep)
		{
			LastVisualStep = VisualStep;
			ActiveSpinVisual->UpdateWedge(FMath::Lerp(90.0f, 140.0f, Alpha),
				bReachedSpunUp ? FLinearColor(1.0f, 0.25f, 0.08f, 1.0f) : FLinearColor(1.0f, 0.54f, 0.16f, 1.0f));
		}
	}
}

const FGameplayTagContainer* UGA_Hudson_RMB_SpinBarrels::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Hudson_RMB_SpinBarrels::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// RMB starts its wind-down lockout when spin ends, not when the player first presses RMB.
}
