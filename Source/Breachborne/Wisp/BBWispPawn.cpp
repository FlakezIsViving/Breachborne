#include "BBWispPawn.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/OverlapResult.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBAllyBot.h"
#include "Breachborne/UI/WispIndicatorWidget.h"
#include "Breachborne/Breachborne.h"

ABBWispPawn::ABBWispPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	// Configure inherited capsule — small and ethereal for the wisp
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (Capsule)
	{
		Capsule->SetCapsuleSize(30.0f, 30.0f);
		Capsule->SetCollisionProfileName(TEXT("Pawn"));
		// Wisps are ethereal — other pawns can walk through them
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	}

	// Proximity detection sphere (rez/stomp)
	ProximitySphere = CreateDefaultSubobject<USphereComponent>(TEXT("ProximitySphere"));
	ProximitySphere->SetupAttachment(RootComponent);
	ProximitySphere->SetSphereRadius(ProximityRadius);
	ProximitySphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	ProximitySphere->SetGenerateOverlapEvents(true);

	// Execute range sphere (E-key interaction)
	ExecuteSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ExecuteSphere"));
	ExecuteSphere->SetupAttachment(RootComponent);
	ExecuteSphere->SetSphereRadius(ExecuteRadius);
	ExecuteSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	ExecuteSphere->SetGenerateOverlapEvents(true);

	// Visual mesh — small glowing sphere
	WispMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WispMesh"));
	WispMesh->SetupAttachment(RootComponent);
	WispMesh->SetRelativeScale3D(FVector(0.5f));
	WispMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded())
	{
		WispMesh->SetStaticMesh(SphereMesh.Object);
	}

	// Configure CharacterMovementComponent for slow wisp walking
	// CMC gives us free client→server movement replication (ServerMove RPCs)
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (CMC)
	{
		CMC->MaxWalkSpeed = WispMoveSpeed;
		CMC->MaxAcceleration = 2000.0f;
		CMC->BrakingDecelerationWalking = 4000.0f;
		CMC->bOrientRotationToMovement = false;
		CMC->GravityScale = 1.0f;
		CMC->JumpZVelocity = 0.0f;
		CMC->AirControl = 0.0f;
		CMC->SetMovementMode(MOVE_Walking);
	}

	// Camera — top-down, same style as hunter
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 2000.0f;
	CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->CameraLagSpeed = 0.0f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Indicator widget (screen-space bars above wisp)
	IndicatorWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("IndicatorWidget"));
	IndicatorWidgetComp->SetupAttachment(RootComponent);
	IndicatorWidgetComp->SetRelativeLocation(FVector(0.0f, 0.0f, 95.0f));
	IndicatorWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	IndicatorWidgetComp->SetDrawSize(FVector2D(150.0f, 46.0f));
	IndicatorWidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	IndicatorWidgetComp->SetVisibility(true);
	IndicatorWidgetComp->SetWidgetClass(UWispIndicatorWidget::StaticClass());

	// Networking
	bReplicates = true;
	SetReplicatingMovement(true);
}

