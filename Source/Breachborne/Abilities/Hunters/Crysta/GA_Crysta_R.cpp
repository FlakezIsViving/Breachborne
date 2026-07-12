#include "GA_Crysta_R.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBGlideBot.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Combat/BBReverberationMarkEffect.h"
#include "Breachborne/Combat/BBTargetDummy.h"
#include "Breachborne/Combat/BBTestAlly.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Engine/OverlapResult.h"

namespace
{
	bool ResolveCrystaRTarget(AActor* Actor, int32 SourceTeamID, UAbilitySystemComponent*& OutASC)
	{
		OutASC = nullptr;
		int32 TargetTeamID = SourceTeamID;
		if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(Actor))
		{
			if (const ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>())
			{
				TargetTeamID = PS->GetTeamID();
			}
			OutASC = Hunter->GetAbilitySystemComponent();
		}
		else if (ABBTargetDummy* Dummy = Cast<ABBTargetDummy>(Actor))
		{
			TargetTeamID = Dummy->GetTeamID();
			OutASC = Dummy->GetAbilitySystemComponent();
		}
		else if (ABBGlideBot* GlideBot = Cast<ABBGlideBot>(Actor))
		{
			TargetTeamID = GlideBot->GetTeamID();
			OutASC = GlideBot->GetAbilitySystemComponent();
		}
		else if (ABBTestAlly* TestAlly = Cast<ABBTestAlly>(Actor))
		{
			TargetTeamID = TestAlly->GetTeamID();
			OutASC = TestAlly->GetAbilitySystemComponent();
		}
		return OutASC && TargetTeamID != SourceTeamID && !OutASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead);
	}
}

UGA_Crysta_R::UGA_Crysta_R()
{
	AbilityInputTag = BBGameplayTags::InputTag_R;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::TargetedArea, MaxRange, BeamSweepRadius);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Crysta_R);
	SetAssetTags(AssetTags);
	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Crysta_R);
	DamageEffectClass = UBBDamageEffect::StaticClass();
	MarkEffectClass = UBBReverberationMarkEffect::StaticClass();
	BeamVisualClass = ABBPrimitiveBeamActor::StaticClass();
}

void UGA_Crysta_R::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	const ABreachbornePlayerState* PS = GetBBPlayerState();
	if (!Hunter || !PS)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedAimDirection = GetAimDirection();
	CachedCastLocation = Hunter->GetActorLocation();
	const FVector RawTarget = GetAimLocation();
	const float AimDistance = FVector::Dist2D(CachedCastLocation, RawTarget);
	CachedTargetLocation = CachedCastLocation + CachedAimDirection * FMath::Clamp(AimDistance, 250.0f, MaxRange);
	CachedTargetLocation.Z = CachedCastLocation.Z;
	CachedSourceTeamID = PS->GetTeamID();
	DetonatedTargets.Reset();

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Crysta_R, EBBAbilityAnimationPhase::Start);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Crysta_R_Warning, CachedTargetLocation, CachedAimDirection);

	if (!Hunter->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	SpawnLaneIndicators();

	SalvoTimerHandles.SetNum(5);
	for (int32 Index = 0; Index < 5; ++Index)
	{
		FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &UGA_Crysta_R::FireSalvo, Index);
		Hunter->GetWorldTimerManager().SetTimer(SalvoTimerHandles[Index], Delegate, SalvoInterval * Index, false);
	}
	FTimerHandle FinishHandle;
	FTimerDelegate FinishDelegate = FTimerDelegate::CreateUObject(this, &UGA_Crysta_R::FinishSalvo);
	Hunter->GetWorldTimerManager().SetTimer(FinishHandle, FinishDelegate, SalvoInterval * 5 + 0.05f, false);
	SalvoTimerHandles.Add(FinishHandle);
}

void UGA_Crysta_R::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (const AHunterCharacter* Hunter = GetHunterCharacter())
	{
		for (FTimerHandle& TimerHandle : SalvoTimerHandles)
		{
			Hunter->GetWorldTimerManager().ClearTimer(TimerHandle);
		}
	}
	SalvoTimerHandles.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Crysta_R::FireSalvo(int32 SalvoIndex)
{
	TArray<FVector> Starts;
	TArray<FVector> Ends;
	BuildLaneSegments(Starts, Ends);
	if (Starts.Num() < 2 || Ends.Num() < 2)
	{
		return;
	}

	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Crysta_R_Fire, CachedTargetLocation, CachedAimDirection);
	if (SalvoIndex == 4)
	{
		HitAlongSegment(Starts[0], Ends[0]);
		HitAlongSegment(Starts[1], Ends[1]);
	}
	else
	{
		const int32 LaneIndex = SalvoIndex % 2;
		HitAlongSegment(Starts[LaneIndex], Ends[LaneIndex]);
	}
}

