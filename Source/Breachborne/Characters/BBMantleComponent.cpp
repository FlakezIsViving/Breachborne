#include "BBMantleComponent.h"
#include "BBCharacterMovementComponent.h"
#include "BBMantleSurfaceComponent.h"
#include "GliderComponent.h"
#include "HunterCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"

UBBMantleComponent::UBBMantleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBBMantleComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner)
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | beginplay failed | owner is not character | owner=%s"),
			*GetNameSafe(GetOwner()));
		return;
	}

	BBMovement = Cast<UBBCharacterMovementComponent>(Owner->GetCharacterMovement());
	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | beginplay | owner=%s hasMovement=%d authority=%d"),
		*GetNameSafe(Owner), BBMovement != nullptr, Owner->HasAuthority());

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		ASC = ASI->GetAbilitySystemComponent();
	}

	if (UCapsuleComponent* Capsule = Owner->GetCapsuleComponent())
	{
		Capsule->OnComponentHit.AddDynamic(this, &UBBMantleComponent::OnCapsuleHit);
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | beginplay bound capsule hit | owner=%s capsule=%s"),
			*GetNameSafe(Owner), *GetNameSafe(Capsule));
	}
}

bool UBBMantleComponent::IsMantling() const
{
	return BBMovement && BBMovement->IsMantling();
}

bool UBBMantleComponent::HasRecentMantleCandidate() const
{
	const UWorld* World = GetWorld();
	return World && (World->GetTimeSeconds() - LastFailedAttemptTime) <= FailedAttemptCooldown;
}

void UBBMantleComponent::OnCapsuleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("mantle debug | hit ignored non-authority | owner=%s hitActor=%s comp=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(Hit.GetActor()),
			*GetNameSafe(Hit.GetComponent()));
		return;
	}

	if (GFrameCounter == LastMantleTriggerFrame)
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("mantle debug | hit ignored same frame | owner=%s frame=%u hitActor=%s"),
			*GetNameSafe(GetOwner()), GFrameCounter, *GetNameSafe(Hit.GetActor()));
		return;
	}

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | hit | owner=%s hitActor=%s hitComp=%s profile=%s actorHasTag=%d compHasTag=%d impact=%s normal=%s velocity=%s mode=%d custom=%d falling=%d gliding=%d spiked=%d"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(Hit.GetActor()),
		*GetNameSafe(Hit.GetComponent()),
		Hit.GetComponent() ? *Hit.GetComponent()->GetCollisionProfileName().ToString() : TEXT("none"),
		Hit.GetActor() ? Hit.GetActor()->ActorHasTag(RecoverableActorTag) : 0,
		Hit.GetComponent() ? Hit.GetComponent()->ComponentHasTag(RecoverableActorTag) : 0,
		*Hit.ImpactPoint.ToCompactString(),
		*Hit.ImpactNormal.ToCompactString(),
		BBMovement ? *BBMovement->Velocity.ToCompactString() : TEXT("no-movement"),
		BBMovement ? static_cast<int32>(BBMovement->MovementMode) : -1,
		BBMovement ? static_cast<int32>(BBMovement->CustomMovementMode) : -1,
		BBMovement ? BBMovement->IsFalling() : 0,
		BBMovement ? BBMovement->IsGliding() : 0,
		BBMovement ? BBMovement->IsSpiked() : 0);

	if (ValidateAndTriggerMantle(Hit))
	{
		LastMantleTriggerFrame = GFrameCounter;
	}
}