FBBWispTickResult ABBWispPawn::ResolveWispTick(const FBBWispTickInput& Input)
{
	FBBWispTickResult Result;
	Result.HP = FMath::Clamp(Input.CurrentHP, 0.0f, FMath::Max(0.0f, Input.MaxHP));
	Result.Rez = FMath::Clamp(Input.CurrentRez, 0.0f, 1.0f);

	if (Input.bCarried)
	{
		Result.TotalReviveMultiplier = Input.MaxReviveMultiplier;
	}
	else
	{
		Result.ProximityMultiplier = (Input.bAllyNearby || Input.bHealingActive) && !Input.bEnemyNearby ? 1.0f : 0.0f;
		const float ProgressPerBaseTick = Input.RezFillRate * Input.DeltaTime;
		const float HealReviveProgress = FMath::Max(0.0f, Input.HealAmount) * Input.HealToReviveConversion;
		Result.HealMultiplier = ProgressPerBaseTick > 0.0f
			? HealReviveProgress / ProgressPerBaseTick
			: 0.0f;
		Result.TotalReviveMultiplier = Input.bEnemyNearby
			? 0.0f
			: FMath::Min(Result.ProximityMultiplier + Result.HealMultiplier, Input.MaxReviveMultiplier);
	}

	Result.bDecayPaused = Result.TotalReviveMultiplier > 0.0f;
	if (Result.bDecayPaused)
	{
		Result.Rez = FMath::Min(
			Result.Rez + Input.RezFillRate * Input.DeltaTime * Result.TotalReviveMultiplier,
			1.0f);
		return Result;
	}

	Result.Rez = FMath::Max(Result.Rez - Input.RezDecayRate * Input.DeltaTime, 0.0f);
	const float DrainMultiplier = Input.bEnemyNearby ? Input.StompDrainMultiplier : 1.0f;
	Result.HP = FMath::Max(Result.HP - Input.BaseDrainRate * DrainMultiplier * Input.DeltaTime, 0.0f);
	return Result;
}

bool ABBWispPawn::ResolveHealingReviveLatch(bool bWasLatched, bool bReceivedHealing, bool bEnemyNearby)
{
	return !bEnemyNearby && (bWasLatched || bReceivedHealing);
}

void ABBWispPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABBWispPawn, WispState);
	DOREPLIFETIME(ABBWispPawn, RezBarProgress);
	DOREPLIFETIME(ABBWispPawn, ReplicatedWispHP);
	DOREPLIFETIME(ABBWispPawn, ReplicatedMaxWispHP);
	DOREPLIFETIME(ABBWispPawn, OwningPlayerState);
	DOREPLIFETIME(ABBWispPawn, ExecutingHunter);
	DOREPLIFETIME(ABBWispPawn, CarriedBy);
}

void ABBWispPawn::InitWisp(ABreachbornePlayerState* OwnerPS)
{
	if (!HasAuthority() || !OwnerPS)
	{
		return;
	}

	OwningPlayerState = OwnerPS;
	WispState = EWispState::Active;
	RezBarProgress = 0.0f;
	ExecutingHunter = nullptr;
	HealAccumulated = 0.0f;
	bHealingReviveLatched = false;

	// Update proximity sphere radius from tuning constant
	if (ProximitySphere)
	{
		ProximitySphere->SetSphereRadius(ProximityRadius);
	}
	if (ExecuteSphere)
	{
		ExecuteSphere->SetSphereRadius(ExecuteRadius);
	}

	// Update movement speed on CMC
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->MaxWalkSpeed = WispMoveSpeed;
	}

	// Reset wisp HP to max for the decay bar
	ReplicatedWispHP = ReplicatedMaxWispHP;

	// Initialize the indicator widget on all clients
	if (IndicatorWidgetComp)
	{
		IndicatorWidgetComp->SetWidgetClass(UWispIndicatorWidget::StaticClass());
		IndicatorWidgetComp->InitWidget();
		if (UWispIndicatorWidget* WispWidget = Cast<UWispIndicatorWidget>(IndicatorWidgetComp->GetUserWidgetObject()))
		{
			WispWidget->SetTargetWisp(this);
		}
	}

	// Start the server tick timer
	GetWorldTimerManager().SetTimer(WispTickTimerHandle, this, &ABBWispPawn::ServerWispTick,
		WispTickInterval, true);

	UE_LOG(LogBreachborne, Log, TEXT("WispPawn: Initialized for %s | TickTimerActive=%s | Interval=%.2fs"),
		*OwnerPS->GetPlayerName(),
		GetWorldTimerManager().IsTimerActive(WispTickTimerHandle) ? TEXT("YES") : TEXT("NO"),
		WispTickInterval);
}

