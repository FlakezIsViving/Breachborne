#include "BBStormShift_BulletTrains.h"
#include "EngineUtils.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBMovementSet.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Storm/BBTrain.h"
#include "Breachborne/Storm/BBTrainTrack.h"
#include "Breachborne/Breachborne.h"

TArray<FStormPhaseConfig> UBBStormShift_BulletTrains::GetPhaseConfigs(float InitialRadius) const
{
	// 4 escalating phases. 20% shorter shrinks than Default, ramping damage.
	TArray<FStormPhaseConfig> Configs;

	FStormPhaseConfig Phase1;
	Phase1.TargetRadiusFraction = 0.65f;
	Phase1.ShrinkDuration       = 48.0f;  // vs Default 60s
	Phase1.HoldDuration         = 24.0f;
	Phase1.DamagePerTick        = 5.0f;
	Phase1.DamageTickInterval   = 1.0f;
	Configs.Add(Phase1);

	FStormPhaseConfig Phase2;
	Phase2.TargetRadiusFraction = 0.40f;
	Phase2.ShrinkDuration       = 40.0f;
	Phase2.HoldDuration         = 20.0f;
	Phase2.DamagePerTick        = 12.0f;
	Phase2.DamageTickInterval   = 1.0f;
	Configs.Add(Phase2);

	FStormPhaseConfig Phase3;
	Phase3.TargetRadiusFraction = 0.20f;
	Phase3.ShrinkDuration       = 32.0f;
	Phase3.HoldDuration         = 15.0f;
	Phase3.DamagePerTick        = 25.0f;
	Phase3.DamageTickInterval   = 0.8f;
	Configs.Add(Phase3);

	FStormPhaseConfig Phase4;
	Phase4.TargetRadiusFraction = 0.05f;
	Phase4.ShrinkDuration       = 24.0f;
	Phase4.HoldDuration         = 0.0f;
	Phase4.DamagePerTick        = 50.0f;  // 2x Default final phase
	Phase4.DamageTickInterval   = 0.5f;
	Configs.Add(Phase4);

	return Configs;
}

FVector2D UBBStormShift_BulletTrains::PickNextCenter(int32 PhaseIndex, const FVector2D& CurrentCenter, float NewRadius) const
{
	if (PhaseIndex == 0)
	{
		// Phase 1 starts at current center (map origin)
		return CurrentCenter;
	}

	// Each subsequent phase: drift the center up to CenterDriftFraction of remaining radius
	const float MaxDrift = NewRadius * CenterDriftFraction;
	const float DriftAngle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float DriftDist  = FMath::FRandRange(0.0f, MaxDrift);

	const FVector2D Drift(FMath::Cos(DriftAngle) * DriftDist, FMath::Sin(DriftAngle) * DriftDist);

	UE_LOG(LogBreachborne, Log, TEXT("BulletTrains: Phase %d center drifts by (%.0f, %.0f)"),
		PhaseIndex + 1, Drift.X, Drift.Y);

	return CurrentCenter + Drift;
}

void UBBStormShift_BulletTrains::ApplyShiftEffects(UWorld* World, int32 PhaseIndex)
{
	if (!World)
	{
		return;
	}

	// --- Speed buff (existing behavior) ---
	UGameplayEffect* SpeedBuffGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_BulletTrains_SpeedBuff"));
	SpeedBuffGE->DurationPolicy = EGameplayEffectDurationType::Infinite;

	FGameplayModifierInfo SpeedMod;
	SpeedMod.Attribute      = UBBMovementSet::GetMoveSpeedAttribute();
	SpeedMod.ModifierOp     = EGameplayModOp::Multiplicitive;
	SpeedMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(SpeedMultiplier));
	SpeedBuffGE->Modifiers.Add(SpeedMod);

	UTargetTagsGameplayEffectComponent& TagComp = SpeedBuffGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::Effect_StormShift_BulletTrains);
	TagComp.SetAndApplyTargetTagChanges(TagContainer);

	int32 ApplyCount = 0;
	for (AHunterCharacter* Hunter : TActorRange<AHunterCharacter>(World))
	{
		if (!Hunter->HasAuthority())
		{
			continue;
		}

		UAbilitySystemComponent* ASC = Hunter->GetAbilitySystemComponent();
		if (!ASC)
		{
			continue;
		}

		const ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>();
		if (PS && PS->GetIsAlive())
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(SpeedBuffGE->GetClass(), 1, Context);
			if (!Spec.IsValid())
			{
				ASC->ApplyGameplayEffectToSelf(SpeedBuffGE, 1, ASC->MakeEffectContext());
				ApplyCount++;
			}
		}
	}

	UE_LOG(LogBreachborne, Warning, TEXT("BulletTrains Phase %d: Applied +30%% MoveSpeed buff to %d hunter(s)"),
		PhaseIndex + 1, ApplyCount);

	// --- Spawn trains ---
	SpawnTrains(World);
}