void UGA_Crysta_R::FinishSalvo()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Crysta_R::HitAlongSegment(const FVector& Start, const FVector& End)
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !Hunter->HasAuthority())
	{
		return;
	}

	TArray<FHitResult> Hits;
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CrystaSalvo), false, Hunter);
	Hunter->GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjParams, FCollisionShape::MakeSphere(BeamSweepRadius), Params);
	SpawnBeamVisual(Start, End, BeamSweepRadius, 0.24f, FLinearColor(1.0f, 0.12f, 0.95f, 1.0f));

	TSet<AActor*> HitActors;
	for (const FHitResult& Hit : Hits)
	{
		AActor* Actor = Hit.GetActor();
		if (!Actor || HitActors.Contains(Actor))
		{
			continue;
		}
		HitActors.Add(Actor);

		UAbilitySystemComponent* TargetASC = nullptr;
		if (!ResolveCrystaRTarget(Actor, CachedSourceTeamID, TargetASC))
		{
			continue;
		}

		DetonateOnce(TargetASC, Actor);
		ApplyDamage(TargetASC, ShotDamage);
		ApplyMark(TargetASC);
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Crysta_R_Impact, Actor->GetActorLocation(), CachedAimDirection);
		FActorSpawnParameters BurstParams;
		BurstParams.Owner = Hunter;
		BurstParams.Instigator = Hunter;
		BurstParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBurstActor* Impact = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
			ABBPrimitiveBurstActor::StaticClass(), Actor->GetActorLocation(), FRotator::ZeroRotator, BurstParams))
		{
			Impact->InitBurst(Actor->GetActorLocation(), 72.0f, 0.2f,
				FLinearColor(0.94f, 0.27f, 0.82f, 1.0f));
		}
	}
}

void UGA_Crysta_R::SpawnBeamVisual(const FVector& Start, const FVector& End, float Radius, float LifetimeSeconds, const FLinearColor& Color) const
{
	const AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !Hunter->HasAuthority() || !BeamVisualClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = const_cast<AHunterCharacter*>(Hunter);
	SpawnParams.Instigator = const_cast<AHunterCharacter*>(Hunter);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveBeamActor* Beam = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(BeamVisualClass, FTransform::Identity, SpawnParams))
	{
		Beam->InitBeam(Start, End, Radius, LifetimeSeconds, Color);
	}
}

void UGA_Crysta_R::SpawnLaneIndicators()
{
	TArray<FVector> Starts;
	TArray<FVector> Ends;
	BuildLaneSegments(Starts, Ends);
	if (Starts.Num() != Ends.Num())
	{
		return;
	}

	const float IndicatorLifetime = SalvoInterval * 5.0f + 0.35f;
	for (int32 Index = 0; Index < Starts.Num(); ++Index)
	{
		SpawnBeamVisual(Starts[Index] - FVector(0.0f, 0.0f, 45.0f), Ends[Index] - FVector(0.0f, 0.0f, 45.0f),
			IndicatorBeamRadius, IndicatorLifetime, FLinearColor(0.72f, 0.72f, 0.72f, 1.0f));
	}
}

void UGA_Crysta_R::ApplyDamage(UAbilitySystemComponent* TargetASC, float Damage) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !TargetASC || !DamageEffectClass || Damage <= 0.0f)
	{
		return;
	}
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, Damage);
		ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void UGA_Crysta_R::ApplyMark(UAbilitySystemComponent* TargetASC) const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !TargetASC || !MarkEffectClass)
	{
		return;
	}
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(MarkEffectClass, 1.0f, ASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void UGA_Crysta_R::DetonateOnce(UAbilitySystemComponent* TargetASC, AActor* TargetActor)
{
	if (!TargetASC || !TargetActor || DetonatedTargets.Contains(TargetActor) || !TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Crysta_Reverberation))
	{
		return;
	}
	DetonatedTargets.Add(TargetActor);
	ApplyDamage(TargetASC, DetonationDamage);
	FGameplayTagContainer MarkTags;
	MarkTags.AddTag(BBGameplayTags::State_Crysta_Reverberation);
	TargetASC->RemoveActiveEffectsWithTags(MarkTags);

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayEventData EventData;
		EventData.Instigator = GetHunterCharacter();
		EventData.Target = TargetActor;
		ASC->HandleGameplayEvent(BBGameplayTags::Event_Crysta_ReverberationDetonated, &EventData);
	}
}

void UGA_Crysta_R::BuildLaneSegments(TArray<FVector>& OutStarts, TArray<FVector>& OutEnds) const
{
	OutStarts.Reset();
	OutEnds.Reset();

	const FVector Forward = CachedAimDirection.GetSafeNormal2D();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal2D();
	if (Forward.IsNearlyZero() || Right.IsNearlyZero())
	{
		return;
	}

	const FVector StartBase = CachedCastLocation + Forward * LauncherForwardOffset + FVector(0.0f, 0.0f, 95.0f);
	const FVector ConvergencePoint = CachedTargetLocation + FVector(0.0f, 0.0f, 95.0f);
	const float ConvergenceDistance = FVector::Dist2D(CachedCastLocation, CachedTargetLocation);
	const float ExtensionDistance = FMath::Max(0.0f, MaxRange - ConvergenceDistance);

	auto AddSideLane = [&](float SideSign)
	{
		const FVector Start = StartBase + Right * SideSign * LauncherSideOffset;
		FVector BeamDirection = (ConvergencePoint - Start).GetSafeNormal2D();
		if (BeamDirection.IsNearlyZero())
		{
			BeamDirection = Forward;
		}

		FVector End = ConvergencePoint + BeamDirection * ExtensionDistance;
		End.Z = ConvergencePoint.Z;
		OutStarts.Add(Start);
		OutEnds.Add(End);
	};

	AddSideLane(-1.0f);
	AddSideLane(1.0f);
}

const FGameplayTagContainer* UGA_Crysta_R::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Crysta_R::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