void ABBWispPawn::ServerBeginExecute(AHunterCharacter* Executor)
{
	if (!HasAuthority() || !Executor)
	{
		return;
	}

	if (WispState != EWispState::Active && WispState != EWispState::BeingRevived)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("WispPawn: Cannot execute — wisp state is %d"),
			static_cast<uint8>(WispState));
		return;
	}

	WispState = EWispState::BeingExecuted;
	ExecutingHunter = Executor;

	// Add State_Executing tag and register damage listener on executor's ASC
	if (ABreachbornePlayerState* ExecPS = Executor->GetPlayerState<ABreachbornePlayerState>())
	{
		if (UAbilitySystemComponent* ExecASC = ExecPS->GetBBAbilitySystemComponent())
		{
			ExecASC->AddLooseGameplayTag(BBGameplayTags::State_Executing);

			ExecutorDamageDelegate = ExecASC->GetGameplayAttributeValueChangeDelegate(
				UBBHealthSet::GetHealthAttribute()).AddUObject(this, &ABBWispPawn::OnExecutorDamaged);
		}
	}

	// Start execute timer
	GetWorldTimerManager().SetTimer(ExecuteTimerHandle, this, &ABBWispPawn::ServerCompleteExecute,
		ExecuteDuration, false);

	UE_LOG(LogBreachborne, Log, TEXT("WispPawn: %s is executing wisp of %s (%.1fs)"),
		*Executor->GetName(),
		OwningPlayerState ? *OwningPlayerState->GetPlayerName() : TEXT("Unknown"),
		ExecuteDuration);
}

void ABBWispPawn::ServerCancelExecute()
{
	if (!HasAuthority())
	{
		return;
	}

	if (WispState != EWispState::BeingExecuted)
	{
		return;
	}

	// Clear execute timer
	GetWorldTimerManager().ClearTimer(ExecuteTimerHandle);

	// Unbind damage listener and remove State_Executing tag from executor
	if (ExecutingHunter)
	{
		if (ABreachbornePlayerState* ExecPS = ExecutingHunter->GetPlayerState<ABreachbornePlayerState>())
		{
			if (UAbilitySystemComponent* ExecASC = ExecPS->GetBBAbilitySystemComponent())
			{
				if (ExecutorDamageDelegate.IsValid())
				{
					ExecASC->GetGameplayAttributeValueChangeDelegate(
						UBBHealthSet::GetHealthAttribute()).Remove(ExecutorDamageDelegate);
					ExecutorDamageDelegate.Reset();
				}
				ExecASC->RemoveLooseGameplayTag(BBGameplayTags::State_Executing);
			}
		}
	}

	WispState = EWispState::Active;
	ExecutingHunter = nullptr;

	UE_LOG(LogBreachborne, Log, TEXT("WispPawn: Execute cancelled on wisp of %s"),
		OwningPlayerState ? *OwningPlayerState->GetPlayerName() : TEXT("Unknown"));
}

void ABBWispPawn::ServerCompleteExecute()
{
	if (!HasAuthority())
	{
		return;
	}

	if (WispState != EWispState::BeingExecuted || !ExecutingHunter)
	{
		return;
	}

	UE_LOG(LogBreachborne, Warning, TEXT("WispPawn: Execute completed! %s executed wisp of %s"),
		*ExecutingHunter->GetName(),
		OwningPlayerState ? *OwningPlayerState->GetPlayerName() : TEXT("Unknown"));

	// Unbind damage listener and remove State_Executing tag from executor
	if (ABreachbornePlayerState* ExecPS = ExecutingHunter->GetPlayerState<ABreachbornePlayerState>())
	{
		if (UAbilitySystemComponent* ExecASC = ExecPS->GetBBAbilitySystemComponent())
		{
			if (ExecutorDamageDelegate.IsValid())
			{
				ExecASC->GetGameplayAttributeValueChangeDelegate(
					UBBHealthSet::GetHealthAttribute()).Remove(ExecutorDamageDelegate);
				ExecutorDamageDelegate.Reset();
			}
			ExecASC->RemoveLooseGameplayTag(BBGameplayTags::State_Executing);
		}
	}

	// Stop wisp tick
	GetWorldTimerManager().ClearTimer(WispTickTimerHandle);

	// Notify GameMode
	OnWispExecuted.Broadcast(this, ExecutingHunter);
}