bool UBBMantleComponent::ValidateAndTriggerMantle(const FHitResult& Hit)
{
	FName RejectReason(NAME_None);

	if (!IsMovementStateEligible())
	{
		RejectReason = TEXT("bad-movement-state");
	}
	else if (HasRecentMantleCandidate())
	{
		RejectReason = TEXT("cooldown");
	}
	else if (!IsRecoverableSurface(Hit))
	{
		RejectReason = TEXT("non-opt-in-surface");
	}
	else
	{
		FBBMantleTarget Target;
		if (BuildMantleTarget(Hit, Target, RejectReason))
		{
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | enter requested | start=%s landing=%s floorZ=%.1f inward=%s outward=%s duration=%.2f"),
				*Target.StartLocation.ToCompactString(),
				*Target.LandingLocation.ToCompactString(),
				Target.LandingFloorZ,
				*Target.InwardDirection.ToCompactString(),
				*Target.OutwardNormal.ToCompactString(),
				Target.Duration);
			SetMantlingTag(true);
			CancelGliderForMantle();
			BBMovement->EnterMantling(Target);
			SetMantlingTag(false);
			return true;
		}
	}

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | reject | reason=%s actor=%s comp=%s falling=%d gliding=%d spiked=%d movementMode=%d customMode=%d velocity=%s"),
		*RejectReason.ToString(),
		Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("?"),
		*GetNameSafe(Hit.GetComponent()),
		BBMovement ? BBMovement->IsFalling() : 0,
		BBMovement ? BBMovement->IsGliding() : 0,
		BBMovement ? BBMovement->IsSpiked() : 0,
		BBMovement ? static_cast<int32>(BBMovement->MovementMode) : -1,
		BBMovement ? static_cast<int32>(BBMovement->CustomMovementMode) : -1,
		BBMovement ? *BBMovement->Velocity.ToCompactString() : TEXT("no-movement"));

	if (RejectReason != TEXT("non-opt-in-surface")
		&& RejectReason != TEXT("bad-movement-state")
		&& RejectReason != TEXT("cooldown"))
	{
		MarkFailedAttempt();
	}
	return false;
}

bool UBBMantleComponent::BuildMantleTarget(const FHitResult& Hit, FBBMantleTarget& OutTarget, FName& OutRejectReason) const
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	UCapsuleComponent* Capsule = Owner ? Owner->GetCapsuleComponent() : nullptr;
	if (!Owner || !World || !BBMovement || !Capsule)
	{
		OutRejectReason = TEXT("missing-owner-world-or-capsule");
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | target failed | reason=%s owner=%s world=%d movement=%d capsule=%d"),
			*OutRejectReason.ToString(), *GetNameSafe(Owner), World != nullptr, BBMovement != nullptr, Capsule != nullptr);
		return false;
	}

	if (Hit.GetActor() && Hit.GetActor()->IsA(APawn::StaticClass()))
	{
		OutRejectReason = TEXT("hit-pawn");
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | target failed | reason=%s actor=%s"),
			*OutRejectReason.ToString(), *GetNameSafe(Hit.GetActor()));
		return false;
	}

	if (Hit.ImpactNormal.Z >= BBMovement->GetWalkableFloorZ())
	{
		OutRejectReason = TEXT("not-wall-or-underside");
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | target failed | reason=%s normal=%s walkableFloorZ=%.2f"),
			*OutRejectReason.ToString(), *Hit.ImpactNormal.ToCompactString(), BBMovement->GetWalkableFloorZ());
		return false;
	}

	const FVector OutwardNormal = Hit.ImpactNormal.GetSafeNormal();
	FVector InwardDirection = -OutwardNormal.GetSafeNormal2D();
	if (InwardDirection.IsNearlyZero())
	{
		InwardDirection = Owner->GetVelocity().GetSafeNormal2D();
	}
	if (InwardDirection.IsNearlyZero())
	{
		OutRejectReason = TEXT("no-inward-direction");
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | target failed | reason=%s outwardNormal=%s ownerVelocity=%s"),
			*OutRejectReason.ToString(), *OutwardNormal.ToCompactString(), *Owner->GetVelocity().ToCompactString());
		return false;
	}

	const FVector SideDirection = FVector::CrossProduct(FVector::UpVector, InwardDirection).GetSafeNormal();
	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float SafeInset = FMath::Max(LandingInset, CapsuleRadius + 40.0f);

	TArray<FVector> CandidateOffsets;
	CandidateOffsets.Add(InwardDirection * SafeInset);
	CandidateOffsets.Add(InwardDirection * SafeInset + SideDirection * LandingSideRetryOffset);
	CandidateOffsets.Add(InwardDirection * SafeInset - SideDirection * LandingSideRetryOffset);
	CandidateOffsets.Add(InwardDirection * (SafeInset + LandingDeepRetryOffset));

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AbyssMantleTrace), false, Owner);
	const FVector CharLoc = Owner->GetActorLocation();
	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | target search start | char=%s impact=%s outward=%s inward=%s capsuleRadius=%.1f capsuleHalfHeight=%.1f safeInset=%.1f candidates=%d traceChannel=%d"),
		*CharLoc.ToCompactString(),
		*Hit.ImpactPoint.ToCompactString(),
		*OutwardNormal.ToCompactString(),
		*InwardDirection.ToCompactString(),
		CapsuleRadius,
		CapsuleHalfHeight,
		SafeInset,
		CandidateOffsets.Num(),
		static_cast<int32>(TraceChannel.GetValue()));

	for (int32 CandidateIndex = 0; CandidateIndex < CandidateOffsets.Num(); ++CandidateIndex)
	{
		const FVector& Offset = CandidateOffsets[CandidateIndex];
		const FVector ProbeXY = Hit.ImpactPoint + Offset;
		const FVector ProbeStart = ProbeXY + FVector::UpVector * MaxTopSearchHeight;
		const FVector ProbeEnd = FVector(ProbeXY.X, ProbeXY.Y, CharLoc.Z - 50.0f);

		FHitResult GroundHit;
		const bool bHitGround = World->LineTraceSingleByChannel(GroundHit, ProbeStart, ProbeEnd, TraceChannel, QueryParams);
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | candidate %d trace | offset=%s start=%s end=%s hit=%d hitActor=%s hitComp=%s point=%s normal=%s"),
			CandidateIndex,
			*Offset.ToCompactString(),
			*ProbeStart.ToCompactString(),
			*ProbeEnd.ToCompactString(),
			bHitGround,
			*GetNameSafe(GroundHit.GetActor()),
			*GetNameSafe(GroundHit.GetComponent()),
			*GroundHit.ImpactPoint.ToCompactString(),
			*GroundHit.ImpactNormal.ToCompactString());

