#include "BBCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Characters/BBMantleComponent.h"
#include "Breachborne/Characters/GliderComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBMovementSet.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Breachborne.h"

UBBCharacterMovementComponent::UBBCharacterMovementComponent()
{
	// Default CMC settings are inherited from HunterCharacter constructor
}

bool UBBCharacterMovementComponent::IsGliding() const
{
	return MovementMode == MOVE_Custom
		&& CustomMovementMode == static_cast<uint8>(EBBCustomMovementMode::Gliding);
}

bool UBBCharacterMovementComponent::IsSpiked() const
{
	return MovementMode == MOVE_Custom
		&& CustomMovementMode == static_cast<uint8>(EBBCustomMovementMode::Spiked);
}

void UBBCharacterMovementComponent::EnterGliding()
{
	bHasGlideTargetZ = false;
	GlideTargetZ = 0.0f;

	if (UWorld* World = GetWorld())
	{
		const FVector CurrentLocation = UpdatedComponent ? UpdatedComponent->GetComponentLocation() : FVector::ZeroVector;
		float NearbyTargetZ = 0.0f;
		if (FindNearbyWalkableGlideTargetZ(CurrentLocation, NearbyTargetZ))
		{
			GlideTargetZ = NearbyTargetZ;
			bHasGlideTargetZ = true;
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("glider debug | recovery target found | current=%s targetZ=%.1f"),
				*CurrentLocation.ToCompactString(),
				GlideTargetZ);
		}
		else
		{
			UE_LOG(LogBreachborne, Warning, TEXT("BB_DROP|GLIDE|NoGroundTarget actor=%s current=%s action=controlled_descent"),
				*GetNameSafe(GetOwner()),
				*CurrentLocation.ToCompactString());
		}
	}

	SetMovementMode(MOVE_Custom, static_cast<uint8>(EBBCustomMovementMode::Gliding));
	UE_LOG(LogBreachborne, Warning, TEXT("BB_DROP|GLIDE|Enter actor=%s loc=%s hasTarget=%d targetZ=%.1f"),
		*GetNameSafe(GetOwner()),
		UpdatedComponent ? *UpdatedComponent->GetComponentLocation().ToCompactString() : TEXT("no-updated-component"),
		bHasGlideTargetZ ? 1 : 0,
		GlideTargetZ);
}

bool UBBCharacterMovementComponent::FindNearbyWalkableGlideTargetZ(const FVector& CurrentLocation, float& OutTargetZ) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Forward = CharacterOwner ? CharacterOwner->GetActorForwardVector().GetSafeNormal2D() : FVector::ForwardVector;
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal2D();
	const float Radius = FMath::Max(100.0f, GlideNearbyGroundSampleRadius);
	const float ForwardBias = FMath::Max(0.0f, GlideNearbyGroundSampleForwardBias);

	TArray<FVector, TInlineAllocator<10>> SampleOffsets;
	SampleOffsets.Add(FVector::ZeroVector);
	SampleOffsets.Add(Forward * ForwardBias);
	SampleOffsets.Add(Forward * Radius);
	SampleOffsets.Add(-Forward * Radius);
	SampleOffsets.Add(Right * Radius);
	SampleOffsets.Add(-Right * Radius);
	SampleOffsets.Add((Forward + Right).GetSafeNormal2D() * Radius);
	SampleOffsets.Add((Forward - Right).GetSafeNormal2D() * Radius);
	SampleOffsets.Add((-Forward + Right).GetSafeNormal2D() * Radius);
	SampleOffsets.Add((-Forward - Right).GetSafeNormal2D() * Radius);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(GlideNearbyGroundSearch), false, CharacterOwner);
	float TotalGroundZ = 0.0f;
	int32 GroundHitCount = 0;

	for (const FVector& Offset : SampleOffsets)
	{
		const FVector SampleCenter = CurrentLocation + Offset;
		const FVector TraceStart = SampleCenter + FVector::UpVector * GlideTopSearchHeight;
		const FVector TraceEnd = SampleCenter - FVector::UpVector * GlideTopSearchDepth;

		FHitResult GroundHit;
		if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, Params)
			&& GroundHit.ImpactNormal.Z >= GetWalkableFloorZ())
		{
			TotalGroundZ += GroundHit.ImpactPoint.Z;
			++GroundHitCount;
		}
	}

	if (GroundHitCount <= 0)
	{
		return false;
	}

	const float AverageGroundZ = TotalGroundZ / static_cast<float>(GroundHitCount);
	OutTargetZ = AverageGroundZ + GlideRecoveryClearance;
	return true;
}

