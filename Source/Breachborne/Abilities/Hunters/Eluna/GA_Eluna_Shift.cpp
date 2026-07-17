#include "GA_Eluna_Shift.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Wisp/BBWispPawn.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

// ------------------------------------------------------------------
// Ground Dash
// ------------------------------------------------------------------

UGA_Eluna_GroundDash::UGA_Eluna_GroundDash()
{
	AbilityInputTag = BBGameplayTags::InputTag_Shift;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Movement, DashDistance, 55.0f);
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Eluna_GroundDash);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Eluna_GroundDash);
}

bool UGA_Eluna_GroundDash::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const bool bResult = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	return bResult;
}

void UGA_Eluna_GroundDash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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

	UCharacterMovementComponent* CMC = Hunter->GetCharacterMovement();
	if (!CMC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector DashDir = CMC->GetCurrentAcceleration().GetSafeNormal2D();
	if (DashDir.IsNearlyZero())
	{
		DashDir = GetAimDirection();
	}


	FVector LaunchVelocity = DashDir * DashSpeed;
	LaunchVelocity.Z = DashLift;
	const FGameplayTag VisualAbilityTag = IsA(UGA_Eluna_AerialDash::StaticClass())
		? BBGameplayTags::Ability_Hunter_Eluna_AerialDash
		: BBGameplayTags::Ability_Hunter_Eluna_GroundDash;
	PlayVisualMontage(VisualAbilityTag, EBBAbilityAnimationPhase::Start);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Eluna_Shift_Trail, Hunter->GetActorLocation(), DashDir);
	const FVector DashStart = Hunter->GetActorLocation();
	if (Hunter->HasAuthority())
	{
		FActorSpawnParameters TrailParams;
		TrailParams.Owner = Hunter;
		TrailParams.Instigator = Hunter;
		TrailParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBeamActor* Trail = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
			ABBPrimitiveBeamActor::StaticClass(), DashStart, FRotator::ZeroRotator, TrailParams))
		{
			Trail->InitBeam(DashStart, DashStart + DashDir * DashDistance, 9.0f, 0.28f,
				FLinearColor(0.86f, 0.92f, 1.0f, 1.0f));
		}
	}
	Hunter->LaunchCharacter(LaunchVelocity, true, true);
	if (Hunter->HasAuthority() && TryCollectWispAlongPath(DashStart, DashStart + DashDir * DashDistance))
	{
		FActorSpawnParameters PulseParams;
		PulseParams.Owner = Hunter;
		PulseParams.Instigator = Hunter;
		PulseParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBurstActor* Pulse = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
			ABBPrimitiveBurstActor::StaticClass(), Hunter->GetActorLocation(), FRotator::ZeroRotator, PulseParams))
		{
			Pulse->InitBurst(Hunter->GetActorLocation(), WispDetectionRadius * 1.5f, 0.22f,
				FLinearColor(0.30f, 0.79f, 0.94f, 1.0f));
		}
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UGA_Eluna_GroundDash::TryCollectWispAlongPath(const FVector& DashStart, const FVector& DashEnd)
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		return false;
	}

	ABreachbornePlayerState* SourcePS = GetBBPlayerState();
	if (!SourcePS)
	{
		return false;
	}

	const int32 SourceTeam = SourcePS->GetTeamID();
	FVector ClippedDashEnd = DashEnd;
	FHitResult WorldHit;
	FCollisionQueryParams WorldParams(SCENE_QUERY_STAT(ElunaDashWorldClip), false, Hunter);
	FCollisionObjectQueryParams WorldObjects;
	WorldObjects.AddObjectTypesToQuery(ECC_WorldStatic);
	if (Hunter->GetWorld()->LineTraceSingleByObjectType(
		WorldHit, DashStart, DashEnd, WorldObjects, WorldParams))
	{
		ClippedDashEnd = WorldHit.Location;
	}

	ABBWispPawn* CollectedWisp = nullptr;
	float ClosestDistanceAlongDash = TNumericLimits<float>::Max();
	const FVector DashDelta = ClippedDashEnd - DashStart;
	const float DashLengthSquared = DashDelta.SizeSquared();
	for (TActorIterator<ABBWispPawn> It(Hunter->GetWorld()); It; ++It)
	{
		ABBWispPawn* Wisp = *It;
		const ABreachbornePlayerState* WispPS = Wisp ? Wisp->GetOwningPlayerState() : nullptr;
		if (!Wisp || Wisp->GetCarrier() || !WispPS || WispPS->GetTeamID() != SourceTeam
			|| Wisp->GetWispState() == EWispState::Revived
			|| Wisp->GetWispState() == EWispState::BeingExecuted)
		{
			continue;
		}

		const FVector WispLocation = Wisp->GetActorLocation();
		if (FMath::PointDistToSegment(WispLocation, DashStart, ClippedDashEnd) > WispDetectionRadius)
		{
			continue;
		}

		const float DistanceAlongDash = DashLengthSquared > KINDA_SMALL_NUMBER
			? FVector::DotProduct(WispLocation - DashStart, DashDelta) / DashLengthSquared
			: 0.0f;
		if (DistanceAlongDash < ClosestDistanceAlongDash)
		{
			ClosestDistanceAlongDash = DistanceAlongDash;
			CollectedWisp = Wisp;
		}
	}
	if (!CollectedWisp)
	{
		return false;
	}

	CollectedWisp->SetCarrier(Hunter);

	// CommitAbility has already applied the cooldown. Remove this dash's
	// specific cooldown effect on the server and its predicted owner copy.
	int32 RemovedCooldownCount = 0;
	UBBAbilitySystemComponent* ASC = GetBBAbilitySystemComponent();
	if (ASC)
	{
		RemovedCooldownCount = ASC->RefundCooldownsByTags(*GetCooldownTags());
	}
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_ELUNA_SHIFT|WISP_COLLECTED|ability=%s|wisp=%s|cooldown_effects_removed=%d|cooldown_refund=full"),
		*GetClass()->GetName(), *CollectedWisp->GetName(), RemovedCooldownCount);

	return true;
}