void ABBWispPawn::ServerKillFromAbyss()
{
	if (!HasAuthority())
	{
		return;
	}

	bDiedFromAbyss = true;

	// Stop wisp tick
	GetWorldTimerManager().ClearTimer(WispTickTimerHandle);
	GetWorldTimerManager().ClearTimer(ExecuteTimerHandle);

	// Broadcast wisp death (not executed)
	OnWispDied.Broadcast(this, false);
}

void ABBWispPawn::SetCarrier(AHunterCharacter* Carrier)
{
	if (Carrier == CarriedBy)
	{
		return;
	}

	if (Carrier)
	{
		CarriedBy = Carrier;
		ApplyCarrierState();
		ForceNetUpdate();
		UE_LOG(LogBreachborne, Log, TEXT("WispPawn: %s picked up by %s"),
			OwningPlayerState ? *OwningPlayerState->GetPlayerName() : TEXT("Unknown"),
			*Carrier->GetName());
	}
	else
	{
		CarriedBy = nullptr;
		ApplyCarrierState();
		ForceNetUpdate();
		UE_LOG(LogBreachborne, Log, TEXT("WispPawn: %s dropped"),
			OwningPlayerState ? *OwningPlayerState->GetPlayerName() : TEXT("Unknown"));
	}
}

void ABBWispPawn::ApplyCarrierState()
{
	if (CarriedBy)
	{
		if (UCharacterMovementComponent* CMC = GetCharacterMovement())
		{
			CMC->StopMovementImmediately();
			CMC->DisableMovement();
		}
		SetActorEnableCollision(false);
		if (HasAuthority())
		{
			SetReplicateMovement(false);
		}
		AttachToActor(CarriedBy, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetActorRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	}
	else
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetActorEnableCollision(true);
		if (HasAuthority())
		{
			SetReplicateMovement(true);
		}
		if (UCharacterMovementComponent* CMC = GetCharacterMovement())
		{
			CMC->SetMovementMode(MOVE_Walking);
		}
	}
}

void ABBWispPawn::OnRep_CarriedBy()
{
	ApplyCarrierState();
}