void UBBCharacterMovementComponent::ExitGliding()
{
	bHasGlideTargetZ = false;
	GlideTargetZ = 0.0f;
	SetMovementMode(MOVE_Falling);
}

void UBBCharacterMovementComponent::EnterSpiked()
{
	SetMovementMode(MOVE_Custom, static_cast<uint8>(EBBCustomMovementMode::Spiked));
}

void UBBCharacterMovementComponent::ExitSpiked()
{
	SetMovementMode(MOVE_Falling);
}

void UBBCharacterMovementComponent::EnterMantling(const FBBMantleTarget& InTarget)
{
	MantleTarget = InTarget;
	MantleTarget.StartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	MantlePreviousPathLocation = MantleTarget.StartLocation;

	const FVector CurrentLocation = UpdatedComponent ? UpdatedComponent->GetComponentLocation() : MantleTarget.StartLocation;
	const float GravityMagnitude = FMath::Max(FMath::Abs(GetGravityZ()), 1.0f);
	const float TargetApexZ = MantleTarget.LandingLocation.Z + MantleBoostExtraHeight;
	const float RequiredLiftHeight = FMath::Max(TargetApexZ - CurrentLocation.Z, 0.0f);
	const float RequiredUpwardSpeed = FMath::Sqrt(2.0f * GravityMagnitude * RequiredLiftHeight);
	const float KickedUpwardSpeed = RequiredUpwardSpeed * MantleInitialKickMultiplier;
	const float UpwardSpeed = FMath::Clamp(KickedUpwardSpeed, MantleMinUpwardBoostSpeed, MantleMaxUpwardBoostSpeed);
	const FVector InwardVelocity = MantleTarget.InwardDirection.GetSafeNormal2D() * MantleBoostInwardSpeed;

	Velocity.X = InwardVelocity.X;
	Velocity.Y = InwardVelocity.Y;
	Velocity.Z = FMath::Max(Velocity.Z, UpwardSpeed);
	SetMovementMode(MOVE_Falling);

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | boost applied | start=%s current=%s landing=%s floorZ=%.1f targetApexZ=%.1f requiredHeight=%.1f requiredUpSpeed=%.1f kickMultiplier=%.2f kickedUpSpeed=%.1f appliedVelocity=%s inward=%s outward=%s mode=%d custom=%d"),
		*MantleTarget.StartLocation.ToCompactString(),
		*CurrentLocation.ToCompactString(),
		*MantleTarget.LandingLocation.ToCompactString(),
		MantleTarget.LandingFloorZ,
		TargetApexZ,
		RequiredLiftHeight,
		RequiredUpwardSpeed,
		MantleInitialKickMultiplier,
		KickedUpwardSpeed,
		*Velocity.ToCompactString(),
		*MantleTarget.InwardDirection.ToCompactString(),
		*MantleTarget.OutwardNormal.ToCompactString(),
		static_cast<int32>(MovementMode),
		static_cast<int32>(CustomMovementMode));

	MantleTarget = FBBMantleTarget();
	MantlePreviousPathLocation = FVector::ZeroVector;
}

void UBBCharacterMovementComponent::FinishMantling()
{
	if (!IsMantling())
	{
		return;
	}

	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, true);
	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | movement finish | location=%s exitVelocity=%s floorWalkable=%d floorDist=%.1f"),
		UpdatedComponent ? *UpdatedComponent->GetComponentLocation().ToCompactString() : TEXT("no-updated-component"),
		*Velocity.ToCompactString(),
		CurrentFloor.IsWalkableFloor(),
		CurrentFloor.FloorDist);
	SetMovementMode(MOVE_Walking);
}