#if ENABLE_DRAW_DEBUG
		DrawDebugLine(World, ProbeStart, ProbeEnd, bHitGround ? FColor::Green : FColor::Red, false, 2.0f, 0, 2.0f);
#endif

		if (!bHitGround)
		{
			OutRejectReason = TEXT("no-top-floor");
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | candidate %d reject | reason=%s"), CandidateIndex, *OutRejectReason.ToString());
			continue;
		}

		if (GroundHit.ImpactNormal.Z < BBMovement->GetWalkableFloorZ())
		{
			OutRejectReason = TEXT("floor-not-walkable");
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | candidate %d reject | reason=%s floorNormalZ=%.2f walkableFloorZ=%.2f"),
				CandidateIndex, *OutRejectReason.ToString(), GroundHit.ImpactNormal.Z, BBMovement->GetWalkableFloorZ());
			continue;
		}

		const float DepthBelowGround = GroundHit.ImpactPoint.Z - CharLoc.Z;
		if (DepthBelowGround < MinMantleDepth)
		{
			OutRejectReason = TEXT("too-shallow");
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | candidate %d reject | reason=%s depth=%.1f min=%.1f"),
				CandidateIndex, *OutRejectReason.ToString(), DepthBelowGround, MinMantleDepth);
			continue;
		}
		if (DepthBelowGround > MaxMantleDepth)
		{
			OutRejectReason = TEXT("too-deep");
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | candidate %d reject | reason=%s depth=%.1f max=%.1f"),
				CandidateIndex, *OutRejectReason.ToString(), DepthBelowGround, MaxMantleDepth);
			continue;
		}

		const FVector LandingCenter = GroundHit.ImpactPoint + FVector::UpVector * (CapsuleHalfHeight + LandingVerticalClearance);
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | candidate %d landing validate | floor=%s landingCenter=%s depth=%.1f clearance=%.1f"),
			CandidateIndex,
			*GroundHit.ImpactPoint.ToCompactString(),
			*LandingCenter.ToCompactString(),
			DepthBelowGround,
			LandingVerticalClearance);
		if (!ValidateLanding(LandingCenter, CapsuleRadius, CapsuleHalfHeight, OutRejectReason))
		{
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | candidate %d reject | reason=%s landing=%s"),
				CandidateIndex, *OutRejectReason.ToString(), *LandingCenter.ToCompactString());
			continue;
		}

		OutTarget.StartLocation = CharLoc;
		OutTarget.LandingLocation = LandingCenter;
		OutTarget.LandingFloorZ = GroundHit.ImpactPoint.Z;
		OutTarget.OutwardNormal = OutwardNormal;
		OutTarget.InwardDirection = InwardDirection;
		OutTarget.Duration = MantleDuration;