void ABBWispPawn::ServerWispTick()
{
	UE_LOG(LogBreachborne, Verbose, TEXT("[WISP_TICK] ENTER | Wisp=%s | Auth=%s | State=%d | OwnerPS=%s"),
		*GetName(), HasAuthority() ? TEXT("YES") : TEXT("NO"), static_cast<uint8>(WispState),
		OwningPlayerState ? *OwningPlayerState->GetPlayerName() : TEXT("NULL"));

	if (!HasAuthority() || !OwningPlayerState)
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("[WISP_TICK] EARLY RETURN — NoAuth=%s | NoOwner=%s"),
			!HasAuthority() ? TEXT("YES") : TEXT("NO"), !OwningPlayerState ? TEXT("YES") : TEXT("NO"));
		return;
	}

	// Skip tick during execute — execute timer handles death
	if (WispState == EWispState::BeingExecuted || WispState == EWispState::Revived)
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("[WISP_TICK] SKIP — State=%d (Executed/Revived)"), static_cast<uint8>(WispState));
		return;
	}



	// --- Carried by Eluna: max revive speed, no drain ---
	if (CarriedBy != nullptr)
	{
		const float PrevProgress = RezBarProgress;
		FBBWispTickInput TickInput;
		TickInput.CurrentHP = ReplicatedWispHP;
		TickInput.MaxHP = ReplicatedMaxWispHP;
		TickInput.CurrentRez = RezBarProgress;
		TickInput.DeltaTime = WispTickInterval;
		TickInput.BaseDrainRate = BaseDrainRate;
		TickInput.StompDrainMultiplier = StompDrainMultiplier;
		TickInput.RezFillRate = RezFillRate;
		TickInput.RezDecayRate = RezDecayRate;
		TickInput.HealToReviveConversion = HealToReviveConversion;
		TickInput.MaxReviveMultiplier = MaxReviveMultiplier;
		TickInput.bCarried = true;
		const FBBWispTickResult TickResult = ResolveWispTick(TickInput);
		ReplicatedWispHP = TickResult.HP;
		RezBarProgress = TickResult.Rez;

		UE_LOG(LogBreachborne, Log, TEXT("Wisp: CARRIED — Rez %.1f%% -> %.1f%% (+%.2f%%)"),
			PrevProgress * 100.0f, RezBarProgress * 100.0f, (RezBarProgress - PrevProgress) * 100.0f);

		if (RezBarProgress >= 1.0f)
		{
			const float Elapsed = (ReviveStartTime > 0.0f) ? (GetWorld()->GetTimeSeconds() - ReviveStartTime) : -1.0f;
			UE_LOG(LogBreachborne, Warning, TEXT("Wisp: REVIVE COMPLETE (carried) after %.2fs — triggering revive"), Elapsed);
			WispState = EWispState::Revived;
			GetWorldTimerManager().ClearTimer(WispTickTimerHandle);
			OnWispReviveReady.Broadcast(this);
		}
		return;
	}

	// Detect nearby hunters
	bool bAllyNearby = false;
	bool bEnemyNearby = false;
	float NearestAllyDist = -1.0f;
	FString NearestAllyName;
	const int32 OwnerTeamID = OwningPlayerState->GetTeamID();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	FCollisionShape Sphere = FCollisionShape::MakeSphere(ProximityRadius);
	FCollisionObjectQueryParams ObjectParams(ECC_Pawn);
	GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity,
		ObjectParams, Sphere, QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OverlapActor = Overlap.GetActor();
		if (!OverlapActor)
		{
			continue;
		}

		if (AHunterCharacter* NearbyHunter = Cast<AHunterCharacter>(OverlapActor))
		{
			ABreachbornePlayerState* NearbyPS = NearbyHunter->GetPlayerState<ABreachbornePlayerState>();
			if (!NearbyPS || !NearbyPS->GetIsAlive())
			{
				continue;
			}

			const float Dist = FVector::Dist(GetActorLocation(), NearbyHunter->GetActorLocation());
			if (NearbyPS->GetTeamID() == OwnerTeamID)
			{
				bAllyNearby = true;
				if (NearestAllyDist < 0.0f || Dist < NearestAllyDist)
				{
					NearestAllyDist = Dist;
					NearestAllyName = NearbyHunter->GetName();
				}
			}
			else
			{
				bEnemyNearby = true;
			}
			continue;
		}

		if (ABBAllyBot* AllyBot = Cast<ABBAllyBot>(OverlapActor))
		{
			if (AllyBot->GetTeamID() == OwnerTeamID && AllyBot->GetIsAlive())
			{
				bAllyNearby = true;
				const float Dist = FVector::Dist(GetActorLocation(), AllyBot->GetActorLocation());
				if (NearestAllyDist < 0.0f || Dist < NearestAllyDist)
				{
					NearestAllyDist = Dist;
					NearestAllyName = AllyBot->GetName();
				}
			}
		}
	}

	// Track previous state for transition logging
	const EWispState PrevState = WispState;

	// A heal starts a persistent revival source. Enemy contest clears it; Eluna
	// carry is handled above and deliberately bypasses this branch.
	const bool bReceivedHealing = HealAccumulated > 0.0f;
	bHealingReviveLatched = ResolveHealingReviveLatch(
		bHealingReviveLatched, bReceivedHealing, bEnemyNearby);
	const bool bHasReviveHealing = bHealingReviveLatched;
	const bool bCanRevive = !bEnemyNearby && (bAllyNearby || bHasReviveHealing);
	if (bCanRevive && WispState == EWispState::Active)
	{
		WispState = EWispState::BeingRevived;
		ReviveStartTime = GetWorld()->GetTimeSeconds();
		UE_LOG(LogBreachborne, Warning,
			TEXT("Wisp: REVIVE STARTED | Source=%s | Ally=%s Dist=%.0f | Rate=%.2f/s | EstTime=%.1fs"),
			bAllyNearby ? TEXT("Proximity") : TEXT("Healing"),
			*NearestAllyName, NearestAllyDist, RezFillRate, 1.0f / RezFillRate);
	}
	else if (!bCanRevive && WispState == EWispState::BeingRevived)
	{
		WispState = EWispState::Active;
		const float Elapsed = (ReviveStartTime > 0.0f) ? (GetWorld()->GetTimeSeconds() - ReviveStartTime) : 0.0f;
		ReviveStartTime = -1.0f;
		UE_LOG(LogBreachborne, Warning, TEXT("Wisp: REVIVE INTERRUPTED after %.2fs | RezBar=%.1f%% | Source removed or contested"),
			Elapsed, RezBarProgress * 100.0f);
	}

	if (WispState != PrevState && WispState != EWispState::BeingRevived)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Wisp: State %d -> %d | Ally=%s Dist=%.0f"),
			static_cast<uint8>(PrevState), static_cast<uint8>(WispState),
			bAllyNearby ? TEXT("YES") : TEXT("NO"),
			NearestAllyDist);
	}

	const float PreTickHP = ReplicatedWispHP;
	const float PreTickRez = RezBarProgress;
	const float HealConsumed = HealAccumulated;
	HealAccumulated = 0.0f;

	FBBWispTickInput TickInput;
	TickInput.CurrentHP = ReplicatedWispHP;
	TickInput.MaxHP = ReplicatedMaxWispHP;
	TickInput.CurrentRez = RezBarProgress;
	TickInput.HealAmount = HealConsumed;
	TickInput.DeltaTime = WispTickInterval;
	TickInput.BaseDrainRate = BaseDrainRate;
	TickInput.StompDrainMultiplier = StompDrainMultiplier;
	TickInput.RezFillRate = RezFillRate;
	TickInput.RezDecayRate = RezDecayRate;
	TickInput.HealToReviveConversion = HealToReviveConversion;
	TickInput.MaxReviveMultiplier = MaxReviveMultiplier;
	TickInput.bAllyNearby = bAllyNearby;
	TickInput.bHealingActive = bHasReviveHealing;
	TickInput.bEnemyNearby = bEnemyNearby;
	const FBBWispTickResult TickResult = ResolveWispTick(TickInput);
	ReplicatedWispHP = TickResult.HP;
	RezBarProgress = TickResult.Rez;

	// Comprehensive tick log
	UE_LOG(LogBreachborne, VeryVerbose, TEXT("[WISP_TICK] %s | HP=%.1f -> %.1f/%.1f | Rez=%.1f%% -> %.1f%% | ProxMult=%.1f | Heal=%.1f->Mult=%.2f | TotalMult=%.1f | DecayPaused=%s | Enemy=%s"),
		OwningPlayerState ? *OwningPlayerState->GetPlayerName() : TEXT("Unknown"),
		PreTickHP, ReplicatedWispHP, ReplicatedMaxWispHP,
		PreTickRez * 100.0f, RezBarProgress * 100.0f,
		TickResult.ProximityMultiplier, HealConsumed, TickResult.HealMultiplier,
		TickResult.TotalReviveMultiplier,
		TickResult.bDecayPaused ? TEXT("YES") : TEXT("NO"),
		bEnemyNearby ? TEXT("YES") : TEXT("NO"));

	// Check for revive completion
	if (RezBarProgress >= 1.0f)
	{
		const float Elapsed = (ReviveStartTime > 0.0f) ? (GetWorld()->GetTimeSeconds() - ReviveStartTime) : -1.0f;
		UE_LOG(LogBreachborne, Warning, TEXT("Wisp: REVIVE COMPLETE after %.2fs — triggering revive"), Elapsed);
		WispState = EWispState::Revived;
		GetWorldTimerManager().ClearTimer(WispTickTimerHandle);
		OnWispReviveReady.Broadcast(this);
		return;
	}

	// Check for death after potential drain
	if (ReplicatedWispHP <= 0.0f)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Wisp: HP DEPLETED — wisp died"));
		GetWorldTimerManager().ClearTimer(WispTickTimerHandle);
		OnWispDied.Broadcast(this, false);
	}

	UE_LOG(LogBreachborne, Verbose, TEXT("[WISP_TICK] EXIT | Wisp=%s | Rez=%.1f%% | HP=%.1f/%.1f"),
		*GetName(), RezBarProgress * 100.0f, ReplicatedWispHP, ReplicatedMaxWispHP);
}

