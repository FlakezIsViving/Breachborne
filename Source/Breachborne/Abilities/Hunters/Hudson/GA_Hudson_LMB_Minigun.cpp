#include "GA_Hudson_LMB_Minigun.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBGlideBot.h"
#include "Breachborne/Combat/BBHealEffect.h"
#include "Breachborne/Combat/BBTargetDummy.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

UGA_Hudson_LMB_Minigun::UGA_Hudson_LMB_Minigun()
{
	AbilityInputTag = BBGameplayTags::InputTag_LMB;
	bActivateOnInputHeld = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Hudson_LMB);
	SetAssetTags(AssetTags);

	DamageEffectClass = UBBDamageEffect::StaticClass();
	HealEffectClass = UBBHealEffect::StaticClass();
}

void UGA_Hudson_LMB_Minigun::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(BBGameplayTags::State_Hudson_Firing);
	}

	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_LMB, EBBAbilityAnimationPhase::Loop);
	FireStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	FireShot();
	ScheduleNextShot();
}

void UGA_Hudson_LMB_Minigun::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(BBGameplayTags::State_Hudson_Firing, 0);
		if (UBBAbilitySystemComponent* BBASC = Cast<UBBAbilitySystemComponent>(ASC))
		{
			BBASC->CancelAbilityByInputTag(BBGameplayTags::InputTag_RMB);
		}
	}
	StopVisualMontage(BBGameplayTags::Ability_Hunter_Hudson_LMB, EBBAbilityAnimationPhase::Loop);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Hudson_LMB_Minigun::ScheduleNextShot()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const bool bSpunUp = ASC && ASC->HasMatchingGameplayTag(BBGameplayTags::State_Hudson_SpunUp);
	World->GetTimerManager().SetTimer(FireTimerHandle, this, &UGA_Hudson_LMB_Minigun::FireShot,
		bSpunUp ? SpunUpFireInterval : NormalFireInterval, false);
}

void UGA_Hudson_LMB_Minigun::FireShot()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const ABreachbornePlayerState* SourcePS = GetBBPlayerState();
	if (!Hunter || !SourceASC || !SourcePS)
	{
		return;
	}

	const bool bSpunUp = SourceASC->HasMatchingGameplayTag(BBGameplayTags::State_Hudson_SpunUp);
	const float Range = bSpunUp ? SpunUpRange : NormalRange;
	const float HeldSeconds = GetWorld() ? FMath::Max(0.0f, GetWorld()->GetTimeSeconds() - FireStartTime) : 0.0f;
	const float AccuracyAlpha = FMath::Clamp(HeldSeconds / FMath::Max(0.01f, AccuracyRampSeconds), 0.0f, 1.0f);
	const float SpreadDegrees = FMath::Lerp(InitialSpreadDegrees, MinimumSpreadDegrees, AccuracyAlpha);

	FVector AimDir = GetAimDirection();
	if (Hunter->HasAuthority())
	{
		const float SpreadYaw = FMath::FRandRange(-SpreadDegrees, SpreadDegrees);
		AimDir = AimDir.RotateAngleAxis(SpreadYaw, FVector::UpVector).GetSafeNormal2D();
	}

	const FVector Start = Hunter->GetActorLocation() + AimDir * 70.0f + FVector(0.0f, 0.0f, 45.0f);
	const FVector End = Start + AimDir * Range;
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_LMB_Fire, Start, AimDir);

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HudsonMinigun), false, Hunter);
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);
	Hunter->GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjParams, FCollisionShape::MakeSphere(22.0f), Params);

	const FColor TraceColor = bSpunUp ? FColor::Red : FColor::Yellow;
	Hunter->Multicast_DrawDebugLine(Start, End, TraceColor, 0.12f, bSpunUp ? 5.0f : 2.5f);

	if (Hunter->HasAuthority())
	{
		TSet<AActor*> DamagedActors;
		int32 ValidHits = 0;
		for (const FHitResult& Hit : Hits)
		{
			AActor* HitActor = Hit.GetActor();
			if (!HitActor || DamagedActors.Contains(HitActor))
			{
				continue;
			}

			UAbilitySystemComponent* TargetASC = nullptr;
			int32 TargetTeamID = SourcePS->GetTeamID();
			if (!ResolveTarget(HitActor, TargetASC, TargetTeamID) || !TargetASC || TargetTeamID == SourcePS->GetTeamID())
			{
				continue;
			}

			ApplyDamage(SourceASC, TargetASC, BaseDamage);
			DamagedActors.Add(HitActor);
			++ValidHits;
			ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_LMB_Impact, Hit.ImpactPoint, Hit.ImpactNormal);
			Hunter->Multicast_DrawDebugSphere(Hit.ImpactPoint, 18.0f, TraceColor, 0.18f);

			if (!bSpunUp)
			{
				break;
			}
		}

		if (bSpunUp && ValidHits > 0)
		{
			if (const UBBHealthSet* HealthSet = GetBBPlayerState() ? GetBBPlayerState()->GetHealthSet() : nullptr)
			{
				ApplySelfHeal(SourceASC, HealthSet->GetMaxHealth() * SpunUpHealMaxHealthFraction * ValidHits);
				ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Hudson_LMB_Heal, Hunter->GetActorLocation() + FVector(0.0f, 0.0f, 115.0f));
				Hunter->Multicast_DrawDebugSphere(Hunter->GetActorLocation() + FVector(0.0f, 0.0f, 115.0f), 34.0f, FColor::Green, 0.22f);
			}
		}
	}

	if (IsActive())
	{
		ScheduleNextShot();
	}
}

bool UGA_Hudson_LMB_Minigun::ResolveTarget(AActor* Actor, UAbilitySystemComponent*& OutASC, int32& OutTeamID) const
{
	if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(Actor))
	{
		const ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>();
		OutASC = Hunter->GetAbilitySystemComponent();
		OutTeamID = PS ? PS->GetTeamID() : OutTeamID;
		return OutASC != nullptr;
	}
	if (ABBTargetDummy* Dummy = Cast<ABBTargetDummy>(Actor))
	{
		OutASC = Dummy->GetAbilitySystemComponent();
		OutTeamID = Dummy->GetTeamID();
		return OutASC != nullptr;
	}
	if (ABBGlideBot* GlideBot = Cast<ABBGlideBot>(Actor))
	{
		OutASC = GlideBot->GetAbilitySystemComponent();
		OutTeamID = GlideBot->GetTeamID();
		return OutASC != nullptr;
	}
	return false;
}

void UGA_Hudson_LMB_Minigun::ApplyDamage(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float Damage)
{
	if (!SourceASC || !TargetASC || !DamageEffectClass)
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, Damage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data, TargetASC);
	}
}

void UGA_Hudson_LMB_Minigun::ApplySelfHeal(UAbilitySystemComponent* SourceASC, float HealAmount)
{
	if (!SourceASC || !HealEffectClass || HealAmount <= 0.0f)
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(HealEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_HealAmount, HealAmount);
		SourceASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
	}
}
