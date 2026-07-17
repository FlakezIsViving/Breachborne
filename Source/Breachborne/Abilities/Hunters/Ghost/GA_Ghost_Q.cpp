#include "GA_Ghost_Q.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBTargetDummy.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Breachborne.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	FVector ResolveGhostQBeamStart(const AHunterCharacter* Hunter, const FVector& AimDirection)
	{
		if (Hunter)
		{
			if (const USkeletalMeshComponent* Mesh = Hunter->GetMesh())
			{
				static const FName PreferredMuzzleSocket(TEXT("Muzzle_Front"));
				static const FName FallbackMuzzleSocket(TEXT("Muzzle"));
				if (Mesh->DoesSocketExist(PreferredMuzzleSocket))
				{
					return Mesh->GetSocketLocation(PreferredMuzzleSocket);
				}
				if (Mesh->DoesSocketExist(FallbackMuzzleSocket))
				{
					return Mesh->GetSocketLocation(FallbackMuzzleSocket);
				}
			}

			return Hunter->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f) + AimDirection * 55.0f;
		}

		return FVector::ZeroVector;
	}
}

UGA_Ghost_Q::UGA_Ghost_Q()
{
	AbilityInputTag = BBGameplayTags::InputTag_Q;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Directional, MaxRange);
	bActivateOnInputHeld = false;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Ghost_Q);
	SetAssetTags(AssetTags);

	// Build cooldown tag container
	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Ghost_Q);

	DamageEffectClass = UBBDamageEffect::StaticClass();
	BeamVisualClass = ABBPrimitiveBeamActor::StaticClass();
	BurstVisualClass = ABBPrimitiveBurstActor::StaticClass();
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

	PendingAimDirection = GetAimDirection();
	PendingStartLocation = ResolveGhostQBeamStart(Hunter, PendingAimDirection);
	PendingEndLocation = PendingStartLocation + (PendingAimDirection * MaxRange);
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Ghost_Q, EBBAbilityAnimationPhase::Start);
	ExecuteBeamVisualCue(BBGameplayTags::GameplayCue_Hunter_Ghost_Q_Telegraph,
		PendingStartLocation, PendingEndLocation);
	if (bIsServer && BeamVisualClass)
	{
		FActorSpawnParameters VisualParams;
		VisualParams.Owner = Hunter;
		VisualParams.Instigator = Hunter;
		VisualParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBeamActor* Beam = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
			BeamVisualClass, FTransform::Identity, VisualParams))
		{
			Beam->InitBeam(PendingStartLocation, PendingEndLocation, 7.0f, FireDelay,
				FLinearColor(0.55f, 0.58f, 0.60f, 1.0f));
		}
	}

	if (bIsServer)
	{
		Hunter->GetWorldTimerManager().SetTimer(
			FireDelayHandle, this, &UGA_Ghost_Q::FireLaser, FireDelay, false);
	}
}

void UGA_Ghost_Q::FireLaser()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !Hunter->HasAuthority())
	{
		return;
	}

	const bool bIsServer = true;
	const FVector StartLocation = PendingStartLocation;
	const FVector EndLocation = PendingEndLocation;
	const FVector AimDir = PendingAimDirection;
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Ghost_Q, EBBAbilityAnimationPhase::Fire);
	ExecuteBeamVisualCue(BBGameplayTags::GameplayCue_Hunter_Ghost_Q_Fire, StartLocation, EndLocation);
	if (BeamVisualClass)
	{
		FActorSpawnParameters VisualParams;
		VisualParams.Owner = Hunter;
		VisualParams.Instigator = Hunter;
		VisualParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBeamActor* Beam = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
			BeamVisualClass, FTransform::Identity, VisualParams))
		{
			Beam->InitBeam(StartLocation, EndLocation, 25.0f, 0.24f,
				FLinearColor(0.13f, 0.90f, 0.72f, 1.0f));
		}
	}

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
				if (BurstVisualClass)
				{
					FActorSpawnParameters VisualParams;
					VisualParams.Owner = Hunter;
					VisualParams.Instigator = Hunter;
					VisualParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					if (ABBPrimitiveBurstActor* Burst = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
						BurstVisualClass, Hit.ImpactPoint, FRotator::ZeroRotator, VisualParams))
					{
						Burst->InitBurst(Hit.ImpactPoint, 48.0f, 0.22f, FLinearColor(0.90f, 1.0f, 1.0f, 1.0f));
					}
				}

				UE_LOG(LogBreachborne, Log, TEXT("Ghost Q: Hit %s (Team %d) for %.0f base damage"), *HitActor->GetName(), TargetTeamID, BaseDamage);
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Ghost_Q::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().ClearTimer(FireDelayHandle);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

const FGameplayTagContainer* UGA_Ghost_Q::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Ghost_Q::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	ApplyBBCooldown(Handle, ActorInfo, ActivationInfo, CooldownDuration);
}
