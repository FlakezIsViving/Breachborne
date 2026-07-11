#include "BBVisionConeComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Core/BreachborneGameState.h"
#include "Breachborne/Breachborne.h"

UBBVisionConeComponent::UBBVisionConeComponent()
{
	// Tick on owning client for rendering; also ticks on server for brush-visibility checks
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f; // 10Hz — fine for vision queries
}

void UBBVisionConeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Determine day/night
	const ABreachborneGameState* GS = World->GetGameState<ABreachborneGameState>();
	const bool bIsNight = GS && GS->GetDayNightPhase() == EBBDayNightPhase::Night;
	float BaseRange = bIsNight ? NightVisionRange : DayVisionRange;

	// If inside a brush (State_InBrush tag), reduce outward vision
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			if (ASC->HasMatchingGameplayTag(BBGameplayTags::State_InBrush))
			{
				BaseRange *= BrushVisionMultiplier;
			}
		}
	}

	CachedEffectiveRange = BaseRange;
}

bool UBBVisionConeComponent::IsInVisionCone(const FVector& WorldLocation) const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	const FVector OwnerLoc  = Owner->GetActorLocation();
	const FVector ToTarget  = WorldLocation - OwnerLoc;
	const float   Distance2D = FVector2D::Distance(FVector2D(OwnerLoc.X, OwnerLoc.Y), FVector2D(WorldLocation.X, WorldLocation.Y));

	if (Distance2D > CachedEffectiveRange)
	{
		return false;
	}

	// Cone check against owner forward direction
	const FVector Forward  = Owner->GetActorForwardVector().GetSafeNormal2D();
	const FVector ToTarget2D = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal2D();
	const float   CosHalf  = FMath::Cos(FMath::DegreesToRadians(VisionHalfAngle));

	return FVector::DotProduct(Forward, ToTarget2D) >= CosHalf;
}

float UBBVisionConeComponent::GetEffectiveRange() const
{
	return CachedEffectiveRange;
}
