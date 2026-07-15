#include "GA_Kingpin_R.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBAntiHealEffect.h"
#include "Breachborne/Combat/BBDamageExecution.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Combat/BBPrimitiveWedgeActor.h"
#include "Breachborne/Breachborne.h"

namespace
{
	constexpr int32 KingpinUltimateShellCount = 2;
}

UGA_Kingpin_R::UGA_Kingpin_R()
{
	AbilityInputTag = BBGameplayTags::InputTag_R;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, ShotgunRange);
	bActivateOnInputHeld = false;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Kingpin_R);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Kingpin_R);
}

void UGA_Kingpin_R::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
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

	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;
	ShellsFired = 0;
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_R, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_R_Start, Hunter->GetActorLocation(), Hunter->GetActorForwardVector());

	FireShell();

	if (ShellDelay <= 0.0f)
	{
		FireShell();
	}
	else if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ShellTimerHandle, this, &UGA_Kingpin_R::FireShell, ShellDelay, false);
	}
}

void UGA_Kingpin_R::FireShell()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const ABreachbornePlayerState* KingpinPS = GetBBPlayerState();
	if (Hunter && !Hunter->HasAuthority())
	{
		return;
	}
	if (!Hunter || !SourceASC || !KingpinPS)
	{
		if (CachedActorInfo)
		{
			EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, true);
		}
		return;
	}

	++ShellsFired;

	const FVector Origin = Hunter->GetActorLocation();
	const FVector Forward = Hunter->GetActorForwardVector().GetSafeNormal2D();
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(ShotgunHalfAngle));
	FActorSpawnParameters VisualParams;
	VisualParams.Owner = Hunter;
	VisualParams.Instigator = Hunter;
	VisualParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveWedgeActor* Blast = Hunter->GetWorld()->SpawnActor<ABBPrimitiveWedgeActor>(
		ABBPrimitiveWedgeActor::StaticClass(), Origin, FRotator::ZeroRotator, VisualParams))
	{
		const FLinearColor ShellColor = ShellsFired == 1
			? FLinearColor(0.90f, 0.23f, 0.18f, 1.0f)
			: FLinearColor(1.0f, 0.69f, 0.13f, 1.0f);
		Blast->InitWedge(Origin, Forward, ShotgunRange, ShotgunHalfAngle, 10.0f, 0.24f, ShellColor);
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(KingpinShotgunUlt), false, Hunter);
	Hunter->GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(ShotgunRange), Params);

	int32 HitCount = 0;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AHunterCharacter* HitHunter = Cast<AHunterCharacter>(Overlap.GetActor());
		if (!HitHunter || HitHunter == Hunter)
		{
			continue;
		}

		const ABreachbornePlayerState* TargetPS = HitHunter->GetPlayerState<ABreachbornePlayerState>();
		if (!TargetPS || TargetPS->GetTeamID() == KingpinPS->GetTeamID())
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = TargetPS->GetAbilitySystemComponent();
		if (!TargetASC || TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
		{
			continue;
		}

		const FVector ToTarget = (HitHunter->GetActorLocation() - Origin).GetSafeNormal2D();
		if (FVector::DotProduct(Forward, ToTarget) < CosHalfAngle)
		{
			continue;
		}

		ApplyShellHit(SourceASC, TargetASC, Hunter);
		++HitCount;
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_R_Impact, HitHunter->GetActorLocation(), ToTarget);
		if (ABBPrimitiveBurstActor* Impact = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
			ABBPrimitiveBurstActor::StaticClass(), HitHunter->GetActorLocation(), FRotator::ZeroRotator, VisualParams))
		{
			Impact->InitBurst(HitHunter->GetActorLocation(), 125.0f, 0.25f,
				FLinearColor(1.0f, 0.69f, 0.13f, 1.0f));
		}

		UE_LOG(LogBreachborne, Log, TEXT("Kingpin R: Shell %d hit %s for %.0f damage + anti-heal %.1fs"),
			ShellsFired, *HitHunter->GetName(), DamagePerShell, AntiHealDuration);
	}

	UE_LOG(LogBreachborne, Log, TEXT("Kingpin R: Shell %d hit %d target(s)"), ShellsFired, HitCount);

	if (ShellsFired >= KingpinUltimateShellCount && CachedActorInfo)
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
	}
}

void UGA_Kingpin_R::ApplyShellHit(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, AActor* InstigatorActor)
{
	if (!SourceASC || !TargetASC || !InstigatorActor)
	{
		return;
	}

	FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
	Ctx.AddInstigator(InstigatorActor, InstigatorActor);

	UGameplayEffect* DmgGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_Kingpin_R_Shotgun_Damage"));
	DmgGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UBBDamageExecution::StaticClass();
	DmgGE->Executions.Add(ExecDef);

	FGameplayEffectSpec DmgSpec(DmgGE, Ctx, 1.0f);
	DmgSpec.SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, DamagePerShell);
	TargetASC->ApplyGameplayEffectSpecToSelf(DmgSpec);

	FGameplayTagContainer AntiHealGrantedTags;
	AntiHealGrantedTags.AddTag(BBGameplayTags::State_AntiHeal);
	TargetASC->RemoveActiveEffectsWithGrantedTags(AntiHealGrantedTags);

	FGameplayEffectSpecHandle AntiHealSpec = SourceASC->MakeOutgoingSpec(UBBAntiHealEffect::StaticClass(), 1.0f, Ctx);
	if (AntiHealSpec.IsValid())
	{
		AntiHealSpec.Data->SetDuration(AntiHealDuration, false);
		SourceASC->ApplyGameplayEffectSpecToTarget(*AntiHealSpec.Data, TargetASC);
	}
}

void UGA_Kingpin_R::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShellTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

const FGameplayTagContainer* UGA_Kingpin_R::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Kingpin_R::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
