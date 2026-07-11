#include "GA_Ghost_Q.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBTargetDummy.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Breachborne.h"
#include "DrawDebugHelpers.h"

UGA_Ghost_Q::UGA_Ghost_Q()
{
	AbilityInputTag = BBGameplayTags::InputTag_Q;
	bActivateOnInputHeld = false;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Ghost_Q);
	SetAssetTags(AssetTags);

	// Build cooldown tag container
	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Ghost_Q);

	DamageEffectClass = UBBDamageEffect::StaticClass();
}

void UGA_Ghost_Q::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogBreachborne, Log, TEXT("Ghost Q: ActivateAbility called"));

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Ghost Q: CommitAbility FAILED (on cooldown?)"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Ghost Q: GetHunterCharacter() returned nullptr"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const bool bIsServer = Hunter->HasAuthority();

	const FVector StartLocation = Hunter->GetActorLocation();
	const FVector AimDir = GetAimDirection();
	const FVector EndLocation = StartLocation + (AimDir * MaxRange);
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Ghost_Q, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Ghost_Q_Fire, StartLocation, AimDir);

	// Sphere sweep multi-trace against Pawn object type — pierces all targets in line
	static constexpr float SweepRadius = 25.0f;
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Hunter);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	Hunter->GetWorld()->SweepMultiByObjectType(
		HitResults,
		StartLocation,
		EndLocation,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(SweepRadius),
		QueryParams
	);

	// Draw debug trace line — cyan for Q, visible for 2s
	if (Hunter->IsLocallyControlled())
	{
		const FColor QColor = HitResults.Num() > 0 ? FColor::Cyan : FColor::Blue;
		DrawDebugLine(Hunter->GetWorld(), StartLocation, EndLocation, QColor, false, 2.0f, 0, 4.0f);
		for (const FHitResult& Hit : HitResults)
		{
			DrawDebugSphere(Hunter->GetWorld(), Hit.ImpactPoint, 15.0f, 8, FColor::Cyan, false, 2.0f);
		}
	}

	UE_LOG(LogBreachborne, Log, TEXT("Ghost Q: %s | Trace from %s dir %s | Hits: %d"),
		bIsServer ? TEXT("SERVER") : TEXT("CLIENT"),
		*StartLocation.ToString(), *AimDir.ToString(), HitResults.Num());

	if (bIsServer && HitResults.Num() > 0)
	{
		const ABreachbornePlayerState* ShooterPS = GetBBPlayerState();
		UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

		if (ShooterPS && SourceASC)
		{
			// Track already-damaged actors to avoid double-hits from multi-component actors
			TSet<AActor*> DamagedActors;

			for (const FHitResult& Hit : HitResults)
			{
				AActor* HitActor = Hit.GetActor();
				if (!HitActor || DamagedActors.Contains(HitActor))
				{
					continue;
				}

				// Resolve target ASC and team — supports both HunterCharacter and TargetDummy
				UAbilitySystemComponent* TargetASC = nullptr;
				int32 TargetTeamID = ShooterPS->GetTeamID(); // default = same team → skip

				if (AHunterCharacter* HitHunter = Cast<AHunterCharacter>(HitActor))
				{
					if (const ABreachbornePlayerState* TargetPS = HitHunter->GetPlayerState<ABreachbornePlayerState>())
					{
						TargetTeamID = TargetPS->GetTeamID();
					}
					TargetASC = HitHunter->GetAbilitySystemComponent();
				}
				else if (ABBTargetDummy* Dummy = Cast<ABBTargetDummy>(HitActor))
				{
					TargetTeamID = Dummy->GetTeamID();
					TargetASC = Dummy->GetAbilitySystemComponent();
				}

				if (TargetTeamID == ShooterPS->GetTeamID() || !TargetASC || !DamageEffectClass)
				{
					continue;
				}

				FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1, SourceASC->MakeEffectContext());
				if (SpecHandle.IsValid())
				{
					SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, BaseDamage);
					SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
				}

				DamagedActors.Add(HitActor);
				ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Ghost_Q_Impact, Hit.ImpactPoint, Hit.ImpactNormal);

				UE_LOG(LogBreachborne, Log, TEXT("Ghost Q: Hit %s (Team %d) for %.0f base damage"), *HitActor->GetName(), TargetTeamID, BaseDamage);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Ghost_Q::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Ghost_Q::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = NewObject<UGameplayEffect>(GetTransientPackage());
	CooldownGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	CooldownGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(CooldownDuration));
	UTargetTagsGameplayEffectComponent& TagComp = CooldownGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::Cooldown_Hunter_Ghost_Q);
	TagComp.SetAndApplyTargetTagChanges(TagContainer);

	ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CooldownGE, GetAbilityLevel());
}