void UBBCharacterMovementComponent::AbortMantling()
{
	if (!IsMantling())
	{
		return;
	}

	Velocity.Z = FMath::Min(Velocity.Z, 0.0f);
	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | movement abort | location=%s velocity=%s"),
		UpdatedComponent ? *UpdatedComponent->GetComponentLocation().ToCompactString() : TEXT("no-updated-component"),
		*Velocity.ToCompactString());
	SetMovementMode(MOVE_Falling);
}

void UBBCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	switch (static_cast<EBBCustomMovementMode>(CustomMovementMode))
	{
	case EBBCustomMovementMode::Gliding:
		PhysGliding(deltaTime, Iterations);
		break;

	case EBBCustomMovementMode::Spiked:
		PhysSpiked(deltaTime, Iterations);
		break;

	case EBBCustomMovementMode::Mantling:
		PhysMantling(deltaTime, Iterations);
		break;

	default:
		Super::PhysCustom(deltaTime, Iterations);
		break;
	}
}

void UBBCharacterMovementComponent::PhysGliding(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	const FVector CursorDir = GetPawnOwner()->GetActorForwardVector().GetSafeNormal2D();
	const FVector TargetVel = CursorDir * GlidingMaxSpeed;
	const float AccelRate = (GlidingAccelerationTime > KINDA_SMALL_NUMBER)
		? GlidingMaxSpeed / GlidingAccelerationTime
		: GlidingMaxSpeed;

	const FVector CurrentHorizontalVel(Velocity.X, Velocity.Y, 0.0f);
	const float CurrentSpeed = CurrentHorizontalVel.Size();

	if (CurrentSpeed > SharpTurnSpeedThreshold)
	{
		const FVector CurrentDir = CurrentHorizontalVel.GetSafeNormal();
		const float Dot = FVector::DotProduct(CurrentDir, CursorDir);

		if (Dot < 0.0f)
		{
			Velocity.X = FMath::FInterpConstantTo(Velocity.X, 0.0f, DeltaTime, GlidingSharpTurnBraking);
			Velocity.Y = FMath::FInterpConstantTo(Velocity.Y, 0.0f, DeltaTime, GlidingSharpTurnBraking);
		}
		else
		{
			Velocity.X = FMath::FInterpConstantTo(Velocity.X, TargetVel.X, DeltaTime, AccelRate);
			Velocity.Y = FMath::FInterpConstantTo(Velocity.Y, TargetVel.Y, DeltaTime, AccelRate);
		}
	}
	else
	{
		Velocity.X = FMath::FInterpConstantTo(Velocity.X, TargetVel.X, DeltaTime, AccelRate);
		Velocity.Y = FMath::FInterpConstantTo(Velocity.Y, TargetVel.Y, DeltaTime, AccelRate);
	}

	bool bHasWalkableGroundBelow = false;

	if (UpdatedComponent && GetWorld())
	{
		const FVector TraceStart = UpdatedComponent->GetComponentLocation();
		const FVector TraceEnd = TraceStart - FVector::UpVector * GlideGroundProbeDistance;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GlideGroundProbe), false, CharacterOwner);

		FHitResult GroundHit;
		if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, Params)
			&& GroundHit.ImpactNormal.Z >= GetWalkableFloorZ())
		{
			bHasWalkableGroundBelow = true;
		}
	}

	float DesiredVerticalVelocity = 0.0f;
	if (bHasWalkableGroundBelow)
	{
		DesiredVerticalVelocity = -GlideSolidGroundSinkSpeed;
	}
	else if (bHasGlideTargetZ && UpdatedComponent)
	{
		const float HeightError = GlideTargetZ - UpdatedComponent->GetComponentLocation().Z;
		if (FMath::Abs(HeightError) > GlideSoftCeilingOffset)
		{
			DesiredVerticalVelocity = FMath::Clamp(HeightError * GlideHeightCorrectionSpeed, -GlideMaxFallSpeed, GlideMaxRiseSpeed);
		}
	}
	else
	{
		DesiredVerticalVelocity = -GlideMaxFallSpeed;
	}

	Velocity.Z = FMath::FInterpTo(Velocity.Z, DesiredVerticalVelocity, DeltaTime, GlideVerticalVelocityInterpSpeed);
	Velocity.Z = FMath::Clamp(Velocity.Z, -GlideMaxFallSpeed, GlideMaxRiseSpeed);

	const FVector Delta = Velocity * DeltaTime;
	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);

	if (Hit.IsValidBlockingHit())
	{
		if (Hit.ImpactNormal.Z > GetWalkableFloorZ())
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, true);
			SetMovementMode(MOVE_Walking);
			if (ACharacter* OwnerCharacter = CharacterOwner)
			{
				if (UGliderComponent* Glider = OwnerCharacter->FindComponentByClass<UGliderComponent>())
				{
					UE_LOG(LogBreachborne, VeryVerbose, TEXT("glider debug | closed on walkable hit | actor=%s comp=%s fuel=%.2f"),
						*GetNameSafe(Hit.GetActor()),
						*GetNameSafe(Hit.GetComponent()),
						Glider->GetFuelLevel());
					UE_LOG(LogBreachborne, Warning, TEXT("BB_DROP|GLIDE|WalkableHitClose owner=%s hitActor=%s hitComp=%s loc=%s fuel=%.2f"),
						*GetNameSafe(OwnerCharacter),
						*GetNameSafe(Hit.GetActor()),
						*GetNameSafe(Hit.GetComponent()),
						UpdatedComponent ? *UpdatedComponent->GetComponentLocation().ToCompactString() : TEXT("no-updated-component"),
						Glider->GetFuelLevel());
					Glider->ForceCloseGlider();
				}
			}
			return;
		}
		if (ACharacter* OwnerCharacter = CharacterOwner)
		{
			if (UGliderComponent* Glider = OwnerCharacter->FindComponentByClass<UGliderComponent>())
			{
				UE_LOG(LogBreachborne, Warning, TEXT("BB_DROP|GLIDE|BlockedClose owner=%s hitActor=%s hitComp=%s point=%s normal=%s loc=%s velocity=%s fuel=%.2f"),
					*GetNameSafe(OwnerCharacter),
					*GetNameSafe(Hit.GetActor()),
					*GetNameSafe(Hit.GetComponent()),
					*Hit.ImpactPoint.ToCompactString(),
					*Hit.ImpactNormal.ToCompactString(),
					UpdatedComponent ? *UpdatedComponent->GetComponentLocation().ToCompactString() : TEXT("no-updated-component"),
					*Velocity.ToCompactString(),
					Glider->GetFuelLevel());
				Glider->ForceCloseGlider();
				return;
			}
		}
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | gliding blocking hit before slide | actor=%s comp=%s point=%s normal=%s velocity=%s"),
			*GetNameSafe(Hit.GetActor()),
			*GetNameSafe(Hit.GetComponent()),
			*Hit.ImpactPoint.ToCompactString(),
			*Hit.ImpactNormal.ToCompactString(),
			*Velocity.ToCompactString());
		SlideAlongSurface(Delta, 1.0f - Hit.Time, Hit.ImpactNormal, Hit, true);
	}
}