void UBBStormShift_BulletTrains::RemoveShiftEffects(UWorld* World)
{
	if (!World)
	{
		return;
	}

	// Remove speed buff
	FGameplayTagContainer RemoveTags;
	RemoveTags.AddTag(BBGameplayTags::Effect_StormShift_BulletTrains);

	int32 RemoveCount = 0;
	for (AHunterCharacter* Hunter : TActorRange<AHunterCharacter>(World))
	{
		if (!Hunter->HasAuthority())
		{
			continue;
		}

		UAbilitySystemComponent* ASC = Hunter->GetAbilitySystemComponent();
		if (!ASC)
		{
			continue;
		}

		const int32 Removed = ASC->RemoveActiveEffectsWithGrantedTags(RemoveTags);
		RemoveCount += Removed;
	}

	UE_LOG(LogBreachborne, Log, TEXT("BulletTrains: Removed speed buff from %d hunter(s)"), RemoveCount);

	// Destroy trains
	DestroyTrains();
}

void UBBStormShift_BulletTrains::SpawnTrains(UWorld* World)
{
	if (!World)
	{
		return;
	}

	// Find all train tracks in the world
	TArray<ABBTrainTrack*> Tracks;
	for (TActorIterator<ABBTrainTrack> It(World); It; ++It)
	{
		Tracks.Add(*It);
	}

	if (Tracks.Num() == 0)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BulletTrains: No train tracks found in world!"));
		return;
	}

	UE_LOG(LogBreachborne, Log, TEXT("BulletTrains: Found %d track(s)"), Tracks.Num());

	// Spawn trains on each track
	// In BulletTrains mode: 3 mini trains per track, equidistant
	// For now, we always spawn in BulletTrains mode when this shift is active
	const bool bBulletTrainMode = true;

	for (ABBTrainTrack* Track : Tracks)
	{
		if (!Track)
		{
			continue;
		}

		if (bBulletTrainMode)
		{
			// 3 mini trains, equidistant
			const float Spacing = 1.0f / 3.0f;
			for (int32 i = 0; i < 3; ++i)
			{
				SpawnTrainOnTrack(Track, i * Spacing, true);
			}
		}
		else
		{
			// 1 normal train
			SpawnTrainOnTrack(Track, 0.0f, false);
		}
	}

	UE_LOG(LogBreachborne, Log, TEXT("BulletTrains: Spawned %d train(s) total"), SpawnedTrains.Num());
}

void UBBStormShift_BulletTrains::DestroyTrains()
{
	for (ABBTrain* Train : SpawnedTrains)
	{
		if (Train)
		{
			Train->Destroy();
		}
	}
	SpawnedTrains.Empty();

	UE_LOG(LogBreachborne, Log, TEXT("BulletTrains: All trains destroyed"));
}

void UBBStormShift_BulletTrains::SpawnTrainOnTrack(ABBTrainTrack* Track, float InitialProgress, bool bBulletTrainMode)
{
	if (!Track || !Track->GetWorld())
	{
		return;
	}

	UWorld* World = Track->GetWorld();
	const FTransform SpawnTransform = Track->GetTransformAtProgress(InitialProgress, true);

	ABBTrain* Train = World->SpawnActor<ABBTrain>(ABBTrain::StaticClass(), SpawnTransform);
	if (!Train)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BulletTrains: Failed to spawn train on track %s"), *Track->GetName());
		return;
	}

	const ETrainMode Mode = bBulletTrainMode ? ETrainMode::BulletTrains : ETrainMode::Normal;
	Train->InitializeTrain(Track, Mode);
	Train->StartMoving();

	SpawnedTrains.Add(Train);

	UE_LOG(LogBreachborne, Log, TEXT("BulletTrains: Spawned train on %s at progress %.2f (mode=%s)"),
		*Track->GetName(), InitialProgress, bBulletTrainMode ? TEXT("BulletTrains") : TEXT("Normal"));
}
