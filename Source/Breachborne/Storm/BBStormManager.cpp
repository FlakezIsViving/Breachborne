#include "BBStormManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "StormShiftBase.h"
#include "StormShift_Default.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBDamageExecution.h"
#include "Breachborne/Breachborne.h"

ABBStormManager::ABBStormManager()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// Defaults
	CurrentCenter = FVector2D::ZeroVector;
	CurrentRadius = 0.0f;
	TargetCenter = FVector2D::ZeroVector;
	TargetRadius = 0.0f;
	CurrentPhaseIndex = 0;
	PhaseState = EStormPhaseState::Waiting;
}

void ABBStormManager::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	// Initialize ASC — actor-owned (storm is the source of damage, not a player)
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	// Create the programmatic damage GameplayEffect
	StormDamageGE = NewObject<UGameplayEffect>(this, TEXT("GE_StormDamage"));
	StormDamageGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	// Add BBDamageExecution as the execution calculation
	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UBBDamageExecution::StaticClass();
	StormDamageGE->Executions.Add(ExecDef);

	// Tag the effect so systems can identify storm damage
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::Effect_StormDamage);
	StormDamageGE->InheritableOwnedTagsContainer = TagContainer;

	// Set up the storm shift
	if (!StormShiftClass)
	{
		StormShiftClass = UStormShift_Default::StaticClass();
	}
	ActiveShift = NewObject<UStormShiftBase>(this, StormShiftClass);

	// Cache phase configs
	PhaseConfigs = ActiveShift->GetPhaseConfigs(InitialRadius);

	// Set initial state
	CurrentCenter = InitialCenter;
	CurrentRadius = InitialRadius;
	TargetCenter = InitialCenter;
	TargetRadius = InitialRadius;
	PhaseState = EStormPhaseState::Waiting;
	CurrentPhaseIndex = 0;

	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT("=== STORM MANAGER INITIALIZED ==="));
	UE_LOG(LogBreachborne, Warning, TEXT("    Shift: %s | Phases: %d | Radius: %.0f"),
		*ActiveShift->GetShiftName().ToString(), PhaseConfigs.Num(), InitialRadius);

	if (bAutoStart)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("    Auto-starting in %.1fs"), AutoStartDelay);
		GetWorldTimerManager().SetTimer(AutoStartTimerHandle, this, &ABBStormManager::StartStorm, AutoStartDelay, false);
	}
}

void ABBStormManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABBStormManager, CurrentCenter);
	DOREPLIFETIME(ABBStormManager, CurrentRadius);
	DOREPLIFETIME(ABBStormManager, TargetCenter);
	DOREPLIFETIME(ABBStormManager, TargetRadius);
	DOREPLIFETIME(ABBStormManager, CurrentPhaseIndex);
	DOREPLIFETIME(ABBStormManager, PhaseState);
}

UAbilitySystemComponent* ABBStormManager::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABBStormManager::StartStorm()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(AutoStartTimerHandle);

	if (PhaseConfigs.Num() == 0)
	{
		UE_LOG(LogBreachborne, Error, TEXT("StormManager: Cannot start storm — no phase configs!"));
		return;
	}

	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT("=== STORM STARTED === Shift: %s, Phases: %d"),
		ActiveShift ? *ActiveShift->GetShiftName().ToString() : TEXT("None"), PhaseConfigs.Num());

	AdvanceToPhase(0);
}