void UBBCharacterMovementComponent::PhysSpiked(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	const float GravityZ = GetGravityZ();
	Velocity.Z += GravityZ * DeltaTime;
	Velocity.Z = FMath::Min(Velocity.Z, SpikedDownwardBoost);

	Velocity.X = FMath::FInterpConstantTo(Velocity.X, 0.0f, DeltaTime, 1000.0f);
	Velocity.Y = FMath::FInterpConstantTo(Velocity.Y, 0.0f, DeltaTime, 1000.0f);

	const FVector Delta = Velocity * DeltaTime;
	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);

	if (Hit.IsValidBlockingHit())
	{
		if (Hit.ImpactNormal.Z > GetWalkableFloorZ())
		{
			SetMovementMode(MOVE_Walking);
			return;
		}
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | spiked blocking hit before slide | actor=%s comp=%s point=%s normal=%s velocity=%s"),
			*GetNameSafe(Hit.GetActor()),
			*GetNameSafe(Hit.GetComponent()),
			*Hit.ImpactPoint.ToCompactString(),
			*Hit.ImpactNormal.ToCompactString(),
			*Velocity.ToCompactString());
		SlideAlongSurface(Delta, 1.0f - Hit.Time, Hit.ImpactNormal, Hit, true);
	}
}

float UBBCharacterMovementComponent::GetMaxSpeed() const
{
	if (IsGliding())
	{
		return GlidingMaxSpeed;
	}
	if (IsMantling())
	{
		return 0.0f;
	}

	float Speed = Super::GetMaxSpeed();
	if (const ACharacter* Character = CharacterOwner)
	{
		if (const ABreachbornePlayerState* PS = Character->GetPlayerState<ABreachbornePlayerState>())
		{
			if (const UBBMovementSet* MovementSet = PS->GetMovementSet())
			{
				Speed = MovementSet->GetMoveSpeed();
			}

			if (const UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
			{
				if (ASC->HasMatchingGameplayTag(BBGameplayTags::State_Hudson_Spinning)
					|| ASC->HasMatchingGameplayTag(BBGameplayTags::State_Hudson_SpunUp))
				{
					Speed *= 0.5f;
				}
			}
		}
	}

	return Speed;
}

void UBBCharacterMovementComponent::PhysMantling(float DeltaTime, int32 Iterations)
{
	if (DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (!MantleTarget.IsValid() || !UpdatedComponent)
	{
		if (CharacterOwner && !CharacterOwner->HasAuthority())
		{
			return;
		}

		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | movement abort reason=invalid-target | hasUpdated=%d hasAuthority=%d mode=%d custom=%d"),
			UpdatedComponent != nullptr,
			CharacterOwner ? CharacterOwner->HasAuthority() : 0,
			static_cast<int32>(MovementMode),
			static_cast<int32>(CustomMovementMode));
		AbortMantling();
		return;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : MantleTarget.StartTime + MantleTarget.Duration;
	const float Elapsed = Now - MantleTarget.StartTime;
	if (Elapsed > MantleMaxDuration)
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | movement abort reason=timeout | elapsed=%.2f max=%.2f location=%s"),
			Elapsed,
			MantleMaxDuration,
			UpdatedComponent ? *UpdatedComponent->GetComponentLocation().ToCompactString() : TEXT("no-updated-component"));
		AbortMantling();
		return;
	}

	const float Alpha = FMath::Clamp(Elapsed / FMath::Max(MantleTarget.Duration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	const FVector ApexLocation(
		MantleTarget.StartLocation.X,
		MantleTarget.StartLocation.Y,
		FMath::Max(MantleTarget.StartLocation.Z, MantleTarget.LandingLocation.Z + MantleBoostExtraHeight));
	const FVector AboveLandingLocation(
		MantleTarget.LandingLocation.X,
		MantleTarget.LandingLocation.Y,
		ApexLocation.Z);

	FVector DesiredLocation = MantleTarget.LandingLocation;
	float SegmentAlpha = 1.0f;
	if (Alpha < 0.45f)
	{
		SegmentAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha / 0.45f);
		DesiredLocation = FMath::Lerp(MantleTarget.StartLocation, ApexLocation, SegmentAlpha);
	}
	else if (Alpha < 0.80f)
	{
		SegmentAlpha = FMath::SmoothStep(0.0f, 1.0f, (Alpha - 0.45f) / 0.35f);
		DesiredLocation = FMath::Lerp(ApexLocation, AboveLandingLocation, SegmentAlpha);
	}
	else
	{
		SegmentAlpha = FMath::SmoothStep(0.0f, 1.0f, (Alpha - 0.80f) / 0.20f);
		DesiredLocation = FMath::Lerp(AboveLandingLocation, MantleTarget.LandingLocation, SegmentAlpha);
	}
	const FVector Delta = DesiredLocation - UpdatedComponent->GetComponentLocation();

	UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | movement tick | alpha=%.2f segmentAlpha=%.2f elapsed=%.2f current=%s apex=%s aboveLanding=%s desired=%s delta=%s"),
		Alpha,
		SegmentAlpha,
		Elapsed,
		*UpdatedComponent->GetComponentLocation().ToCompactString(),
		*ApexLocation.ToCompactString(),
		*AboveLandingLocation.ToCompactString(),
		*DesiredLocation.ToCompactString(),
		*Delta.ToCompactString());

	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentRotation(), true, Hit);

	if (Hit.IsValidBlockingHit())
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | movement abort reason=path-blocked | actor=%s comp=%s point=%s normal=%s time=%.2f attemptedDelta=%s"),
			*GetNameSafe(Hit.GetActor()),
			*GetNameSafe(Hit.GetComponent()),
			*Hit.ImpactPoint.ToCompactString(),
			*Hit.ImpactNormal.ToCompactString(),
			Hit.Time,
			*Delta.ToCompactString());
		AbortMantling();
		return;
	}

	Velocity = DeltaTime > KINDA_SMALL_NUMBER
		? (UpdatedComponent->GetComponentLocation() - MantlePreviousPathLocation) / DeltaTime
		: FVector::ZeroVector;
	MantlePreviousPathLocation = UpdatedComponent->GetComponentLocation();