void ABBWispPawn::OnExecutorDamaged(const FOnAttributeChangeData& Data)
{
	// If the executor's health decreased while executing, cancel the execute
	if (WispState == EWispState::BeingExecuted && Data.NewValue < Data.OldValue)
	{
		UE_LOG(LogBreachborne, Log, TEXT("WispPawn: Executor took damage during execute — cancelling"));
		ServerCancelExecute();
	}
}

void ABBWispPawn::BeginPlay()
{
	Super::BeginPlay();

	// Bind the indicator widget to this wisp (runs on all clients + server)
	if (IndicatorWidgetComp)
	{
		if (UWispIndicatorWidget* WispWidget = Cast<UWispIndicatorWidget>(IndicatorWidgetComp->GetUserWidgetObject()))
		{
			WispWidget->SetTargetWisp(this);
			UE_LOG(LogBreachborne, Log, TEXT("[WISP_UI] Widget bound in BeginPlay for %s | NetMode=%s | Widget=%s"),
				*GetName(),
				HasAuthority() ? TEXT("Server") : TEXT("Client"),
				*WispWidget->GetName());
		}
		else
		{
			UE_LOG(LogBreachborne, Warning, TEXT("[WISP_UI] BeginPlay: WidgetComponent has no UserWidgetObject for %s | NetMode=%s"),
				*GetName(), HasAuthority() ? TEXT("Server") : TEXT("Client"));
		}
	}
	else
	{
		UE_LOG(LogBreachborne, Warning, TEXT("[WISP_UI] BeginPlay: No IndicatorWidgetComp on %s"), *GetName());
	}
}

void ABBWispPawn::ApplyHeal(float HealAmount)
{
	if (!HasAuthority() || HealAmount <= 0.0f)
	{
		return;
	}

	HealAccumulated += HealAmount;
	UE_LOG(LogBreachborne, Log, TEXT("[WISP_HEAL] Applied %.1f | Accumulated=%.1f | Wisp=%s"),
		HealAmount, HealAccumulated,
		OwningPlayerState ? *OwningPlayerState->GetPlayerName() : TEXT("Unknown"));
}

void ABBWispPawn::OnRep_WispHP()
{
	UE_LOG(LogBreachborne, Verbose, TEXT("[WISP_UI] OnRep_WispHP | HP=%.1f/%.1f | Wisp=%s"),
		ReplicatedWispHP, ReplicatedMaxWispHP, *GetName());
}

void ABBWispPawn::OnRep_WispState()
{
	UE_LOG(LogBreachborne, Verbose, TEXT("WispPawn: State replicated to %d"), static_cast<uint8>(WispState));
}