#if ENABLE_DRAW_DEBUG
		DrawDebugPoint(World, GroundHit.ImpactPoint, 14.0f, FColor::Blue, false, 2.0f);
		DrawDebugCapsule(World, LandingCenter, CapsuleHalfHeight, CapsuleRadius, FQuat::Identity, FColor::Cyan, false, 2.0f);
#endif

		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | candidate %d accepted | depth=%.1f landing=%s actor=%s comp=%s"),
			CandidateIndex,
			DepthBelowGround,
			*LandingCenter.ToCompactString(),
			*GetNameSafe(Hit.GetActor()),
			*GetNameSafe(Hit.GetComponent()));
		return true;
	}

	if (OutRejectReason.IsNone())
	{
		OutRejectReason = TEXT("no-valid-landing");
	}
	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | target search failed | finalReason=%s"), *OutRejectReason.ToString());
	return false;
}

bool UBBMantleComponent::ValidateLanding(const FVector& LandingCenter, float CapsuleRadius, float CapsuleHalfHeight, FName& OutRejectReason) const
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner)
	{
		OutRejectReason = TEXT("no-world");
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | landing failed | reason=%s world=%d owner=%s"),
			*OutRejectReason.ToString(), World != nullptr, *GetNameSafe(Owner));
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AbyssMantleLanding), false, Owner);
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);

	TArray<FOverlapResult> LandingOverlaps;
	const bool bOverlaps = World->OverlapMultiByChannel(
		LandingOverlaps,
		LandingCenter,
		FQuat::Identity,
		CapsuleSweepChannel,
		CapsuleShape,
		QueryParams);

	if (bOverlaps)
	{
		OutRejectReason = TEXT("no-headroom");
		const FOverlapResult& FirstOverlap = LandingOverlaps[0];
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | landing failed | reason=%s landing=%s radius=%.1f halfHeight=%.1f sweepChannel=%d overlapCount=%d firstActor=%s firstComp=%s"),
			*OutRejectReason.ToString(),
			*LandingCenter.ToCompactString(),
			CapsuleRadius,
			CapsuleHalfHeight,
			static_cast<int32>(CapsuleSweepChannel.GetValue()),
			LandingOverlaps.Num(),
			*GetNameSafe(FirstOverlap.GetActor()),
			*GetNameSafe(FirstOverlap.GetComponent()));
		return false;
	}

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | landing accepted | boost-target landing=%s radius=%.1f halfHeight=%.1f"),
		*LandingCenter.ToCompactString(),
		CapsuleRadius,
		CapsuleHalfHeight);
	return true;
}

bool UBBMantleComponent::IsRecoverableSurface(const FHitResult& Hit) const
{
	const AActor* HitActor = Hit.GetActor();
	const UPrimitiveComponent* HitComponent = Hit.GetComponent();
	if (!HitActor || !HitComponent)
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | surface check | result=0 reason=missing-hit actor=%s comp=%s"),
			*GetNameSafe(HitActor), *GetNameSafe(HitComponent));
		return false;
	}

	if (HitActor->ActorHasTag(RecoverableActorTag) || HitComponent->ComponentHasTag(RecoverableActorTag))
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | surface check | result=1 reason=tag actor=%s comp=%s tag=%s"),
			*GetNameSafe(HitActor), *GetNameSafe(HitComponent), *RecoverableActorTag.ToString());
		return true;
	}

	if (HitComponent->GetCollisionProfileName() == RecoverableCollisionProfile)
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | surface check | result=1 reason=profile actor=%s comp=%s profile=%s"),
			*GetNameSafe(HitActor), *GetNameSafe(HitComponent), *RecoverableCollisionProfile.ToString());
		return true;
	}

	const bool bHasSurfaceComponent = HitActor->FindComponentByClass<UBBMantleSurfaceComponent>() != nullptr;
	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | surface check | result=%d reason=component actor=%s comp=%s profile=%s expectedTag=%s expectedProfile=%s"),
		bHasSurfaceComponent,
		*GetNameSafe(HitActor),
		*GetNameSafe(HitComponent),
		*HitComponent->GetCollisionProfileName().ToString(),
		*RecoverableActorTag.ToString(),
		*RecoverableCollisionProfile.ToString());
	return bHasSurfaceComponent;
}