#if ENABLE_DRAW_DEBUG
	if (GetWorld())
	{
		DrawDebugLine(GetWorld(), MantleTarget.StartLocation, ApexLocation, FColor::Cyan, false, 0.1f, 0, 2.0f);
		DrawDebugLine(GetWorld(), ApexLocation, AboveLandingLocation, FColor::Cyan, false, 0.1f, 0, 2.0f);
		DrawDebugLine(GetWorld(), AboveLandingLocation, MantleTarget.LandingLocation, FColor::Cyan, false, 0.1f, 0, 2.0f);
	}
#endif

	if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
	{
		FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, true);
		if (CurrentFloor.IsWalkableFloor())
		{
			UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | movement reached landing | location=%s floorWalkable=1 floorDist=%.1f"),
				*UpdatedComponent->GetComponentLocation().ToCompactString(),
				CurrentFloor.FloorDist);
			FinishMantling();
			return;
		}

		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | movement abort reason=no-floor-at-landing | location=%s floorWalkable=0 floorDist=%.1f"),
			*UpdatedComponent->GetComponentLocation().ToCompactString(),
			CurrentFloor.FloorDist);
		AbortMantling();
	}
}

void UBBCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (MovementMode == MOVE_Custom
		&& CustomMovementMode == static_cast<uint8>(EBBCustomMovementMode::Mantling))
	{
		if (MantleTarget.StartTime <= 0.0f)
		{
			MantleTarget.StartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
		}
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | movement mode changed enter | previousMode=%d previousCustom=%d startTime=%.3f"),
			static_cast<int32>(PreviousMovementMode),
			static_cast<int32>(PreviousCustomMode),
			MantleTarget.StartTime);
	}
	else if (PreviousMovementMode == MOVE_Custom
		&& PreviousCustomMode == static_cast<uint8>(EBBCustomMovementMode::Mantling))
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | movement mode changed exit | newMode=%d newCustom=%d location=%s"),
			static_cast<int32>(MovementMode),
			static_cast<int32>(CustomMovementMode),
			UpdatedComponent ? *UpdatedComponent->GetComponentLocation().ToCompactString() : TEXT("no-updated-component"));
		MantleTarget = FBBMantleTarget();
		MantlePreviousPathLocation = FVector::ZeroVector;

		if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
		{
			if (UBBMantleComponent* Mantle = Owner->FindComponentByClass<UBBMantleComponent>())
			{
				Mantle->NotifyMantleFinished();
			}
		}
	}
}