void ABBStormManager::ResetStorm()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(DamageTickTimerHandle);
	GetWorldTimerManager().ClearTimer(AutoStartTimerHandle);

	if (!StormShiftClass)
	{
		StormShiftClass = UStormShift_Default::StaticClass();
	}
	ActiveShift = NewObject<UStormShiftBase>(this, StormShiftClass);
	PhaseConfigs = ActiveShift ? ActiveShift->GetPhaseConfigs(InitialRadius) : TArray<FStormPhaseConfig>();

	CurrentCenter = InitialCenter;
	CurrentRadius = InitialRadius;
	TargetCenter = InitialCenter;
	TargetRadius = InitialRadius;
	CurrentPhaseIndex = 0;
	PhaseState = EStormPhaseState::Waiting;
	ShrinkStartRadius = InitialRadius;
	ShrinkStartCenter = InitialCenter;
	PhaseElapsedTime = 0.0f;

	ForceNetUpdate();

	UE_LOG(LogBreachborne, Warning, TEXT("StormManager: reset for new match. Radius: %.0f | Phases: %d"),
		InitialRadius, PhaseConfigs.Num());
}

void ABBStormManager::AdvanceToPhase(int32 PhaseIndex)
{
	if (!PhaseConfigs.IsValidIndex(PhaseIndex))
	{
		// All phases complete
		PhaseState = EStormPhaseState::Finished;
		UE_LOG(LogBreachborne, Warning, TEXT("=== STORM FINISHED === All phases complete. Damage continues at final rate."));
		return;
	}

	CurrentPhaseIndex = PhaseIndex;
	const FStormPhaseConfig& Config = PhaseConfigs[PhaseIndex];

	// Calculate target radius from fraction of initial radius
	TargetRadius = InitialRadius * Config.TargetRadiusFraction;

	// Ask the shift where the next center should be
	if (ActiveShift)
	{
		TargetCenter = ActiveShift->PickNextCenter(PhaseIndex, CurrentCenter, TargetRadius);
		ActiveShift->OnPhaseStarted(PhaseIndex);
	}

	// Store starting values for lerp
	ShrinkStartRadius = CurrentRadius;
	ShrinkStartCenter = CurrentCenter;
	PhaseElapsedTime = 0.0f;
	PhaseState = EStormPhaseState::Shrinking;

	// Start damage timer at this phase's interval
	StartDamageTimer(Config.DamageTickInterval);

	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT("=== STORM PHASE %d === Radius: %.0f -> %.0f | Shrink: %.0fs | Hold: %.0fs | Dmg: %.0f/%.1fs"),
		PhaseIndex + 1, ShrinkStartRadius, TargetRadius,
		Config.ShrinkDuration, Config.HoldDuration,
		Config.DamagePerTick, Config.DamageTickInterval);
}

void ABBStormManager::OnPhaseHoldComplete()
{
	AdvanceToPhase(CurrentPhaseIndex + 1);
}

void ABBStormManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		// Server: drive phase progression
		if (PhaseState == EStormPhaseState::Shrinking)
		{
			if (!PhaseConfigs.IsValidIndex(CurrentPhaseIndex))
			{
				return;
			}

			const FStormPhaseConfig& Config = PhaseConfigs[CurrentPhaseIndex];
			PhaseElapsedTime += DeltaTime;

			const float Alpha = (Config.ShrinkDuration > 0.0f)
				? FMath::Clamp(PhaseElapsedTime / Config.ShrinkDuration, 0.0f, 1.0f)
				: 1.0f;

			CurrentRadius = FMath::Lerp(ShrinkStartRadius, TargetRadius, Alpha);
			CurrentCenter = FMath::Lerp(ShrinkStartCenter, TargetCenter, Alpha);

			if (Alpha >= 1.0f)
			{
				// Shrink complete
				CurrentRadius = TargetRadius;
				CurrentCenter = TargetCenter;

				if (Config.HoldDuration > 0.0f)
				{
					PhaseState = EStormPhaseState::Holding;
					PhaseElapsedTime = 0.0f;
					UE_LOG(LogBreachborne, Log, TEXT("Storm: Phase %d shrink complete. Holding for %.0fs."), CurrentPhaseIndex + 1, Config.HoldDuration);
				}
				else
				{
					// No hold — advance immediately
					AdvanceToPhase(CurrentPhaseIndex + 1);
				}
			}
		}
		else if (PhaseState == EStormPhaseState::Holding)
		{
			if (!PhaseConfigs.IsValidIndex(CurrentPhaseIndex))
			{
				return;
			}

			const FStormPhaseConfig& Config = PhaseConfigs[CurrentPhaseIndex];
			PhaseElapsedTime += DeltaTime;

			if (PhaseElapsedTime >= Config.HoldDuration)
			{
				AdvanceToPhase(CurrentPhaseIndex + 1);
			}
		}
		// Waiting and Finished: no progression logic needed
	}

	// Both server and client: draw debug circles
	DrawDebugStormCircles();
}