bool UBBMantleComponent::IsMovementStateEligible() const
{
	if (!BBMovement || BBMovement->IsMantling())
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | movement eligibility | result=0 hasMovement=%d isMantling=%d"),
			BBMovement != nullptr,
			BBMovement ? BBMovement->IsMantling() : 0);
		return false;
	}

	const bool bEligible = BBMovement->IsFalling() || BBMovement->IsGliding() || BBMovement->IsSpiked();
	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | movement eligibility | result=%d falling=%d gliding=%d spiked=%d mode=%d custom=%d velocity=%s"),
		bEligible,
		BBMovement->IsFalling(),
		BBMovement->IsGliding(),
		BBMovement->IsSpiked(),
		static_cast<int32>(BBMovement->MovementMode),
		static_cast<int32>(BBMovement->CustomMovementMode),
		*BBMovement->Velocity.ToCompactString());
	return bEligible;
}

bool UBBMantleComponent::TryMantle()
{
	ACharacter* Owner = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!Owner || !World || !BBMovement || !Owner->HasAuthority() || !IsMovementStateEligible())
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | manual try failed before trace | owner=%s world=%d movement=%d authority=%d"),
			*GetNameSafe(Owner), World != nullptr, BBMovement != nullptr, Owner ? Owner->HasAuthority() : 0);
		return false;
	}

	UCapsuleComponent* Capsule = Owner->GetCapsuleComponent();
	const float CapsuleRadius = Capsule ? Capsule->GetScaledCapsuleRadius() : 34.0f;
	const FVector Start = Owner->GetActorLocation();
	const FVector Forward = Owner->GetActorForwardVector().GetSafeNormal2D();
	const FVector End = Start + Forward * (CapsuleRadius + 80.0f);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AbyssMantleManualTrace), false, Owner);
	FHitResult WallHit;
	if (!World->LineTraceSingleByChannel(WallHit, Start, End, TraceChannel, QueryParams))
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | manual try trace miss | start=%s end=%s traceChannel=%d"),
			*Start.ToCompactString(), *End.ToCompactString(), static_cast<int32>(TraceChannel.GetValue()));
		return false;
	}

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | manual try trace hit | actor=%s comp=%s point=%s normal=%s"),
		*GetNameSafe(WallHit.GetActor()),
		*GetNameSafe(WallHit.GetComponent()),
		*WallHit.ImpactPoint.ToCompactString(),
		*WallHit.ImpactNormal.ToCompactString());
	return ValidateAndTriggerMantle(WallHit);
}

void UBBMantleComponent::ServerRequestMantle_Implementation()
{
	TryMantle();
}

void UBBMantleComponent::NotifyMantleFinished()
{
	SetMantlingTag(false);
	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | tag cleared and finished notified | owner=%s"), *GetNameSafe(GetOwner()));
}

void UBBMantleComponent::CancelGliderForMantle() const
{
	if (AActor* Owner = GetOwner())
	{
		if (UGliderComponent* Glider = Owner->FindComponentByClass<UGliderComponent>())
		{
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | cancelling glider before mantle | owner=%s"), *GetNameSafe(Owner));
			Glider->CancelForMantle();
		}
		else
		{
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | no glider to cancel before mantle | owner=%s"), *GetNameSafe(Owner));
		}
	}
}

void UBBMantleComponent::MarkFailedAttempt() const
{
	if (const UWorld* World = GetWorld())
	{
		LastFailedAttemptTime = World->GetTimeSeconds();
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | failed attempt cooldown marked | time=%.3f duration=%.3f"),
			LastFailedAttemptTime, FailedAttemptCooldown);
	}
}

void UBBMantleComponent::SetMantlingTag(bool bActive)
{
	if (!ASC)
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | tag update skipped | active=%d reason=no-asc owner=%s"),
			bActive, *GetNameSafe(GetOwner()));
		return;
	}

	if (bActive)
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | tag applied | tag=State.Mantling owner=%s"), *GetNameSafe(GetOwner()));
		ASC->AddLooseGameplayTag(BBGameplayTags::State_Mantling);
	}
	else
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | tag removed | tag=State.Mantling owner=%s"), *GetNameSafe(GetOwner()));
		ASC->RemoveLooseGameplayTag(BBGameplayTags::State_Mantling);
	}
}