const FGameplayTagContainer* UGA_Eluna_GroundDash::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Eluna_GroundDash::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	FGameplayTagContainer GrantedTags = *GetCooldownTags();
	GrantedTags.AddTag(BBGameplayTags::Cooldown_Dash);
	ApplyBBCooldownWithTags(Handle, ActorInfo, ActivationInfo, CooldownDuration, GrantedTags);
}

// ------------------------------------------------------------------
// Aerial Dash
// ------------------------------------------------------------------

UGA_Eluna_AerialDash::UGA_Eluna_AerialDash()
{
	AbilityInputTag = BBGameplayTags::InputTag_Shift;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Eluna_AerialDash);
	SetAssetTags(AssetTags);

	// Aerial dash goes farther
	DashDistance = 900.0f;
	ConfigureRangeIndicator(EBBRangeIndicatorMode::Movement, DashDistance, 55.0f);
	CooldownDuration = 6.0f;
}

bool UGA_Eluna_AerialDash::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const bool bSuperOk = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	bool bAirborne = false;
	if (const AHunterCharacter* Hunter = Cast<AHunterCharacter>(ActorInfo->AvatarActor.Get()))
	{
		if (UCharacterMovementComponent* CMC = Hunter->GetCharacterMovement())
		{
			bAirborne = !CMC->IsMovingOnGround();
		}
	}
	return bSuperOk && bAirborne;
}

const FGameplayTagContainer* UGA_Eluna_AerialDash::GetCooldownTags() const
{
	static FGameplayTagContainer AerialCooldownTags;
	AerialCooldownTags.AddTag(BBGameplayTags::Cooldown_Hunter_Eluna_AerialDash);
	return &AerialCooldownTags;
}