void ABBStormManager::ApplyStormDamage()
{
	if (!HasAuthority() || !AbilitySystemComponent || !StormDamageGE)
	{
		return;
	}

	// Determine the damage for the current phase
	float Damage = 0.0f;
	if (PhaseConfigs.IsValidIndex(CurrentPhaseIndex))
	{
		Damage = PhaseConfigs[CurrentPhaseIndex].DamagePerTick;
	}
	else if (PhaseConfigs.Num() > 0)
	{
		// Finished state: use last phase's damage
		Damage = PhaseConfigs.Last().DamagePerTick;
	}

	if (ActiveShift)
	{
		Damage = ActiveShift->ModifyDamage(Damage, CurrentPhaseIndex);
	}

	if (Damage <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Iterate all characters in the world and damage those outside the safe zone
	for (TActorIterator<ACharacter> It(World); It; ++It)
	{
		ACharacter* Character = *It;
		if (!Character)
		{
			continue;
		}

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Character);
		if (!ASI)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
		if (!TargetASC)
		{
			continue;
		}

		// Skip dead characters
		if (TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
		{
			continue;
		}

		// 2D distance check — top-down game, height is irrelevant
		const FVector CharLoc = Character->GetActorLocation();
		const float Distance2D = FVector2D::Distance(FVector2D(CharLoc.X, CharLoc.Y), CurrentCenter);

		if (Distance2D <= CurrentRadius)
		{
			continue; // Inside safe zone
		}

		// Outside the circle — apply storm damage via GAS pipeline
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		Context.AddInstigator(this, this);

		// Construct spec directly from the programmatic GE instance
		FGameplayEffectSpec Spec(StormDamageGE, Context, 1.0f);
		Spec.SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, Damage);

		TargetASC->ApplyGameplayEffectSpecToSelf(Spec);

		UE_LOG(LogBreachborne, Verbose, TEXT("Storm: %.1f damage to %s (dist: %.0f, radius: %.0f)"),
			Damage, *Character->GetName(), Distance2D, CurrentRadius);
	}
}

void ABBStormManager::StartDamageTimer(float Interval)
{
	GetWorldTimerManager().ClearTimer(DamageTickTimerHandle);

	if (Interval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(DamageTickTimerHandle, this, &ABBStormManager::ApplyStormDamage, Interval, true);
	}
}

void ABBStormManager::DrawDebugStormCircles() const
{
	if (PhaseState == EStormPhaseState::Waiting)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector CurrentCenter3D(CurrentCenter.X, CurrentCenter.Y, DebugDrawZ);
	const FVector TargetCenter3D(TargetCenter.X, TargetCenter.Y, DebugDrawZ);

	// Current safe zone — green
	if (CurrentRadius > 0.0f)
	{
		DrawDebugCircle(World, CurrentCenter3D, CurrentRadius, 64,
			FColor::Green, false, 0.0f, 0, 3.0f,
			FVector::ForwardVector, FVector::RightVector, false);
	}

	// Target zone — yellow (only if different from current)
	if (PhaseState == EStormPhaseState::Shrinking && TargetRadius != CurrentRadius)
	{
		DrawDebugCircle(World, TargetCenter3D, FMath::Max(TargetRadius, 50.0f), 64,
			FColor::Yellow, false, 0.0f, 0, 2.0f,
			FVector::ForwardVector, FVector::RightVector, false);
	}
}
