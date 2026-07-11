#include "GA_Kingpin_Q.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBDamageExecution.h"
#include "Breachborne/Combat/BBStunTagEffect.h"
#include "Breachborne/Breachborne.h"

UGA_Kingpin_Q::UGA_Kingpin_Q()
{
	AbilityInputTag    = BBGameplayTags::InputTag_Q;
	bActivateOnInputHeld = false;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Kingpin_Q);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Kingpin_Q);
}

void UGA_Kingpin_Q::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
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
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	const FVector Origin  = Hunter->GetActorLocation();
	const FVector Forward = Hunter->GetActorForwardVector().GetSafeNormal2D();
	const float   CosHalf = FMath::Cos(FMath::DegreesToRadians(SlamHalfAngle));
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_Q, EBBAbilityAnimationPhase::Start);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_Q_Telegraph, Origin, Forward);

	// --- Hit-area telegraph: yellow wireframe cone on the ground ---
	// Drawn BEFORE the authority gate so the owning client also sees it (Q is
	// LocalPredicted — both server and owning client run ActivateAbility, but the
	// dedicated server has no viewport so a server-only draw is invisible).
	// Built from line primitives (UE's DrawDebugCone is a 3D cone — for a flat top-down
	// hit zone we want a sector outline). Two edge rays from Origin to ±SlamHalfAngle at
	// SlamRange, then ArcSegments short lines stitching the outer arc. 0.5s lifetime.
	if (UWorld* World = Hunter->GetWorld())
	{
		const float IndicatorLifetime = 0.5f;
		const FColor IndicatorColor = FColor::Yellow;
		const float IndicatorThickness = 3.0f;
		constexpr int32 ArcSegments = 12;

		const float HalfAngleRad = FMath::DegreesToRadians(SlamHalfAngle);
		const float YawRad       = FMath::Atan2(Forward.Y, Forward.X);

		auto PointAt = [&](float AngleOffsetRad)
		{
			const float Theta = YawRad + AngleOffsetRad;
			return Origin + FVector(FMath::Cos(Theta) * SlamRange,
									FMath::Sin(Theta) * SlamRange,
									0.0f);
		};

		const FVector LeftEdge  = PointAt(-HalfAngleRad);
		const FVector RightEdge = PointAt(+HalfAngleRad);

		// Two side rays from Kingpin out to the arc
		DrawDebugLine(World, Origin, LeftEdge,  IndicatorColor, false, IndicatorLifetime, 0, IndicatorThickness);
		DrawDebugLine(World, Origin, RightEdge, IndicatorColor, false, IndicatorLifetime, 0, IndicatorThickness);

		// Stitched arc on the outer edge
		FVector PrevArcPoint = LeftEdge;
		for (int32 i = 1; i <= ArcSegments; ++i)
		{
			const float T = static_cast<float>(i) / ArcSegments;
			const float AngleOffsetRad = FMath::Lerp(-HalfAngleRad, +HalfAngleRad, T);
			const FVector ArcPoint = PointAt(AngleOffsetRad);
			DrawDebugLine(World, PrevArcPoint, ArcPoint, IndicatorColor, false, IndicatorLifetime, 0, IndicatorThickness);
			PrevArcPoint = ArcPoint;
		}
	}

	// --- Authority gate: only server runs the hit/damage/stun logic from here on. ---
	if (!Hunter->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	const ABreachbornePlayerState* KingpinPS = GetBBPlayerState();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

	// Sphere overlap then filter by cone angle
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(KingpinSlam), false, Hunter);
	Hunter->GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(SlamRange), Params);

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
		if (TargetPS->GetAbilitySystemComponent()->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
		{
			continue;
		}

		// Cone check
		const FVector ToTarget = (HitHunter->GetActorLocation() - Origin).GetSafeNormal2D();
		if (FVector::DotProduct(Forward, ToTarget) < CosHalf)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = HitHunter->GetAbilitySystemComponent();
		if (!TargetASC)
		{
			continue;
		}

		// Damage
		UGameplayEffect* DmgGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_Kingpin_Q_Damage"));
		DmgGE->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayEffectExecutionDefinition ExecDef;
		ExecDef.CalculationClass = UBBDamageExecution::StaticClass();
		DmgGE->Executions.Add(ExecDef);

		FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
		Ctx.AddInstigator(Hunter, Hunter);

		FGameplayEffectSpec DmgSpec(DmgGE, Ctx, 1.0f);
		DmgSpec.SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, BaseDamage);
		TargetASC->ApplyGameplayEffectSpecToSelf(DmgSpec);

		// Stun GE — UBBStunTagEffect is a real C++ class with a CDO so cross-client
		// replication can resolve it via the net GUID cache. Duration overridden per-spec.
		FGameplayEffectSpecHandle StunSpec = SourceASC->MakeOutgoingSpec(
			UBBStunTagEffect::StaticClass(), /*Level=*/1, Ctx);
		if (StunSpec.IsValid())
		{
			StunSpec.Data->SetDuration(StunDuration, /*bLockDuration=*/false);
			SourceASC->ApplyGameplayEffectSpecToTarget(*StunSpec.Data, TargetASC);
		}

		++HitCount;
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Kingpin_Q_Impact, HitHunter->GetActorLocation(), ToTarget);
		UE_LOG(LogBreachborne, Log, TEXT("Kingpin Q: Stunned %s for %.1fs"), *HitHunter->GetName(), StunDuration);

		// Notify passive that CC was applied (Iron Will cooldown reduction)
		FGameplayEventData CCEvent;
		CCEvent.Instigator = Hunter;
		CCEvent.Target     = HitHunter;
		SourceASC->HandleGameplayEvent(BBGameplayTags::Event_CCApplied, &CCEvent);
	}

	UE_LOG(LogBreachborne, Log, TEXT("Kingpin Q: Headcracker hit %d target(s)"), HitCount);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Kingpin_Q::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Kingpin_Q::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
