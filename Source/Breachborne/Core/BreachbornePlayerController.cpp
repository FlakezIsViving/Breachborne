#include "BreachbornePlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Characters/GliderComponent.h"
#include "Breachborne/Characters/BBMantleComponent.h"
#include "Breachborne/Core/BreachborneTypes.h"
#include "Breachborne/Core/BBGameInstance.h"
#include "Breachborne/Core/BBNetworkDevSettings.h"
#include "Breachborne/Core/BreachborneGameMode.h"
#include "Breachborne/Core/BreachborneGameState.h"
#include "Engine/OverlapResult.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayAbility.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Items/BBInventoryManager.h"
#include "Breachborne/Items/BBEquipmentDefinition.h"
#include "Breachborne/Items/BBConsumableDefinition.h"
#include "Breachborne/Items/BBWorldItem.h"
#include "Breachborne/Items/BBWorldItemSpawner.h"
#include "Breachborne/PvE/BBBasecampActor.h"
#include "Breachborne/Wisp/BBWispPawn.h"
#include "Breachborne/Wisp/BBDeathboxActor.h"
#include "Breachborne/UI/BBCustomLobbyWidget.h"
#include "Breachborne/UI/BBHunterSelectWidget.h"
#include "Breachborne/UI/BBPostMatchWidget.h"
#include "Breachborne/Breachborne.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ABreachbornePlayerController::ABreachbornePlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultMappingContextFinder(TEXT("/Game/Input/IMC_Default.IMC_Default"));
	if (DefaultMappingContextFinder.Succeeded())
	{
		DefaultMappingContext = DefaultMappingContextFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(TEXT("/Game/Input/IA_Move.IA_Move"));
	if (MoveActionFinder.Succeeded())
	{
		MoveAction = MoveActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> JumpActionFinder(TEXT("/Game/Input/IA_Jump.IA_Jump"));
	if (JumpActionFinder.Succeeded())
	{
		JumpAction = JumpActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> PrimaryFireActionFinder(TEXT("/Game/Input/IA_PrimaryFire.IA_PrimaryFire"));
	if (PrimaryFireActionFinder.Succeeded())
	{
		PrimaryFireAction = PrimaryFireActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> SecondaryFireActionFinder(TEXT("/Game/Input/IA_SecondaryFire.IA_SecondaryFire"));
	if (SecondaryFireActionFinder.Succeeded())
	{
		SecondaryFireAction = SecondaryFireActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> DashActionFinder(TEXT("/Game/Input/IA_Dash.IA_Dash"));
	if (DashActionFinder.Succeeded())
	{
		DashAction = DashActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> AbilityQActionFinder(TEXT("/Game/Input/IA_Ability.IA_Ability"));
	if (AbilityQActionFinder.Succeeded())
	{
		AbilityQAction = AbilityQActionFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> UltimateActionFinder(TEXT("/Game/Input/IA_Ultimate.IA_Ultimate"));
	if (UltimateActionFinder.Succeeded())
	{
		UltimateAction = UltimateActionFinder.Object;
	}
}

void ABreachbornePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Add input mapping context (client only — subsystem is local player only)
	if (IsLocalController() && DefaultMappingContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
			UE_LOG(LogBreachborne, Log, TEXT("Input: Added mapping context %s"), *GetNameSafe(DefaultMappingContext));
		}
	}
	else if (IsLocalController())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("Input: DefaultMappingContext is missing on %s"), *GetNameSafe(this));
	}

	if (IsLocalController())
	{
		if (UBBGameInstance* GI = GetGameInstance<UBBGameInstance>())
		{
			GI->HandleLocalPlayerControllerReady(this);
		}
		InitializeAbilitySmoke();
	}
}

void ABreachbornePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	EndAbilityRangePreview();

	UE_LOG(LogBreachborne, Log, TEXT("PlayerController possessed pawn: %s"), InPawn ? *InPawn->GetName() : TEXT("nullptr"));
}

void ABreachbornePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction)
		{
			EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABreachbornePlayerController::HandleMoveInput);
		}

		if (JumpAction)
		{
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ABreachbornePlayerController::HandleJumpStarted);
			EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ABreachbornePlayerController::HandleJumpStopped);
		}

		// --- Ability Input Bindings ---

		if (PrimaryFireAction)
		{
			// LMB: Started (activate) + Completed (release/cancel) for held auto-fire
			EnhancedInput->BindAction(PrimaryFireAction, ETriggerEvent::Started, this, &ABreachbornePlayerController::HandlePrimaryFireStarted);
			EnhancedInput->BindAction(PrimaryFireAction, ETriggerEvent::Completed, this, &ABreachbornePlayerController::HandlePrimaryFireCompleted);
		}

		if (SecondaryFireAction)
		{
			EnhancedInput->BindAction(SecondaryFireAction, ETriggerEvent::Started, this, &ABreachbornePlayerController::HandleSecondaryFireStarted);
			EnhancedInput->BindAction(SecondaryFireAction, ETriggerEvent::Completed, this, &ABreachbornePlayerController::HandleSecondaryFireCompleted);
		}

		if (DashAction)
		{
			EnhancedInput->BindAction(DashAction, ETriggerEvent::Started, this, &ABreachbornePlayerController::HandleDashStarted);
		}

		if (AbilityQAction)
		{
			EnhancedInput->BindAction(AbilityQAction, ETriggerEvent::Started, this, &ABreachbornePlayerController::HandleAbilityQStarted);
		}

		if (UltimateAction)
		{
			EnhancedInput->BindAction(UltimateAction, ETriggerEvent::Started, this, &ABreachbornePlayerController::HandleUltimateStarted);
		}

		if (Power1Action)
		{
			EnhancedInput->BindAction(Power1Action, ETriggerEvent::Started, this, &ABreachbornePlayerController::HandlePower1Started);
		}
		else
		{
			InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ABreachbornePlayerController::HandlePower1KeyPressed);
		}

		if (Power2Action)
		{
			EnhancedInput->BindAction(Power2Action, ETriggerEvent::Started, this, &ABreachbornePlayerController::HandlePower2Started);
		}
		else
		{
			InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ABreachbornePlayerController::HandlePower2KeyPressed);
		}

		if (InteractAction)
		{
			EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ABreachbornePlayerController::HandleInteractStarted);
		}
		else
		{
			InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ABreachbornePlayerController::HandleInteractKeyPressed);
		}

		if (RecallAction)
		{
			EnhancedInput->BindAction(RecallAction, ETriggerEvent::Started, this, &ABreachbornePlayerController::HandleRecallStarted);
		}
		else
		{
			InputComponent->BindKey(EKeys::B, IE_Pressed, this, &ABreachbornePlayerController::HandleRecallKeyPressed);
		}

		// Consumable hotkeys are hard-bound for the basecamp test pass. Move these
		// back to Enhanced Input actions when the keybind UI exists.
		InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ABreachbornePlayerController::HandleConsumable1KeyPressed);
		InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ABreachbornePlayerController::HandleConsumable2KeyPressed);
		InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ABreachbornePlayerController::HandleConsumable3KeyPressed);
		InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ABreachbornePlayerController::HandleConsumable4KeyPressed);
		InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ABreachbornePlayerController::HandleCancelTargetingKeyPressed);
		InputComponent->BindKey(EKeys::M, IE_Pressed, this, &ABreachbornePlayerController::HandleMapToggleKeyPressed);
	}
}

void ABreachbornePlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABreachbornePlayerController, bBasecampRecallActive);
	DOREPLIFETIME(ABreachbornePlayerController, BasecampRecallProgress);
	DOREPLIFETIME(ABreachbornePlayerController, ActiveRecallBasecamp);
}

void ABreachbornePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (IsLocalController())
	{
		if (bAbilitySmokeEnabled)
		{
			UpdateAbilitySmoke(DeltaTime);
			UpdateDeathSmoke(DeltaTime);
			UpdateHitSmoke(DeltaTime);
		}
		else
		{
			UpdateCursorAim(DeltaTime);
		}
		UpdateGliderFromHeldJump();
		UpdateTacticalNukeTargeting(DeltaTime);
		UpdateAbilityRangePreview();
	}
}

void ABreachbornePlayerController::InitializeAbilitySmoke()
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("BBAbilitySmoke")))
	{
		return;
	}

	FParse::Value(FCommandLine::Get(), TEXT("BBAbilitySmokeIndex="), AbilitySmokeIndex);
	FParse::Value(FCommandLine::Get(), TEXT("BBAbilitySmokeHunter="), AbilitySmokeHunterID);
	if (AbilitySmokeIndex < 1 || AbilitySmokeIndex > 2 || AbilitySmokeHunterID < 1 || AbilitySmokeHunterID > 6)
	{
		UE_LOG(LogBreachborne, Error,
			TEXT("BB_ABILITY_SMOKE|CONFIG_FAIL|index=%d hunter=%d"),
			AbilitySmokeIndex, AbilitySmokeHunterID);
		return;
	}

	bAbilitySmokeEnabled = true;
	bDeathSmokeEnabled = FParse::Param(FCommandLine::Get(), TEXT("BBDeathSmoke"));
	bHitSmokeEnabled = FParse::Param(FCommandLine::Get(), TEXT("BBHitSmoke"));
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_ABILITY_SMOKE|CONFIG|index=%d hunter=%d death=%d hit=%d"),
		AbilitySmokeIndex, AbilitySmokeHunterID, bDeathSmokeEnabled ? 1 : 0,
		bHitSmokeEnabled ? 1 : 0);
}

void ABreachbornePlayerController::UpdateDeathSmoke(float DeltaTime)
{
	if (!bDeathSmokeEnabled || bDeathSmokeObserved)
	{
		return;
	}

	if (AbilitySmokeIndex == 1 && bAbilitySmokeComplete && !bDeathSmokeTriggered)
	{
		DeathSmokeTriggerDelay += DeltaTime;
		if (DeathSmokeTriggerDelay >= 1.5f)
		{
			bDeathSmokeTriggered = true;
			bDeathSmokeObserved = true;
			ServerTriggerDeathSmoke();
			UE_LOG(LogBreachborne, Warning, TEXT("BB_DEATH_SMOKE|CLIENT_TRIGGER|index=1"));
		}
		return;
	}

	if (AbilitySmokeIndex == 2)
	{
		if (ABBWispPawn* Wisp = Cast<ABBWispPawn>(GetPawn()))
		{
			if (!bDeathSmokeWispObserved)
			{
				bDeathSmokeWispObserved = true;
				UE_LOG(LogBreachborne, Warning,
					TEXT("BB_DEATH_SMOKE|CLIENT_WISP|index=2 pawn=%s owner=%s hp=%.1f rez=%.3f"),
					*GetNameSafe(Wisp), *GetNameSafe(Wisp->GetOwningPlayerState()),
					Wisp->GetCurrentWispHP(), Wisp->GetRezBarProgress());
			}
			if (!bDeathSmokeHealRequested)
			{
				bDeathSmokeHealRequested = true;
				ServerBeginWispHealSmoke();
				UE_LOG(LogBreachborne, Warning, TEXT("BB_DEATH_SMOKE|CLIENT_HEAL_REQUEST|index=2"));
			}
		}
		else if (bDeathSmokeWispObserved)
		{
			ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
			if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn()); Hunter && PS && PS->GetIsAlive())
			{
				bDeathSmokeObserved = true;
				const UBBHealthSet* HealthSet = PS->GetHealthSet();
				UE_LOG(LogBreachborne, Warning,
					TEXT("BB_DEATH_SMOKE|CLIENT_REVIVED|index=2 pawn=%s health=%.1f alive=1"),
					*GetNameSafe(Hunter), HealthSet ? HealthSet->GetHealth() : -1.0f);
			}
		}
	}
}

void ABreachbornePlayerController::UpdateHitSmoke(float DeltaTime)
{
	if (!bHitSmokeEnabled || !bAbilitySmokeComplete || bHitSmokeReported)
	{
		return;
	}

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn());
	UBBAbilitySystemComponent* ASC = GetBBASC();
	if (!Hunter || !ASC)
	{
		return;
	}

	HitSmokeElapsed += DeltaTime;
	HitSmokeAimAccumulator += DeltaTime;
	if (HitSmokeAimAccumulator >= 0.05f)
	{
		HitSmokeAimAccumulator = 0.0f;
		const float TargetX = AbilitySmokeIndex == 1 ? 100.0f : -100.0f;
		Hunter->ServerSetAimLocation(FVector(TargetX, 0.0f, Hunter->GetActorLocation().Z));
	}

	if (AbilitySmokeIndex == 1 && !bHitSmokePrepared && HitSmokeElapsed >= 0.5f)
	{
		bHitSmokePrepared = true;
		ServerPrepareHitSmoke();
	}

	const float FireTime = AbilitySmokeIndex == 1 ? 1.5f : 5.0f;
	if (!bHitSmokeFired && HitSmokeElapsed >= FireTime)
	{
		bHitSmokeFired = true;
		ActivateAbilitySmokeInput(BBGameplayTags::InputTag_LMB, TEXT("HIT_LMB"));
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_HIT_SMOKE|CLIENT_FIRE|index=%d hunter=%d"),
			AbilitySmokeIndex, AbilitySmokeHunterID);
	}
	if (bHitSmokeFired && !bHitSmokeReleased && HitSmokeElapsed >= FireTime + 1.0f)
	{
		bHitSmokeReleased = true;
		ASC->InputTagReleased(BBGameplayTags::InputTag_LMB);
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_HIT_SMOKE|CLIENT_RELEASE|index=%d hunter=%d"),
			AbilitySmokeIndex, AbilitySmokeHunterID);
	}
	if (bHitSmokeReleased && HitSmokeElapsed >= FireTime + 2.0f)
	{
		bHitSmokeReported = true;
		ServerReportHitSmoke(AbilitySmokeIndex);
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_HIT_SMOKE|CLIENT_COMPLETE|index=%d hunter=%d"),
			AbilitySmokeIndex, AbilitySmokeHunterID);
	}
}

void ABreachbornePlayerController::UpdateAbilitySmoke(float DeltaTime)
{
	if (!bAbilitySmokeEnabled || bAbilitySmokeComplete || !GetWorld())
	{
		return;
	}

	AbilitySmokeElapsed += DeltaTime;
	AbilitySmokePhaseElapsed += DeltaTime;
	AbilitySmokeLobbyRetry -= DeltaTime;
	AbilitySmokeActionDelay -= DeltaTime;

	ABreachborneGameState* GS = GetWorld()->GetGameState<ABreachborneGameState>();
	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!GS || !PS)
	{
		return;
	}

	const EMatchPhase Phase = GS->GetMatchPhase();
	if (Phase != AbilitySmokeLastPhase)
	{
		AbilitySmokeLastPhase = Phase;
		AbilitySmokePhaseElapsed = 0.0f;
		AbilitySmokeLobbyRetry = 0.0f;
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_ABILITY_SMOKE|PHASE|index=%d hunter=%d phase=%d"),
			AbilitySmokeIndex, AbilitySmokeHunterID, static_cast<int32>(Phase));
	}

	if (Phase == EMatchPhase::WaitingForPlayers)
	{
		const int32 DesiredTeam = AbilitySmokeIndex - 1;
		if (AbilitySmokeLobbyRetry <= 0.0f)
		{
			if (PS->GetTeamID() != DesiredTeam || PS->GetLobbySlotIndex() != 0)
			{
				RequestJoinLobbySlot(DesiredTeam, 0);
			}
			else if (AbilitySmokeIndex == 1 && AbilitySmokePhaseElapsed >= 2.5f
				&& GS->GetLobbyActivePlayerCount() >= 2)
			{
				RequestStartLobbyMatch();
			}
			AbilitySmokeLobbyRetry = 0.75f;
		}
		return;
	}

	if (Phase == EMatchPhase::HunterSelect)
	{
		if (AbilitySmokeLobbyRetry <= 0.0f)
		{
			if (PS->GetHunterID() != AbilitySmokeHunterID)
			{
				RequestHunterSelection(AbilitySmokeHunterID);
			}
			else if (!PS->IsReadyForMatch())
			{
				RequestReadyState(true);
			}
			AbilitySmokeLobbyRetry = 0.5f;
		}
		return;
	}

	if (Phase != EMatchPhase::Playing || !PS->GetIsAlive())
	{
		return;
	}

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn());
	UBBAbilitySystemComponent* ASC = GetBBASC();
	if (!Hunter)
	{
		return;
	}

	if (!bAbilitySmokePrepared)
	{
		bAbilitySmokePrepared = true;
		ServerPrepareAbilitySmoke(AbilitySmokeIndex);
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_ABILITY_SMOKE|PREPARE_REQUEST|index=%d hunter=%d"),
			AbilitySmokeIndex, AbilitySmokeHunterID);
	}

	if (!ASC || ASC->GetActivatableAbilities().Num() < 5)
	{
		return;
	}

	if (!bAbilitySmokeReadyLogged)
	{
		bAbilitySmokeReadyLogged = true;
		AbilitySmokeActionDelay = 2.0f;
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_ABILITY_SMOKE|READY|index=%d hunter=%d abilities=%d"),
			AbilitySmokeIndex, AbilitySmokeHunterID, ASC->GetActivatableAbilities().Num());
	}

	AbilitySmokeAimAccumulator += DeltaTime;
	if (AbilitySmokeAimAccumulator >= 0.05f)
	{
		AbilitySmokeAimAccumulator = 0.0f;
		// Fire away from the other smoke client so enemy CC cannot invalidate later activation checks.
		const float TargetX = AbilitySmokeIndex == 1 ? -1700.0f : 1700.0f;
		Hunter->ServerSetAimLocation(FVector(TargetX, 0.0f, Hunter->GetActorLocation().Z));
	}

	if (AbilitySmokeActionDelay > 0.0f)
	{
		return;
	}

	switch (AbilitySmokeStep++)
	{
	case 0:
		ActivateAbilitySmokeInput(BBGameplayTags::InputTag_LMB, TEXT("LMB_PRESS"));
		AbilitySmokeActionDelay = 1.0f;
		break;
	case 1:
		ASC->InputTagReleased(BBGameplayTags::InputTag_LMB);
		UE_LOG(LogBreachborne, Warning, TEXT("BB_ABILITY_SMOKE|RELEASE|index=%d hunter=%d input=LMB"), AbilitySmokeIndex, AbilitySmokeHunterID);
		AbilitySmokeActionDelay = 0.5f;
		break;
	case 2:
		ActivateAbilitySmokeInput(BBGameplayTags::InputTag_RMB, TEXT("RMB_PRESS"));
		AbilitySmokeActionDelay = 1.5f;
		break;
	case 3:
		ASC->InputTagReleased(BBGameplayTags::InputTag_RMB);
		UE_LOG(LogBreachborne, Warning, TEXT("BB_ABILITY_SMOKE|RELEASE|index=%d hunter=%d input=RMB"), AbilitySmokeIndex, AbilitySmokeHunterID);
		AbilitySmokeActionDelay = 0.75f;
		break;
	case 4:
		ActivateAbilitySmokeInput(BBGameplayTags::InputTag_Shift, TEXT("SHIFT"));
		AbilitySmokeActionDelay = 1.0f;
		break;
	case 5:
		ActivateAbilitySmokeInput(BBGameplayTags::InputTag_Q, TEXT("Q_PRESS"));
		AbilitySmokeActionDelay = 1.0f;
		break;
	case 6:
		ActivateAbilitySmokeInput(BBGameplayTags::InputTag_Q, TEXT("Q_REPRESS"));
		AbilitySmokeActionDelay = 1.0f;
		break;
	case 7:
		ActivateAbilitySmokeInput(BBGameplayTags::InputTag_R, TEXT("R"));
		AbilitySmokeActionDelay = 4.0f;
		break;
	default:
		ASC->InputTagReleased(BBGameplayTags::InputTag_LMB);
		ASC->InputTagReleased(BBGameplayTags::InputTag_RMB);
		ASC->CancelAllAbilities();
		bAbilitySmokeComplete = true;
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_ABILITY_SMOKE|COMPLETE|index=%d hunter=%d abilities=%d elapsed=%.2f"),
			AbilitySmokeIndex, AbilitySmokeHunterID, ASC->GetActivatableAbilities().Num(), AbilitySmokeElapsed);
		break;
	}
}

bool ABreachbornePlayerController::ActivateAbilitySmokeInput(const FGameplayTag& InputTag, const TCHAR* ActionName)
{
	UBBAbilitySystemComponent* ASC = GetBBASC();
	const bool bActivated = ASC && ASC->TryActivateAbilityByInputTag(InputTag);
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_ABILITY_SMOKE|ACTIVATE|index=%d hunter=%d action=%s tag=%s success=%d"),
		AbilitySmokeIndex, AbilitySmokeHunterID, ActionName, *InputTag.ToString(), bActivated ? 1 : 0);
	return bActivated;
}

void ABreachbornePlayerController::ServerPrepareAbilitySmoke_Implementation(int32 SmokeIndex)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("BBAbilitySmoke")) || SmokeIndex < 1 || SmokeIndex > 2)
	{
		return;
	}

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn());
	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!Hunter || !PS || !PS->GetIsAlive())
	{
		return;
	}
	AbilitySmokeServerIndex = SmokeIndex;

	const float X = SmokeIndex == 1 ? -350.0f : 350.0f;
	const FVector TraceStart(X, 0.0f, 5000.0f);
	const FVector TraceEnd(X, 0.0f, -5000.0f);
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BBAbilitySmokeGround), false, Hunter);
	const bool bGroundHit = GetWorld()->LineTraceSingleByChannel(
		GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	const float GroundZ = bGroundHit ? GroundHit.ImpactPoint.Z + 110.0f : 200.0f;
	const FVector TargetLocation(X, 0.0f, GroundZ);
	Hunter->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Movement = Hunter->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Walking);
	}
	if (UBBHealthSet* HealthSet = PS->GetHealthSet())
	{
		HealthSet->InitMaxHealth(50000.0f);
		HealthSet->InitHealth(50000.0f);
		PS->UpdateHealthProxy();
	}
	Hunter->ForceNetUpdate();
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_ABILITY_SMOKE|SERVER_PREPARED|index=%d hunter=%d location=%s ground=%d"),
		SmokeIndex, PS->GetHunterID(), *TargetLocation.ToCompactString(), bGroundHit ? 1 : 0);
}

void ABreachbornePlayerController::ServerPrepareHitSmoke_Implementation()
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("BBAbilitySmoke"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("BBHitSmoke"))
		|| AbilitySmokeServerIndex != 1 || !GetWorld())
	{
		return;
	}

	int32 PreparedCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABreachbornePlayerController* SmokeController = Cast<ABreachbornePlayerController>(It->Get());
		if (!SmokeController || SmokeController->AbilitySmokeServerIndex < 1
			|| SmokeController->AbilitySmokeServerIndex > 2)
		{
			continue;
		}

		AHunterCharacter* SmokeHunter = Cast<AHunterCharacter>(SmokeController->GetPawn());
		ABreachbornePlayerState* SmokePS = SmokeController->GetPlayerState<ABreachbornePlayerState>();
		if (!SmokeHunter || !SmokePS || !SmokePS->GetIsAlive())
		{
			continue;
		}

		const float X = SmokeController->AbilitySmokeServerIndex == 1 ? -100.0f : 100.0f;
		const FVector TraceStart(X, 0.0f, 5000.0f);
		const FVector TraceEnd(X, 0.0f, -5000.0f);
		FHitResult GroundHit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BBHitSmokeGround), false, SmokeHunter);
		const bool bGroundHit = GetWorld()->LineTraceSingleByChannel(
			GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
		const float GroundZ = bGroundHit ? GroundHit.ImpactPoint.Z + 110.0f : 200.0f;
		SmokeHunter->SetActorLocation(FVector(X, 0.0f, GroundZ), false, nullptr, ETeleportType::TeleportPhysics);
		if (UCharacterMovementComponent* Movement = SmokeHunter->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Walking);
		}
		if (UBBHealthSet* HealthSet = SmokePS->GetHealthSet())
		{
			HealthSet->SetShield(0.0f);
			HealthSet->SetHealth(50000.0f);
			SmokePS->UpdateHealthProxy();
		}
		SmokeHunter->ForceNetUpdate();
		++PreparedCount;
	}

	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_HIT_SMOKE|SERVER_PREPARED|players=%d separation=200"), PreparedCount);
}

void ABreachbornePlayerController::ServerReportHitSmoke_Implementation(int32 AttackerIndex)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("BBAbilitySmoke"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("BBHitSmoke"))
		|| AttackerIndex != AbilitySmokeServerIndex || AttackerIndex < 1 || AttackerIndex > 2
		|| !GetWorld())
	{
		return;
	}

	ABreachbornePlayerController* VictimController = nullptr;
	const int32 VictimIndex = AttackerIndex == 1 ? 2 : 1;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABreachbornePlayerController* Candidate = Cast<ABreachbornePlayerController>(It->Get());
		if (Candidate && Candidate->AbilitySmokeServerIndex == VictimIndex)
		{
			VictimController = Candidate;
			break;
		}
	}

	ABreachbornePlayerState* VictimPS = VictimController
		? VictimController->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	UBBHealthSet* VictimHealth = VictimPS ? VictimPS->GetHealthSet() : nullptr;
	if (!VictimPS || !VictimHealth)
	{
		UE_LOG(LogBreachborne, Error,
			TEXT("BB_HIT_SMOKE|SERVER_REPORT_FAIL|attacker=%d reason=missing_victim"), AttackerIndex);
		return;
	}

	const float HealthAfter = VictimHealth->GetHealth();
	const float Damage = FMath::Max(0.0f, 50000.0f - HealthAfter);
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_HIT_SMOKE|SERVER_REPORT|attacker=%d hunter=%d victim=%d damage=%.2f health=%.2f success=%d"),
		AttackerIndex, GetPlayerState<ABreachbornePlayerState>()
			? GetPlayerState<ABreachbornePlayerState>()->GetHunterID() : -1,
		VictimIndex, Damage, HealthAfter, Damage > 0.0f ? 1 : 0);

	VictimHealth->SetHealth(50000.0f);
	VictimHealth->SetShield(0.0f);
	VictimPS->UpdateHealthProxy();
}

void ABreachbornePlayerController::ServerTriggerDeathSmoke_Implementation()
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("BBAbilitySmoke"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("BBDeathSmoke"))
		|| AbilitySmokeServerIndex != 1 || !GetWorld())
	{
		return;
	}

	ABreachbornePlayerController* VictimController = nullptr;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABreachbornePlayerController* Candidate = Cast<ABreachbornePlayerController>(It->Get());
		if (Candidate && Candidate->AbilitySmokeServerIndex == 2)
		{
			VictimController = Candidate;
			break;
		}
	}

	UBBAbilitySystemComponent* KillerASC = GetBBASC();
	UBBAbilitySystemComponent* VictimASC = VictimController ? VictimController->GetBBASC() : nullptr;
	ABreachbornePlayerState* VictimPS = VictimController
		? VictimController->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	UBBHealthSet* VictimHealth = VictimPS ? VictimPS->GetHealthSet() : nullptr;
	if (!KillerASC || !VictimASC || !VictimPS || !VictimHealth || !VictimPS->GetIsAlive())
	{
		UE_LOG(LogBreachborne, Error,
			TEXT("BB_DEATH_SMOKE|SERVER_FAIL|killer=%s victim=%s asc=%s health=%s alive=%d"),
			*GetNameSafe(this), *GetNameSafe(VictimController), *GetNameSafe(VictimASC),
			*GetNameSafe(VictimHealth), VictimPS && VictimPS->GetIsAlive() ? 1 : 0);
		return;
	}

	const float HealthBefore = VictimHealth->GetHealth();
	FGameplayEffectContextHandle Context = KillerASC->MakeEffectContext();
	Context.AddInstigator(GetPawn(), GetPawn());
	FGameplayEffectSpecHandle Spec = KillerASC->MakeOutgoingSpec(UBBDamageEffect::StaticClass(), 1.0f, Context);
	if (!Spec.IsValid())
	{
		UE_LOG(LogBreachborne, Error, TEXT("BB_DEATH_SMOKE|SERVER_FAIL|reason=invalid_damage_spec"));
		return;
	}

	Spec.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, 100000.0f);
	KillerASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), VictimASC);
	ABBWispPawn* Wisp = Cast<ABBWispPawn>(VictimController->GetPawn());
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_DEATH_SMOKE|SERVER_RESULT|victim=%s health_before=%.1f health_after=%.1f alive=%d pawn=%s wisp=%d"),
		*GetNameSafe(VictimPS), HealthBefore, VictimHealth->GetHealth(), VictimPS->GetIsAlive() ? 1 : 0,
		*GetNameSafe(VictimController->GetPawn()), Wisp ? 1 : 0);
}

void ABreachbornePlayerController::ServerBeginWispHealSmoke_Implementation()
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("BBAbilitySmoke"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("BBDeathSmoke"))
		|| AbilitySmokeServerIndex != 2)
	{
		return;
	}

	if (!Cast<ABBWispPawn>(GetPawn()))
	{
		return;
	}
	if (!GetWorldTimerManager().IsTimerActive(DeathSmokeHealTimerHandle))
	{
		GetWorldTimerManager().SetTimer(
			DeathSmokeHealTimerHandle, this, &ABreachbornePlayerController::ApplyWispHealSmokeTick,
			0.2f, true, 0.0f);
		UE_LOG(LogBreachborne, Warning, TEXT("BB_DEATH_SMOKE|SERVER_HEAL_STARTED|controller=%s"), *GetName());
	}
}

void ABreachbornePlayerController::ApplyWispHealSmokeTick()
{
	ABBWispPawn* Wisp = Cast<ABBWispPawn>(GetPawn());
	if (!Wisp)
	{
		GetWorldTimerManager().ClearTimer(DeathSmokeHealTimerHandle);
		UE_LOG(LogBreachborne, Warning, TEXT("BB_DEATH_SMOKE|SERVER_HEAL_STOPPED|pawn=%s"), *GetNameSafe(GetPawn()));
		return;
	}

	Wisp->ApplyHeal(20.0f);
	UE_LOG(LogBreachborne, Verbose,
		TEXT("BB_DEATH_SMOKE|SERVER_HEAL|wisp=%s hp=%.1f rez=%.3f"),
		*GetNameSafe(Wisp), Wisp->GetCurrentWispHP(), Wisp->GetRezBarProgress());
}

void ABreachbornePlayerController::NetFlowDump()
{
	const ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	const ABreachborneGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABreachborneGameState>() : nullptr;
	const UBBGameInstance* GI = GetGameInstance<UBBGameInstance>();
	const FString Dump = FString::Printf(
		TEXT("BB_NETFLOW|DUMP|local=%d netMode=%d pawn=%s ps=%s team=%d hunter=%d ready=%d locked=%d phase=%d gi=%s frontend=%d widget=%s"),
		IsLocalController() ? 1 : 0,
		GetWorld() ? static_cast<int32>(GetWorld()->GetNetMode()) : -1,
		*GetNameSafe(GetPawn()),
		*GetNameSafe(PS),
		PS ? PS->GetTeamID() : -999,
		PS ? PS->GetHunterID() : -999,
		PS && PS->IsReadyForMatch() ? 1 : 0,
		PS && PS->IsHunterLocked() ? 1 : 0,
		GS ? static_cast<int32>(GS->GetMatchPhase()) : -1,
		*GetNameSafe(GI),
		GI ? static_cast<int32>(GI->GetFrontendState()) : -1,
		*GetNameSafe(ActiveClientFrontendWidget.Get()));

	UE_LOG(LogBreachborne, Warning, TEXT("%s"), *Dump);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Yellow, Dump);
	}
}

void ABreachbornePlayerController::NetFlowSelectHunter(int32 HunterID)
{
	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|CLIENT_CMD|NetFlowSelectHunter hunter=%d"), HunterID);
	RequestHunterSelection(HunterID);
}

void ABreachbornePlayerController::NetFlowReady(bool bReady)
{
	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|CLIENT_CMD|NetFlowReady ready=%d"), bReady ? 1 : 0);
	RequestReadyState(bReady);
}

void ABreachbornePlayerController::NetFlowValidateHunter(int32 HunterID)
{
	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|CLIENT_CMD|NetFlowValidateHunter hunter=%d"), HunterID);
	ABreachborneGameMode* BBGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABreachborneGameMode>() : nullptr;
	if (!BBGM)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|DEBUG|HunterDefinition hunter=%d valid=0 reason=missing_auth_gamemode"), HunterID);
		return;
	}

	BBGM->DebugValidateHunterDefinition(HunterID);
}

void ABreachbornePlayerController::LobbyDump()
{
	const ABreachborneGameState* GS = GetWorld() ? GetWorld()->GetGameState<ABreachborneGameState>() : nullptr;
	const ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!GS)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|DUMP|missing_gamestate"));
		return;
	}

	const FBBLobbySettings& Settings = GS->GetLobbySettings();
	UE_LOG(LogBreachborne, Warning, TEXT("BB_LOBBY|DUMP|phase=%d localPlayer=%s team=%d slot=%d spectator=%d owner=%s teamSize=%d maxTeams=%d active=%d"),
		static_cast<int32>(GS->GetMatchPhase()),
		PS ? *PS->GetPlayerName() : TEXT("None"),
		PS ? PS->GetTeamID() : -1,
		PS ? PS->GetLobbySlotIndex() : -1,
		PS && PS->IsLobbySpectator() ? 1 : 0,
		*GetNameSafe(GS->GetLobbyOwnerPlayerState()),
		Settings.TeamSize,
		Settings.MaxTeams,
		GS->GetLobbyActivePlayerCount());

	for (const FBBLobbyTeam& Team : GS->GetLobbyTeams())
	{
		FString Line = FString::Printf(TEXT("BB_LOBBY|DUMP|Team %d:"), Team.TeamID);
		for (const FBBLobbySlot& Slot : Team.Slots)
		{
			Line += FString::Printf(TEXT(" [%d:%s]"), Slot.SlotIndex, Slot.PlayerState ? *Slot.PlayerState->GetPlayerName() : TEXT("Open"));
		}
		UE_LOG(LogBreachborne, Warning, TEXT("%s"), *Line);
	}
}

void ABreachbornePlayerController::LobbyJoinSlot(int32 TeamID, int32 SlotIndex)
{
	RequestJoinLobbySlot(TeamID, SlotIndex);
}

void ABreachbornePlayerController::LobbySetTeamSize(int32 TeamSize)
{
	RequestSetLobbyTeamSize(TeamSize);
}

void ABreachbornePlayerController::LobbyStart()
{
	RequestStartLobbyMatch();
}

void ABreachbornePlayerController::ClientEnterFrontendPhase_Implementation(EMatchPhase Phase)
{
	if (!IsLocalController())
	{
		return;
	}

	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|CLIENT|EnterFrontendPhase phase=%d pc=%s pawn=%s gi=%s"),
		static_cast<int32>(Phase),
		*GetNameSafe(this),
		*GetNameSafe(GetPawn()),
		*GetNameSafe(GetGameInstance()));

	ClearClientFrontendWidget();

	if (UBBGameInstance* GI = GetGameInstance<UBBGameInstance>())
	{
		GI->ClearFrontendWidget();
		GI->SetFrontendState(Phase == EMatchPhase::WaitingForPlayers
			? EBBFrontendState::Lobby
			: ((Phase == EMatchPhase::PostMatch || Phase == EMatchPhase::Ended)
				? EBBFrontendState::PostMatch
				: EBBFrontendState::HunterSelect));
	}

	const TSubclassOf<UUserWidget> WidgetClass =
		Phase == EMatchPhase::WaitingForPlayers
			? UBBCustomLobbyWidget::StaticClass()
			: ((Phase == EMatchPhase::PostMatch || Phase == EMatchPhase::Ended)
				? UBBPostMatchWidget::StaticClass()
				: UBBHunterSelectWidget::StaticClass());

	ActiveClientFrontendWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (ActiveClientFrontendWidget)
	{
		ActiveClientFrontendWidget->AddToViewport(1000);
		PositionClientFrontendWidget(ActiveClientFrontendWidget);
		ActiveClientFrontendWidget->SetKeyboardFocus();
		UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|CLIENT|WidgetCreated class=%s widget=%s"),
			*GetNameSafe(WidgetClass.Get()),
			*GetNameSafe(ActiveClientFrontendWidget.Get()));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
				FString::Printf(TEXT("BB_NETFLOW: frontend phase %d widget %s"),
					static_cast<int32>(Phase),
					*GetNameSafe(ActiveClientFrontendWidget.Get())));
		}
	}
	else
	{
		UE_LOG(LogBreachborne, Error, TEXT("BB_NETFLOW|CLIENT|WidgetCreateFailed class=%s"), *GetNameSafe(WidgetClass.Get()));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red, TEXT("BB_NETFLOW: WidgetCreateFailed"));
		}
	}

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (ActiveClientFrontendWidget)
	{
		InputMode.SetWidgetToFocus(ActiveClientFrontendWidget->TakeWidget());
	}
	SetInputMode(InputMode);
}

void ABreachbornePlayerController::ClientEnterGameplayPhase_Implementation()
{
	if (!IsLocalController())
	{
		return;
	}

	UE_LOG(LogBreachborne, Warning, TEXT("BB_NETFLOW|CLIENT|EnterGameplayPhase pc=%s pawn=%s"),
		*GetNameSafe(this),
		*GetNameSafe(GetPawn()));

	ClearClientFrontendWidget();
	if (UBBGameInstance* GI = GetGameInstance<UBBGameInstance>())
	{
		GI->ClearFrontendWidget();
		GI->SetFrontendState(EBBFrontendState::InMatch);
	}

	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void ABreachbornePlayerController::ClearClientFrontendWidget()
{
	if (ActiveClientFrontendWidget)
	{
		ActiveClientFrontendWidget->RemoveFromParent();
		ActiveClientFrontendWidget = nullptr;
	}
}

void ABreachbornePlayerController::PositionClientFrontendWidget(UUserWidget* Widget)
{
	if (!Widget)
	{
		return;
	}

	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	const bool bIsLobbyWidget = Widget->IsA(UBBCustomLobbyWidget::StaticClass());
	const FVector2D DesiredSize = bIsLobbyWidget
		? FVector2D(FMath::Max(360.0f, ViewportSize.X - 32.0f), FMath::Max(360.0f, ViewportSize.Y - 32.0f))
		: FVector2D(FMath::Min(640.0f, FMath::Max(320.0f, ViewportSize.X - 32.0f)), FMath::Min(420.0f, FMath::Max(280.0f, ViewportSize.Y - 32.0f)));
	Widget->SetDesiredSizeInViewport(DesiredSize);
	Widget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	Widget->SetPositionInViewport(ViewportSize * 0.5f, false);
}

void ABreachbornePlayerController::RequestHunterSelection(int32 HunterID)
{
	if (HasAuthority())
	{
		ServerRequestHunterSelection_Implementation(HunterID);
	}
	else
	{
		ServerRequestHunterSelection(HunterID);
	}
}

void ABreachbornePlayerController::RequestReadyState(bool bReady)
{
	if (HasAuthority())
	{
		ServerRequestReadyState_Implementation(bReady);
	}
	else
	{
		ServerRequestReadyState(bReady);
	}
}

void ABreachbornePlayerController::RequestJoinLobbySlot(int32 TeamID, int32 SlotIndex)
{
	if (HasAuthority())
	{
		ServerRequestJoinLobbySlot_Implementation(TeamID, SlotIndex);
	}
	else
	{
		ServerRequestJoinLobbySlot(TeamID, SlotIndex);
	}
}

void ABreachbornePlayerController::RequestJoinSpectators()
{
	if (HasAuthority())
	{
		ServerRequestJoinSpectators_Implementation();
	}
	else
	{
		ServerRequestJoinSpectators();
	}
}

void ABreachbornePlayerController::RequestAutoFillLobbySlot()
{
	if (HasAuthority())
	{
		ServerRequestAutoFillLobbySlot_Implementation();
	}
	else
	{
		ServerRequestAutoFillLobbySlot();
	}
}

void ABreachbornePlayerController::RequestSetLobbyTeamSize(int32 TeamSize)
{
	if (HasAuthority())
	{
		ServerRequestSetLobbyTeamSize_Implementation(TeamSize);
	}
	else
	{
		ServerRequestSetLobbyTeamSize(TeamSize);
	}
}

void ABreachbornePlayerController::RequestSetLobbyDescription(const FString& Description)
{
	if (HasAuthority())
	{
		ServerRequestSetLobbyDescription_Implementation(Description);
	}
	else
	{
		ServerRequestSetLobbyDescription(Description);
	}
}

void ABreachbornePlayerController::RequestSetStormShiftPreset(FName PresetID)
{
	if (HasAuthority())
	{
		ServerRequestSetStormShiftPreset_Implementation(PresetID);
	}
	else
	{
		ServerRequestSetStormShiftPreset(PresetID);
	}
}

void ABreachbornePlayerController::RequestStartLobbyMatch()
{
	if (HasAuthority())
	{
		ServerRequestStartLobbyMatch_Implementation();
	}
	else
	{
		ServerRequestStartLobbyMatch();
	}
}

void ABreachbornePlayerController::ToggleFullscreenMap()
{
	if (UBBGameInstance* GI = GetGameInstance<UBBGameInstance>())
	{
		GI->ToggleFullscreenMap();
	}
}

void ABreachbornePlayerController::ServerRequestHunterSelection_Implementation(int32 HunterID)
{
	if (ABreachborneGameMode* BBGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABreachborneGameMode>() : nullptr)
	{
		BBGM->RequestHunterSelection(this, HunterID);
	}
}

void ABreachbornePlayerController::ServerRequestReadyState_Implementation(bool bReady)
{
	if (ABreachborneGameMode* BBGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABreachborneGameMode>() : nullptr)
	{
		BBGM->SetPlayerReady(this, bReady);
	}
}

void ABreachbornePlayerController::ServerRequestJoinLobbySlot_Implementation(int32 TeamID, int32 SlotIndex)
{
	if (ABreachborneGameMode* BBGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABreachborneGameMode>() : nullptr)
	{
		BBGM->RequestJoinLobbySlot(this, TeamID, SlotIndex);
	}
}

void ABreachbornePlayerController::ServerRequestJoinSpectators_Implementation()
{
	if (ABreachborneGameMode* BBGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABreachborneGameMode>() : nullptr)
	{
		BBGM->RequestJoinSpectators(this);
	}
}

void ABreachbornePlayerController::ServerRequestAutoFillLobbySlot_Implementation()
{
	if (ABreachborneGameMode* BBGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABreachborneGameMode>() : nullptr)
	{
		BBGM->RequestAutoFillLobbySlot(this);
	}
}

void ABreachbornePlayerController::ServerRequestSetLobbyTeamSize_Implementation(int32 TeamSize)
{
	if (ABreachborneGameMode* BBGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABreachborneGameMode>() : nullptr)
	{
		BBGM->RequestSetLobbyTeamSize(this, TeamSize);
	}
}

void ABreachbornePlayerController::ServerRequestSetLobbyDescription_Implementation(const FString& Description)
{
	if (ABreachborneGameMode* BBGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABreachborneGameMode>() : nullptr)
	{
		BBGM->RequestSetLobbyDescription(this, Description);
	}
}

void ABreachbornePlayerController::ServerRequestSetStormShiftPreset_Implementation(FName PresetID)
{
	if (ABreachborneGameMode* BBGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABreachborneGameMode>() : nullptr)
	{
		BBGM->RequestSetStormShiftPreset(this, PresetID);
	}
}

void ABreachbornePlayerController::ServerRequestStartLobbyMatch_Implementation()
{
	if (ABreachborneGameMode* BBGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABreachborneGameMode>() : nullptr)
	{
		BBGM->RequestStartLobbyMatch(this);
	}
}

// --- Movement Input ---

void ABreachbornePlayerController::HandleMoveInput(const FInputActionValue& Value)
{
	const FVector2D MoveInput = Value.Get<FVector2D>();

	if (APawn* ControlledPawn = GetPawn())
	{
		// Top-down: camera looks along -X (yaw=0), so screen up = +X, screen right = +Y
		const FVector ForwardDir(1.0f, 0.0f, 0.0f);   // World +X = screen up
		const FVector RightDir(0.0f, 1.0f, 0.0f);      // World +Y = screen right

		// MoveInput.X = A/D (left/right), MoveInput.Y = W/S (forward/back)
		const FVector MoveDirection = (RightDir * MoveInput.X) + (ForwardDir * MoveInput.Y);

		ControlledPawn->AddMovementInput(MoveDirection, 1.0f);
	}
}

void ABreachbornePlayerController::HandleJumpStarted(const FInputActionValue& Value)
{
	bJumpInputHeld = true;

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn());
	if (!Hunter)
	{
		return;
	}

	UGliderComponent* Glider = Hunter->GetGliderComponent();
	UBBMantleComponent* Mantle = Hunter->GetMantleComponent();
	const bool bIsFalling = Hunter->GetCharacterMovement()->IsFalling();
	if (UAbilitySystemComponent* ASC = Hunter->GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(BBGameplayTags::State_Grounded))
		{
			return;
		}
	}

	if (Mantle && (Mantle->IsMantling() || Mantle->HasRecentMantleCandidate()))
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | jump glider open suppressed | isMantling=%d recentCandidate=%d"),
			Mantle->IsMantling(),
			Mantle->HasRecentMantleCandidate());
		return;
	}

	if (bIsFalling && Glider && Glider->GetGliderState() == EGliderState::Closed)
	{
		// Falling and glider is closed — open glider.
		// Mantle is fully automatic (handled by BBMantleComponent OnComponentHit).
		Glider->ServerRequestOpenGlider();
	}
	else
	{
		// On ground = normal jump. Glider will auto-open via UpdateGliderFromHeldJump
		// once the character becomes airborne (if Space is still held).
		Hunter->Jump();
	}
}

void ABreachbornePlayerController::HandleJumpStopped(const FInputActionValue& Value)
{
	bJumpInputHeld = false;

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn());
	if (!Hunter)
	{
		return;
	}

	UGliderComponent* Glider = Hunter->GetGliderComponent();
	if (Glider && Glider->GetGliderState() == EGliderState::Open)
	{
		// Release jump while gliding = close glider
		Glider->ServerRequestCloseGlider();
	}

	Hunter->StopJumping();
}

void ABreachbornePlayerController::UpdateGliderFromHeldJump()
{
	if (!bJumpInputHeld)
	{
		return;
	}

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn());
	if (!Hunter)
	{
		return;
	}

	UGliderComponent* Glider = Hunter->GetGliderComponent();
	if (!Glider)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = Hunter->GetAbilitySystemComponent())
	{
		if (ASC->HasMatchingGameplayTag(BBGameplayTags::State_Grounded))
		{
			return;
		}
	}

	UBBMantleComponent* Mantle = Hunter->GetMantleComponent();
	if (Mantle && Mantle->IsMantling())
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | held jump glider open suppressed | reason=is-mantling"));
		return; // Don't auto-open glider while mantling
	}
	if (Mantle && Mantle->HasRecentMantleCandidate())
	{
		UE_LOG(LogBreachborne, VeryVerbose, TEXT("mantle debug | held jump glider open suppressed | reason=recent-candidate"));
		return; // Give abyss mantle recovery a short window before opening glider.
	}

	// If jump is held, character is falling, and glider is closed -> auto-open
	// This handles the "hold Space to jump + glide" case: jump fires on press,
	// character becomes airborne, then this tick detects held + falling -> opens glider.
	if (Hunter->GetCharacterMovement()->IsFalling()
		&& Glider->GetGliderState() == EGliderState::Closed)
	{
		Glider->ServerRequestOpenGlider();
	}
}

// --- Ability Input Handlers ---

void ABreachbornePlayerController::HandlePrimaryFireStarted(const FInputActionValue& Value)
{
	if (bTacticalNukeTargetingActive)
	{
		ConfirmTacticalNukeTargeting();
		return;
	}

	if (UBBAbilitySystemComponent* ASC = GetBBASC())
	{
		if (ASC->TryActivateAbilityByInputTag(BBGameplayTags::InputTag_LMB))
		{
			BeginAbilityRangePreview(BBGameplayTags::InputTag_LMB, true);
		}
	}
}

void ABreachbornePlayerController::HandlePrimaryFireCompleted(const FInputActionValue& Value)
{
	if (UBBAbilitySystemComponent* ASC = GetBBASC())
	{
		ASC->InputTagReleased(BBGameplayTags::InputTag_LMB);
	}
	EndAbilityRangePreview(BBGameplayTags::InputTag_LMB);
}

void ABreachbornePlayerController::HandleSecondaryFireStarted(const FInputActionValue& Value)
{
	if (bTacticalNukeTargetingActive)
	{
		CancelTacticalNukeTargeting();
		return;
	}

	if (UBBAbilitySystemComponent* ASC = GetBBASC())
	{
		if (ASC->TryActivateAbilityByInputTag(BBGameplayTags::InputTag_RMB))
		{
			BeginAbilityRangePreview(BBGameplayTags::InputTag_RMB, true);
		}
	}
}

void ABreachbornePlayerController::HandleSecondaryFireCompleted(const FInputActionValue& Value)
{
	if (UBBAbilitySystemComponent* ASC = GetBBASC())
	{
		ASC->InputTagReleased(BBGameplayTags::InputTag_RMB);
	}
	EndAbilityRangePreview(BBGameplayTags::InputTag_RMB);
}

void ABreachbornePlayerController::HandleDashStarted(const FInputActionValue& Value)
{
	if (UBBAbilitySystemComponent* ASC = GetBBASC())
	{
		if (ASC->TryActivateAbilityByInputTag(BBGameplayTags::InputTag_Shift))
		{
			BeginAbilityRangePreview(BBGameplayTags::InputTag_Shift, false);
		}
	}
}

void ABreachbornePlayerController::HandleAbilityQStarted(const FInputActionValue& Value)
{
	if (UBBAbilitySystemComponent* ASC = GetBBASC())
	{
		if (ASC->TryActivateAbilityByInputTag(BBGameplayTags::InputTag_Q))
		{
			BeginAbilityRangePreview(BBGameplayTags::InputTag_Q, false);
		}
	}
}

void ABreachbornePlayerController::HandleUltimateStarted(const FInputActionValue& Value)
{
	if (UBBAbilitySystemComponent* ASC = GetBBASC())
	{
		if (ASC->TryActivateAbilityByInputTag(BBGameplayTags::InputTag_R))
		{
			BeginAbilityRangePreview(BBGameplayTags::InputTag_R, false);
		}
	}
}

void ABreachbornePlayerController::HandlePower1Started(const FInputActionValue& Value)
{
	HandlePower1KeyPressed();
}

void ABreachbornePlayerController::HandlePower2Started(const FInputActionValue& Value)
{
	HandlePower2KeyPressed();
}

void ABreachbornePlayerController::HandlePower1KeyPressed()
{
	if (bTacticalNukeTargetingActive)
	{
		if (TacticalNukeTargetingInputTag.MatchesTagExact(BBGameplayTags::InputTag_Power1))
		{
			ConfirmTacticalNukeTargeting();
		}
		return;
	}

	if (IsTacticalNukeEquippedForInput(BBGameplayTags::InputTag_Power1))
	{
		BeginTacticalNukeTargeting(BBGameplayTags::InputTag_Power1);
		return;
	}

	if (UBBAbilitySystemComponent* ASC = GetBBASC())
	{
		if (ASC->TryActivateAbilityByInputTag(BBGameplayTags::InputTag_Power1))
		{
			BeginAbilityRangePreview(BBGameplayTags::InputTag_Power1, false);
		}
	}
}

void ABreachbornePlayerController::HandlePower2KeyPressed()
{
	if (bTacticalNukeTargetingActive)
	{
		if (TacticalNukeTargetingInputTag.MatchesTagExact(BBGameplayTags::InputTag_Power2))
		{
			ConfirmTacticalNukeTargeting();
		}
		return;
	}

	if (IsTacticalNukeEquippedForInput(BBGameplayTags::InputTag_Power2))
	{
		BeginTacticalNukeTargeting(BBGameplayTags::InputTag_Power2);
		return;
	}

	if (UBBAbilitySystemComponent* ASC = GetBBASC())
	{
		if (ASC->TryActivateAbilityByInputTag(BBGameplayTags::InputTag_Power2))
		{
			BeginAbilityRangePreview(BBGameplayTags::InputTag_Power2, false);
		}
	}
}

void ABreachbornePlayerController::HandleInteractStarted(const FInputActionValue& Value)
{
	HandleInteractKeyPressed();
}

void ABreachbornePlayerController::HandleInteractKeyPressed()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const float SearchRadius = 300.0f;
	AHunterCharacter* Hunter = Cast<AHunterCharacter>(ControlledPawn);
	ABreachbornePlayerState* InteractingPS = GetPlayerState<ABreachbornePlayerState>();

	if (Hunter && InteractingPS && InteractingPS->GetIsAlive())
	{
		ABBWispPawn* NearestEnemyWisp = nullptr;
		float NearestWispDistSq = FLT_MAX;
		for (TActorIterator<ABBWispPawn> It(GetWorld()); It; ++It)
		{
			ABBWispPawn* Wisp = *It;
			const ABreachbornePlayerState* WispOwner = Wisp ? Wisp->GetOwningPlayerState() : nullptr;
			if (!WispOwner || WispOwner->GetTeamID() == InteractingPS->GetTeamID())
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(PawnLocation, Wisp->GetActorLocation());
			if (DistSq <= FMath::Square(Wisp->GetExecuteRadius()) && DistSq < NearestWispDistSq)
			{
				NearestWispDistSq = DistSq;
				NearestEnemyWisp = Wisp;
			}
		}
		if (NearestEnemyWisp)
		{
			ServerInteractWithWisp(NearestEnemyWisp);
			return;
		}

		ABBDeathboxActor* NearestAllyDeathbox = nullptr;
		float NearestDeathboxDistSq = FLT_MAX;
		for (TActorIterator<ABBDeathboxActor> It(GetWorld()); It; ++It)
		{
			ABBDeathboxActor* Deathbox = *It;
			if (!Deathbox)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(PawnLocation, Deathbox->GetActorLocation());
			if (DistSq <= FMath::Square(Deathbox->GetInteractRadius()) && DistSq < NearestDeathboxDistSq)
			{
				NearestDeathboxDistSq = DistSq;
				NearestAllyDeathbox = Deathbox;
			}
		}
		if (NearestAllyDeathbox)
		{
			ServerInteractWithDeathbox(NearestAllyDeathbox);
			return;
		}
	}

	ABBBasecampActor* NearestBasecamp = nullptr;
	float NearestBasecampDistSq = FLT_MAX;

	TArray<FOverlapResult> BasecampOverlaps;
	FCollisionShape BasecampSphere = FCollisionShape::MakeSphere(SearchRadius);
	if (GetWorld()->OverlapMultiByChannel(BasecampOverlaps, PawnLocation, FQuat::Identity, ECC_WorldDynamic, BasecampSphere))
	{
		for (const FOverlapResult& Overlap : BasecampOverlaps)
		{
			ABBBasecampActor* Basecamp = Cast<ABBBasecampActor>(Overlap.GetActor());
			if (Basecamp && Basecamp->IsActorInsideAnyStation(ControlledPawn))
			{
				const float DistSq = FVector::DistSquared(PawnLocation, Basecamp->GetActorLocation());
				if (DistSq < NearestBasecampDistSq)
				{
					NearestBasecampDistSq = DistSq;
					NearestBasecamp = Basecamp;
				}
			}
		}
	}

	if (NearestBasecamp)
	{
		ServerInteractWithBasecamp(NearestBasecamp);
		return;
	}

	// Find the nearest interactable world item and request pickup.
	ABBWorldItem* NearestItem = nullptr;
	float NearestDistSq = FLT_MAX;

	// Sphere overlap to find nearby world items
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(SearchRadius);
	if (GetWorld()->OverlapMultiByChannel(Overlaps, PawnLocation, FQuat::Identity, ECC_WorldDynamic, Sphere))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			ABBWorldItem* WorldItem = Cast<ABBWorldItem>(Overlap.GetActor());
			if (WorldItem && !WorldItem->IsPickedUp())
			{
				const float DistSq = FVector::DistSquared(PawnLocation, WorldItem->GetActorLocation());
				if (DistSq < NearestDistSq)
				{
					NearestDistSq = DistSq;
					NearestItem = WorldItem;
				}
			}
		}
	}

	if (NearestItem)
	{
		NearestItem->ServerRequestPickup(this);
	}
}

void ABreachbornePlayerController::HandleRecallStarted(const FInputActionValue& Value)
{
	HandleRecallKeyPressed();
}

void ABreachbornePlayerController::HandleRecallKeyPressed()
{
	ServerStartBasecampRecall();
}

void ABreachbornePlayerController::HandleConsumable1(const FInputActionValue& Value)
{
	ServerUseConsumable(0);
}

void ABreachbornePlayerController::HandleConsumable1KeyPressed()
{
	ServerUseConsumable(0);
}

void ABreachbornePlayerController::HandleConsumable2(const FInputActionValue& Value)
{
	ServerUseConsumable(1);
}

void ABreachbornePlayerController::HandleConsumable2KeyPressed()
{
	ServerUseConsumable(1);
}

void ABreachbornePlayerController::HandleConsumable3(const FInputActionValue& Value)
{
	ServerUseConsumable(2);
}

void ABreachbornePlayerController::HandleConsumable3KeyPressed()
{
	ServerUseConsumable(2);
}

void ABreachbornePlayerController::HandleConsumable4(const FInputActionValue& Value)
{
	ServerUseConsumable(3);
}

void ABreachbornePlayerController::HandleConsumable4KeyPressed()
{
	ServerUseConsumable(3);
}

void ABreachbornePlayerController::HandleCancelTargetingKeyPressed()
{
	if (bTacticalNukeTargetingActive)
	{
		CancelTacticalNukeTargeting();
	}
}

void ABreachbornePlayerController::HandleMapToggleKeyPressed()
{
	ToggleFullscreenMap();
}

UBBAbilitySystemComponent* ABreachbornePlayerController::GetBBASC() const
{
	if (const ABreachbornePlayerState* BBPS = GetPlayerState<ABreachbornePlayerState>())
	{
		return BBPS->GetBBAbilitySystemComponent();
	}
	return nullptr;
}

// --- Cursor Aiming ---

void ABreachbornePlayerController::UpdateCursorAim(float DeltaTime)
{
	FVector CursorWorldLocation;
	if (!GetCursorWorldLocation(CursorWorldLocation))
	{
		return;
	}

	// Rotate controller yaw to face cursor
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector DirectionToCursor = (CursorWorldLocation - PawnLocation).GetSafeNormal2D();

	if (!DirectionToCursor.IsNearlyZero())
	{
		const FRotator DesiredRotation = DirectionToCursor.Rotation();
		SetControlRotation(FRotator(0.0f, DesiredRotation.Yaw, 0.0f));
	}

	// Throttle aim location RPC to ~20Hz
	AimUpdateTimer += DeltaTime;
	if (AimUpdateTimer >= AimUpdateInterval)
	{
		AimUpdateTimer = 0.0f;

		if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(ControlledPawn))
		{
			Hunter->ServerSetAimLocation(CursorWorldLocation);
		}
	}
}

bool ABreachbornePlayerController::GetCursorWorldLocation(FVector& OutLocation) const
{
	float MouseX, MouseY;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	FVector WorldOrigin, WorldDirection;
	if (!DeprojectScreenPositionToWorld(MouseX, MouseY, WorldOrigin, WorldDirection))
	{
		return false;
	}

	// Intersect the ray with the ground plane at the pawn's Z height
	const APawn* ControlledPawn = GetPawn();
	const float GroundZ = ControlledPawn ? ControlledPawn->GetActorLocation().Z : 0.0f;

	// Ray-plane intersection: plane normal is Up (0,0,1), plane point is (0,0,GroundZ)
	// t = (GroundZ - WorldOrigin.Z) / WorldDirection.Z
	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	const float T = (GroundZ - WorldOrigin.Z) / WorldDirection.Z;
	if (T < 0.0f)
	{
		return false;
	}

	OutLocation = WorldOrigin + WorldDirection * T;
	return true;
}

const UBBGameplayAbility* ABreachbornePlayerController::FindAbilityForRangePreview(const FGameplayTag& InputTag) const
{
	const UBBAbilitySystemComponent* ASC = GetBBASC();
	if (!ASC || !InputTag.IsValid())
	{
		return nullptr;
	}

	const UBBGameplayAbility* FirstMatch = nullptr;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		const UBBGameplayAbility* Ability = Cast<UBBGameplayAbility>(Spec.Ability);
		if (!Ability || (!Spec.DynamicAbilityTags.HasTagExact(InputTag)
			&& !Ability->GetAbilityInputTag().MatchesTagExact(InputTag)))
		{
			continue;
		}

		if (!Ability->GetRangeIndicatorInfo().IsEnabled())
		{
			continue;
		}

		if (Spec.IsActive())
		{
			return Ability;
		}
		FirstMatch = FirstMatch ? FirstMatch : Ability;
	}

	return FirstMatch;
}

void ABreachbornePlayerController::BeginAbilityRangePreview(const FGameplayTag& InputTag, bool bUntilRelease)
{
	if (!IsLocalController() || !FindAbilityForRangePreview(InputTag))
	{
		return;
	}

	ActiveRangePreviewInputTag = InputTag;
	bRangePreviewUntilRelease = bUntilRelease;
	ActiveRangePreviewEndTime = bUntilRelease || !GetWorld()
		? -1.0f
		: GetWorld()->GetTimeSeconds() + DiscreteRangePreviewDuration;
}

void ABreachbornePlayerController::EndAbilityRangePreview(const FGameplayTag& InputTag)
{
	if (InputTag.IsValid() && !ActiveRangePreviewInputTag.MatchesTagExact(InputTag))
	{
		return;
	}

	ActiveRangePreviewInputTag = FGameplayTag();
	ActiveRangePreviewEndTime = -1.0f;
	bRangePreviewUntilRelease = false;
}

void ABreachbornePlayerController::UpdateAbilityRangePreview()
{
	if (!ActiveRangePreviewInputTag.IsValid())
	{
		return;
	}

	const AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn());
	const ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!Hunter || (PS && !PS->GetIsAlive()) || !FindAbilityForRangePreview(ActiveRangePreviewInputTag))
	{
		EndAbilityRangePreview();
		return;
	}

	if (!bRangePreviewUntilRelease && GetWorld()
		&& ActiveRangePreviewEndTime >= 0.0f
		&& GetWorld()->GetTimeSeconds() >= ActiveRangePreviewEndTime)
	{
		EndAbilityRangePreview();
	}
}

const UBBGameplayAbility* ABreachbornePlayerController::GetActiveRangePreviewAbility() const
{
	return ActiveRangePreviewInputTag.IsValid()
		? FindAbilityForRangePreview(ActiveRangePreviewInputTag)
		: nullptr;
}

bool ABreachbornePlayerController::GetRangePreviewCursorLocation(FVector& OutLocation) const
{
	return GetCursorWorldLocation(OutLocation);
}

bool ABreachbornePlayerController::IsTacticalNukeEquippedForInput(const FGameplayTag& InputTag) const
{
	const ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		return false;
	}

	const FName PowerID = InputTag.MatchesTagExact(BBGameplayTags::InputTag_Power2)
		? PS->GetInventoryData().Power2.PowerID
		: PS->GetInventoryData().Power1.PowerID;
	return PowerID == FName(TEXT("TacticalNuke"));
}

void ABreachbornePlayerController::BeginTacticalNukeTargeting(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	bTacticalNukeTargetingActive = true;
	TacticalNukeTargetingInputTag = InputTag;
	TacticalNukeTargetLocation = GetClampedTacticalNukeTargetLocation();
	BeginAbilityRangePreview(InputTag, true);

	if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn()))
	{
		Hunter->SetAerialStrikeViewActive(true, TacticalNukeAerialViewArmLength);
	}
}

void ABreachbornePlayerController::ConfirmTacticalNukeTargeting()
{
	if (!bTacticalNukeTargetingActive)
	{
		return;
	}

	const FGameplayTag InputTag = TacticalNukeTargetingInputTag;
	const FVector TargetLocation = GetClampedTacticalNukeTargetLocation();
	CancelTacticalNukeTargeting();
	ServerConfirmTargetedPower(InputTag, TargetLocation);
}

void ABreachbornePlayerController::CancelTacticalNukeTargeting()
{
	if (!bTacticalNukeTargetingActive)
	{
		return;
	}

	bTacticalNukeTargetingActive = false;
	TacticalNukeTargetingInputTag = FGameplayTag();
	EndAbilityRangePreview();

	if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn()))
	{
		Hunter->SetAerialStrikeViewActive(false, TacticalNukeAerialViewArmLength);
	}
}

void ABreachbornePlayerController::UpdateTacticalNukeTargeting(float DeltaTime)
{
	if (!bTacticalNukeTargetingActive)
	{
		return;
	}

	TacticalNukeTargetLocation = GetClampedTacticalNukeTargetLocation();
}

FVector ABreachbornePlayerController::GetClampedTacticalNukeTargetLocation() const
{
	FVector CursorLocation = TacticalNukeTargetLocation;
	GetCursorWorldLocation(CursorLocation);

	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return CursorLocation;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	const FVector Offset2D(CursorLocation.X - PawnLocation.X, CursorLocation.Y - PawnLocation.Y, 0.0f);
	const float Dist2D = Offset2D.Size();
	if (Dist2D > TacticalNukeMaxTargetRange)
	{
		const FVector Clamped2D = Offset2D.GetSafeNormal() * TacticalNukeMaxTargetRange;
		CursorLocation.X = PawnLocation.X + Clamped2D.X;
		CursorLocation.Y = PawnLocation.Y + Clamped2D.Y;
	}

	return CursorLocation;
}

bool ABreachbornePlayerController::ConsumePendingTargetedPowerActivation(const FGameplayTag& InputTag, FVector& OutTargetLocation)
{
	if (!PendingTargetedPowerInputTag.MatchesTagExact(InputTag))
	{
		return false;
	}

	OutTargetLocation = PendingTargetedPowerLocation;
	PendingTargetedPowerInputTag = FGameplayTag();
	PendingTargetedPowerLocation = FVector::ZeroVector;
	return true;
}

// --- Debug Console Commands ---

static bool AreServerDebugCheatsAllowed(const ABreachbornePlayerController* Controller)
{
	const UBBNetworkDevSettings* Settings = GetDefault<UBBNetworkDevSettings>();
	if (Settings && Settings->bAllowPackagedDebugCheats)
	{
		return true;
	}

#if WITH_EDITOR
	if (const UWorld* World = Controller ? Controller->GetWorld() : nullptr)
	{
		if (World->WorldType == EWorldType::PIE || World->WorldType == EWorldType::Editor)
		{
			return true;
		}
	}
#endif

	return false;
}

static bool RejectServerDebugCheatIfDisabled(const ABreachbornePlayerController* Controller, const TCHAR* CommandName)
{
	if (AreServerDebugCheatsAllowed(Controller))
	{
		return false;
	}

	UE_LOG(LogBreachborne, Warning, TEXT("BB_CHEAT|SERVER|Rejected command=%s controller=%s reason=disabled"),
		CommandName ? CommandName : TEXT("unknown"),
		*GetNameSafe(Controller));
	return true;
}

void ABreachbornePlayerController::DebugForceGlide()
{
	// Exec runs on the client — forward to server via RPC
	ServerDebugForceGlide();
}

void ABreachbornePlayerController::DebugTriggerSpike()
{
	// Exec runs on the client — forward to server via RPC
	ServerDebugTriggerSpike();
}

void ABreachbornePlayerController::ServerDebugForceGlide_Implementation()
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugForceGlide")))
	{
		return;
	}

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn());
	if (!Hunter)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DebugForceGlide: No hunter pawn"));
		return;
	}

	UGliderComponent* Glider = Hunter->GetGliderComponent();
	if (!Glider)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DebugForceGlide: No glider component"));
		return;
	}

	if (Glider->GetGliderState() != EGliderState::Closed)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DebugForceGlide: Glider already in state %d"), static_cast<uint8>(Glider->GetGliderState()));
		return;
	}

	// Launch upward if on the ground
	if (!Hunter->GetCharacterMovement()->IsFalling())
	{
		Hunter->LaunchCharacter(FVector(0.0f, 0.0f, 1000.0f), false, true);
		UE_LOG(LogBreachborne, Warning, TEXT("DebugForceGlide: Launched %s upward"), *Hunter->GetName());
	}

	// Defer glider open to next tick — LaunchCharacter needs one tick to set movement mode to Falling
	// and the character needs to move above the altitude threshold
	FTimerHandle GlideTimerHandle;
	GetWorldTimerManager().SetTimer(GlideTimerHandle, [WeakGlider = TWeakObjectPtr<UGliderComponent>(Glider)]()
	{
		if (UGliderComponent* G = WeakGlider.Get())
		{
			if (G->GetGliderState() == EGliderState::Closed)
			{
				G->ServerRequestOpenGlider();
			}
		}
	}, 0.1f, false);
}

void ABreachbornePlayerController::ServerDebugTriggerSpike_Implementation()
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugTriggerSpike")))
	{
		return;
	}

	AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn());
	if (!Hunter)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DebugTriggerSpike: No hunter pawn"));
		return;
	}

	UGliderComponent* Glider = Hunter->GetGliderComponent();
	if (!Glider)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DebugTriggerSpike: No glider component"));
		return;
	}

	if (Glider->GetGliderState() != EGliderState::Open)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DebugTriggerSpike: Not gliding — glider state is %d. Use DebugForceGlide first."),
			static_cast<uint8>(Glider->GetGliderState()));
		return;
	}

	// Trigger spike with no instigator (self-spike for testing)
	Glider->TriggerSpike(nullptr);
	UE_LOG(LogBreachborne, Warning, TEXT("DebugTriggerSpike: Spiked %s (self-spike, no instigator)"), *Hunter->GetName());
}

// --- Inventory Debug Console Commands ---

static EEquipmentSlotType ParseSlotName(const FString& SlotName)
{
	if (SlotName.Equals(TEXT("Weapon"), ESearchCase::IgnoreCase)) return EEquipmentSlotType::Weapon;
	if (SlotName.Equals(TEXT("Helmet"), ESearchCase::IgnoreCase)) return EEquipmentSlotType::Helmet;
	if (SlotName.Equals(TEXT("Boots"), ESearchCase::IgnoreCase)) return EEquipmentSlotType::Boots;
	return EEquipmentSlotType::None;
}

static EEvolutionPath ParseEvolutionPath(const FString& PathName)
{
	if (PathName.Equals(TEXT("PathA"), ESearchCase::IgnoreCase) || PathName.Equals(TEXT("A"), ESearchCase::IgnoreCase)) return EEvolutionPath::PathA;
	if (PathName.Equals(TEXT("PathB"), ESearchCase::IgnoreCase) || PathName.Equals(TEXT("B"), ESearchCase::IgnoreCase)) return EEvolutionPath::PathB;
	if (PathName.Equals(TEXT("PathC"), ESearchCase::IgnoreCase) || PathName.Equals(TEXT("C"), ESearchCase::IgnoreCase)) return EEvolutionPath::PathC;
	return EEvolutionPath::None;
}

static EPowerSlotIndex ParsePowerSlotIndex(int32 SlotIndex)
{
	return SlotIndex == 2 ? EPowerSlotIndex::Power2 : EPowerSlotIndex::Power1;
}

static EArmorTier ParseArmorTier(const FString& TierName)
{
	if (TierName.Equals(TEXT("None"), ESearchCase::IgnoreCase)) return EArmorTier::None;
	if (TierName.Equals(TEXT("White"), ESearchCase::IgnoreCase)) return EArmorTier::White;
	if (TierName.Equals(TEXT("Green"), ESearchCase::IgnoreCase)) return EArmorTier::Green;
	if (TierName.Equals(TEXT("Blue"), ESearchCase::IgnoreCase)) return EArmorTier::Blue;
	if (TierName.Equals(TEXT("Purple"), ESearchCase::IgnoreCase)) return EArmorTier::Purple;
	return EArmorTier::None;
}

static FName ParseKeyItemID(const FString& KeyName)
{
	if (KeyName.Equals(TEXT("Yellow"), ESearchCase::IgnoreCase)
		|| KeyName.Equals(TEXT("YellowKey"), ESearchCase::IgnoreCase))
	{
		return FName(TEXT("YellowKey"));
	}
	if (KeyName.Equals(TEXT("Red"), ESearchCase::IgnoreCase)
		|| KeyName.Equals(TEXT("RedKey"), ESearchCase::IgnoreCase))
	{
		return FName(TEXT("RedKey"));
	}
	return FName(*KeyName);
}

void ABreachbornePlayerController::DebugAddGold(int32 Amount)
{
	ServerDebugAddGold(Amount);
}

void ABreachbornePlayerController::DebugAddShards(int32 Amount)
{
	ServerDebugAddShards(Amount);
}

void ABreachbornePlayerController::DebugEquipTestWeapon()
{
	ServerDebugEquipTestItem(EEquipmentSlotType::Weapon);
}

void ABreachbornePlayerController::DebugEquipTestHelmet()
{
	ServerDebugEquipTestItem(EEquipmentSlotType::Helmet);
}

void ABreachbornePlayerController::DebugEquipTestBoots()
{
	ServerDebugEquipTestItem(EEquipmentSlotType::Boots);
}

void ABreachbornePlayerController::DebugUnequip(const FString& SlotName)
{
	EEquipmentSlotType SlotType = ParseSlotName(SlotName);
	if (SlotType == EEquipmentSlotType::None)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DebugUnequip: Invalid slot name '%s'. Use Weapon/Helmet/Boots."), *SlotName);
		return;
	}
	ServerDebugUnequip(SlotType);
}

void ABreachbornePlayerController::DebugUpgrade(const FString& SlotName)
{
	EEquipmentSlotType SlotType = ParseSlotName(SlotName);
	if (SlotType == EEquipmentSlotType::None)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DebugUpgrade: Invalid slot name '%s'. Use Weapon/Helmet/Boots."), *SlotName);
		return;
	}
	ServerDebugUpgrade(SlotType);
}

void ABreachbornePlayerController::DebugEvolve(const FString& SlotName, const FString& PathName)
{
	EEquipmentSlotType SlotType = ParseSlotName(SlotName);
	EEvolutionPath Path = ParseEvolutionPath(PathName);
	if (SlotType == EEquipmentSlotType::None)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DebugEvolve: Invalid slot name '%s'. Use Weapon/Helmet/Boots."), *SlotName);
		return;
	}
	if (Path == EEvolutionPath::None)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DebugEvolve: Invalid path '%s'. Use A/B/C or PathA/PathB/PathC."), *PathName);
		return;
	}
	ServerDebugEvolve(SlotType, Path);
}

void ABreachbornePlayerController::DebugSetArmor(const FString& TierName)
{
	EArmorTier Tier = ParseArmorTier(TierName);
	ServerDebugSetArmor(Tier);
}

void ABreachbornePlayerController::DebugPrintInventory()
{
	// This runs on client — read from replicated data
	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DebugPrintInventory: No PlayerState"));
		return;
	}

	const FRepInventoryData& Inv = PS->GetInventoryData();

	UE_LOG(LogBreachborne, Warning, TEXT(""));
	UE_LOG(LogBreachborne, Warning, TEXT("=== INVENTORY: %s ==="), *PS->GetPlayerName());
	UE_LOG(LogBreachborne, Warning, TEXT("  Gold: %d | Shards: %d"), Inv.Gold, Inv.UpgradeShards);
	UE_LOG(LogBreachborne, Warning, TEXT("  Weapon: %s (Tier %d, Evo %s)"),
		*Inv.Weapon.ItemID.ToString(), Inv.Weapon.UpgradeTier,
		*UEnum::GetValueAsString(Inv.Weapon.Evolution));
	UE_LOG(LogBreachborne, Warning, TEXT("  Helmet: %s (Tier %d, Evo %s)"),
		*Inv.Helmet.ItemID.ToString(), Inv.Helmet.UpgradeTier,
		*UEnum::GetValueAsString(Inv.Helmet.Evolution));
	UE_LOG(LogBreachborne, Warning, TEXT("  Boots:  %s (Tier %d, Evo %s)"),
		*Inv.Boots.ItemID.ToString(), Inv.Boots.UpgradeTier,
		*UEnum::GetValueAsString(Inv.Boots.Evolution));
	UE_LOG(LogBreachborne, Warning, TEXT("  Armor:  %s (HP %.0f)"),
		*UEnum::GetValueAsString(Inv.Armor.Tier), Inv.Armor.CurrentHP);
	UE_LOG(LogBreachborne, Warning, TEXT("  Power1: %s"), *Inv.Power1.PowerID.ToString());
	UE_LOG(LogBreachborne, Warning, TEXT("  Power2: %s"), *Inv.Power2.PowerID.ToString());
	for (int32 i = 0; i < Inv.Backpack.Num(); ++i)
	{
		const FConsumableStack& Stack = Inv.Backpack[i];
		UE_LOG(LogBreachborne, Warning, TEXT("  Backpack[%d]: %s x%d"),
			i, *Stack.ConsumableID.ToString(), Stack.Count);
	}
	UE_LOG(LogBreachborne, Warning, TEXT("==========================="));
}

void ABreachbornePlayerController::DebugGivePower(const FString& PowerName, int32 SlotIndex)
{
	ServerDebugGivePower(PowerName, SlotIndex);
}

void ABreachbornePlayerController::DebugDropPower(int32 SlotIndex)
{
	ServerDebugDropPower(SlotIndex);
}

void ABreachbornePlayerController::DebugPrintPowers()
{
	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DebugPrintPowers: No PlayerState"));
		return;
	}

	const FRepInventoryData& Inv = PS->GetInventoryData();
	UE_LOG(LogBreachborne, Warning, TEXT("PowerInventory: %s Power1(F)=%s Power2(G)=%s"),
		*PS->GetPlayerName(),
		*Inv.Power1.PowerID.ToString(),
		*Inv.Power2.PowerID.ToString());
}

void ABreachbornePlayerController::DebugSpawnPowerPickup(const FString& PowerName)
{
	ServerDebugSpawnPowerPickup(PowerName);
}

void ABreachbornePlayerController::BasecampGiveGold(int32 Amount)
{
	ServerDebugAddGold(Amount);
}

void ABreachbornePlayerController::BasecampGiveViveBeans(int32 Count)
{
	ServerDebugAddConsumable(TEXT("ViveBean"), Count);
}

void ABreachbornePlayerController::BasecampBreakShield()
{
	ServerBasecampBreakShield();
}

void ABreachbornePlayerController::BasecampBreakArmor()
{
	ServerBasecampBreakArmor();
}

void ABreachbornePlayerController::BasecampDamageSelf(float Amount)
{
	ServerBasecampDamageSelf(Amount);
}

// --- Server RPC Implementations for Inventory Debug ---

void ABreachbornePlayerController::ServerDebugAddGold_Implementation(int32 Amount)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugAddGold")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (PS)
	{
		FBBInventoryManager::AddGold(PS, Amount);
	}
}

void ABreachbornePlayerController::ServerDebugAddShards_Implementation(int32 Amount)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugAddShards")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (PS)
	{
		FBBInventoryManager::AddShards(PS, Amount);
	}
}

void ABreachbornePlayerController::ServerDebugEquipTestItem_Implementation(EEquipmentSlotType SlotType)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugEquipTestItem")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		return;
	}

	// Create a temporary equipment definition for testing
	// In production, this would look up from the ItemRegistry
	UBBEquipmentDefinition* TestDef = NewObject<UBBEquipmentDefinition>();
	TestDef->SlotType = SlotType;

	switch (SlotType)
	{
	case EEquipmentSlotType::Weapon:
		TestDef->ItemID = FName(TEXT("TestWeapon"));
		TestDef->BaseStats.AttackPower = 10.0f;
		break;
	case EEquipmentSlotType::Helmet:
		TestDef->ItemID = FName(TEXT("TestHelmet"));
		TestDef->BaseStats.MaxHealth = 50.0f;
		break;
	case EEquipmentSlotType::Boots:
		TestDef->ItemID = FName(TEXT("TestBoots"));
		TestDef->BaseStats.MoveSpeed = 25.0f;
		break;
	default:
		return;
	}

	// Set up shard costs and evolution
	TestDef->ShardCostPerTier = { 5, 5, 10 };
	TestDef->EvolutionTier = 2;
	TestDef->EvolutionA.DisplayName = FText::FromString(TEXT("Evolution A"));
	TestDef->EvolutionA.BonusStats.AttackPower = 15.0f;
	TestDef->EvolutionB.DisplayName = FText::FromString(TEXT("Evolution B"));
	TestDef->EvolutionB.BonusStats.MaxHealth = 30.0f;
	TestDef->EvolutionC.DisplayName = FText::FromString(TEXT("Evolution C"));
	TestDef->EvolutionC.BonusStats.MoveSpeed = 20.0f;

	FBBInventoryManager::EquipItem(PS, TestDef);
}

void ABreachbornePlayerController::ServerDebugUnequip_Implementation(EEquipmentSlotType SlotType)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugUnequip")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (PS)
	{
		FBBInventoryManager::UnequipSlot(PS, SlotType);
	}
}

void ABreachbornePlayerController::ServerDebugUpgrade_Implementation(EEquipmentSlotType SlotType)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugUpgrade")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (PS)
	{
		FBBInventoryManager::UpgradeEquipment(PS, SlotType);
	}
}

void ABreachbornePlayerController::ServerDebugEvolve_Implementation(EEquipmentSlotType SlotType, EEvolutionPath Path)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugEvolve")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (PS)
	{
		FBBInventoryManager::EvolveEquipment(PS, SlotType, Path);
	}
}

void ABreachbornePlayerController::ServerDebugSetArmor_Implementation(EArmorTier Tier)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugSetArmor")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (PS)
	{
		FBBInventoryManager::SetArmorTier(PS, Tier);
	}
}

void ABreachbornePlayerController::DebugAddConsumable(const FString& ConsumableName, int32 Count)
{
	ServerDebugAddConsumable(ConsumableName, Count);
}

void ABreachbornePlayerController::DebugGiveKey(const FString& KeyName, int32 Count)
{
	ServerDebugGiveKey(KeyName, Count);
}

void ABreachbornePlayerController::DebugSpawnKeyPickup(const FString& KeyName)
{
	ServerDebugSpawnKeyPickup(KeyName);
}

void ABreachbornePlayerController::ServerDebugAddConsumable_Implementation(const FString& ConsumableName, int32 Count)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugAddConsumable")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		return;
	}

	if (ConsumableName.Equals(TEXT("ViveBean"), ESearchCase::IgnoreCase)
		|| ConsumableName.Equals(TEXT("ViveBrew"), ESearchCase::IgnoreCase))
	{
		FBBInventoryManager::AddConsumableStack(PS, FName(*ConsumableName), Count);
		return;
	}

	// Create a temp consumable definition
	UBBConsumableDefinition* TempDef = NewObject<UBBConsumableDefinition>();
	TempDef->ConsumableID = FName(*ConsumableName);
	TempDef->MaxStackSize = 5;

	if (ConsumableName.StartsWith(TEXT("Food")))
	{
		TempDef->ConsumableType = EConsumableType::Food;
		TempDef->InstantHealAmount = 100.0f;
	}
	else if (ConsumableName.StartsWith(TEXT("ArmorShard")))
	{
		TempDef->ConsumableType = EConsumableType::ArmorShard;
		TempDef->ArmorRepairAmount = 50.0f;
	}
	else if (ConsumableName.StartsWith(TEXT("ViveBrew")))
	{
		TempDef->ConsumableType = EConsumableType::ViveBrew;
	}

	FBBInventoryManager::AddConsumable(PS, TempDef, Count);
}

void ABreachbornePlayerController::ServerDebugGiveKey_Implementation(const FString& KeyName, int32 Count)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugGiveKey")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		return;
	}

	const FName KeyItemID = ParseKeyItemID(KeyName);
	if (KeyItemID.IsNone())
	{
		return;
	}

	FBBInventoryManager::AddConsumableStack(PS, KeyItemID, FMath::Max(1, Count), 99);
	UE_LOG(LogBreachborne, Warning, TEXT("DebugGiveKey: gave %s x%d to %s"),
		*KeyItemID.ToString(), FMath::Max(1, Count), *PS->GetPlayerName());
}

void ABreachbornePlayerController::ServerDebugSpawnKeyPickup_Implementation(const FString& KeyName)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugSpawnKeyPickup")))
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const FName KeyItemID = ParseKeyItemID(KeyName);
	if (KeyItemID.IsNone())
	{
		return;
	}

	const FVector KeySpawnLocation = ControlledPawn->GetActorLocation()
		+ ControlledPawn->GetActorForwardVector() * 180.0f
		+ FVector(0.0f, 0.0f, 40.0f);
	FBBWorldItemSpawner::SpawnConsumablePickup(GetWorld(), KeySpawnLocation, KeyItemID, 1);
	UE_LOG(LogBreachborne, Warning, TEXT("DebugSpawnKeyPickup: spawned %s at %s"),
		*KeyItemID.ToString(), *KeySpawnLocation.ToString());
}

void ABreachbornePlayerController::ServerDebugGivePower_Implementation(const FString& PowerName, int32 SlotIndex)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugGivePower")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		return;
	}

	const bool bUsePreferredSlot = SlotIndex == 1 || SlotIndex == 2;
	const EPowerSlotIndex Slot = ParsePowerSlotIndex(SlotIndex);
	FBBInventoryManager::EquipPowerByID(PS, FName(*PowerName), Slot, bUsePreferredSlot);
}

void ABreachbornePlayerController::ServerDebugDropPower_Implementation(int32 SlotIndex)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugDropPower")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		return;
	}

	FBBInventoryManager::UnequipPower(PS, ParsePowerSlotIndex(SlotIndex));
}

void ABreachbornePlayerController::ServerDebugSpawnPowerPickup_Implementation(const FString& PowerName)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerDebugSpawnPowerPickup")))
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const FVector PickupSpawnLocation = ControlledPawn->GetActorLocation() + ControlledPawn->GetActorForwardVector() * 220.0f + FVector(0.0f, 0.0f, 40.0f);
	if (FBBWorldItemSpawner::SpawnPowerPickup(GetWorld(), PickupSpawnLocation, FName(*PowerName)))
	{
		UE_LOG(LogBreachborne, Log, TEXT("PowerPickup: spawned debug pickup %s"), *PowerName);
	}
}

void ABreachbornePlayerController::ServerConfirmTargetedPower_Implementation(FGameplayTag InputTag, FVector_NetQuantize TargetLocation)
{
	if (!InputTag.MatchesTagExact(BBGameplayTags::InputTag_Power1) && !InputTag.MatchesTagExact(BBGameplayTags::InputTag_Power2))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	APawn* ControlledPawn = GetPawn();
	if (!PS || !ControlledPawn || !PS->GetIsAlive() || !IsTacticalNukeEquippedForInput(InputTag))
	{
		return;
	}

	const FVector PawnLocation = ControlledPawn->GetActorLocation();
	FVector ValidatedTarget = FVector(TargetLocation);
	const FVector Offset2D(ValidatedTarget.X - PawnLocation.X, ValidatedTarget.Y - PawnLocation.Y, 0.0f);
	const float Dist2D = Offset2D.Size();
	if (Dist2D > TacticalNukeMaxTargetRange)
	{
		const FVector Clamped2D = Offset2D.GetSafeNormal() * TacticalNukeMaxTargetRange;
		ValidatedTarget.X = PawnLocation.X + Clamped2D.X;
		ValidatedTarget.Y = PawnLocation.Y + Clamped2D.Y;
	}

	PendingTargetedPowerInputTag = InputTag;
	PendingTargetedPowerLocation = ValidatedTarget;

	if (UBBAbilitySystemComponent* ASC = GetBBASC())
	{
		const bool bActivated = ASC->TryActivateAbilityByInputTag(InputTag);
		if (!bActivated)
		{
			PendingTargetedPowerInputTag = FGameplayTag();
			PendingTargetedPowerLocation = FVector::ZeroVector;
		}
	}
}

void ABreachbornePlayerController::ServerBasecampBreakShield_Implementation()
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerBasecampBreakShield")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		return;
	}

	if (UBBHealthSet* HealthSet = PS->GetHealthSet())
	{
		const float OldShield = HealthSet->GetShield();
		HealthSet->SetShield(0.0f);
		PS->UpdateHealthProxy();
		UE_LOG(LogBreachborne, Log, TEXT("BasecampInventory: %s shield broken %.0f -> 0"),
			*PS->GetPlayerName(), OldShield);
	}
}

void ABreachbornePlayerController::ServerBasecampBreakArmor_Implementation()
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerBasecampBreakArmor")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!PS)
	{
		return;
	}

	if (!PS->GetInventoryData().Armor.HasArmor())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BasecampArmor: %s cannot break armor - no armor equipped"),
			*PS->GetPlayerName());
		return;
	}

	FBBInventoryManager::ApplyArmorDurabilityDamage(PS, 100000.0f);
}

void ABreachbornePlayerController::ServerBasecampDamageSelf_Implementation(float Amount)
{
	if (RejectServerDebugCheatIfDisabled(this, TEXT("ServerBasecampDamageSelf")))
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (!PS || Amount <= 0.0f)
	{
		return;
	}

	if (UBBHealthSet* HealthSet = PS->GetHealthSet())
	{
		const float OldHealth = HealthSet->GetHealth();
		const float NewHealth = FMath::Max(1.0f, OldHealth - Amount);
		HealthSet->SetHealth(NewHealth);
		PS->UpdateHealthProxy();
		UE_LOG(LogBreachborne, Log, TEXT("BasecampInventory: %s self damage %.0f -> %.0f"),
			*PS->GetPlayerName(), OldHealth, NewHealth);
	}
}

void ABreachbornePlayerController::ServerUseConsumable_Implementation(int32 BackpackSlotIndex)
{
	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	if (PS)
	{
		FBBInventoryManager::UseConsumable(PS, BackpackSlotIndex);
	}
}

void ABreachbornePlayerController::ServerInteractWithBasecamp_Implementation(ABBBasecampActor* Basecamp)
{
	if (Basecamp)
	{
		Basecamp->ServerRequestInteract(this);
	}
}

void ABreachbornePlayerController::ServerInteractWithWisp_Implementation(ABBWispPawn* Wisp)
{
	AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn());
	ABreachbornePlayerState* InteractingPS = GetPlayerState<ABreachbornePlayerState>();
	ABreachbornePlayerState* WispOwner = Wisp ? Wisp->GetOwningPlayerState() : nullptr;
	if (!Hunter || !InteractingPS || !InteractingPS->GetIsAlive() || !WispOwner
		|| WispOwner->GetTeamID() == InteractingPS->GetTeamID()
		|| FVector::DistSquared(Hunter->GetActorLocation(), Wisp->GetActorLocation()) > FMath::Square(Wisp->GetExecuteRadius()))
	{
		UE_LOG(LogBreachborne, Warning, TEXT("WispInteract: execute rejected player=%s wisp=%s"),
			*GetNameSafe(InteractingPS), *GetNameSafe(Wisp));
		return;
	}

	Wisp->ServerBeginExecute(Hunter);
}

void ABreachbornePlayerController::ServerInteractWithDeathbox_Implementation(ABBDeathboxActor* Deathbox)
{
	AHunterCharacter* Hunter = Cast<AHunterCharacter>(GetPawn());
	ABreachbornePlayerState* InteractingPS = GetPlayerState<ABreachbornePlayerState>();
	ABreachbornePlayerState* VictimPS = Deathbox ? Deathbox->GetVictimPlayerState() : nullptr;
	if (!Hunter || !InteractingPS || !InteractingPS->GetIsAlive() || !VictimPS
		|| VictimPS->GetTeamID() != InteractingPS->GetTeamID()
		|| FVector::DistSquared(Hunter->GetActorLocation(), Deathbox->GetActorLocation()) > FMath::Square(Deathbox->GetInteractRadius()))
	{
		UE_LOG(LogBreachborne, Warning, TEXT("DeathboxInteract: revive rejected player=%s deathbox=%s"),
			*GetNameSafe(InteractingPS), *GetNameSafe(Deathbox));
		return;
	}

	Deathbox->ServerBeginReviveChannel(Hunter);
}

void ABreachbornePlayerController::ServerStartBasecampRecall_Implementation()
{
	if (bBasecampRecallActive)
	{
		return;
	}

	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	APawn* ControlledPawn = GetPawn();
	if (!PS || !ControlledPawn || !PS->GetIsAlive() || PS->GetTeamID() == -1)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BasecampRecall: rejected - invalid player state"));
		return;
	}

	ABBBasecampActor* RecallBasecamp = FindActiveRecallBasecampForTeam(PS->GetTeamID());
	if (!RecallBasecamp)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BasecampRecall: rejected - team %d has no active basecamp"), PS->GetTeamID());
		return;
	}

	ActiveRecallBasecamp = RecallBasecamp;
	bBasecampRecallActive = true;
	BasecampRecallProgress = 0.0f;
	BasecampRecallElapsed = 0.0f;
	BasecampRecallStartLocation = ControlledPawn->GetActorLocation();

	if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
	{
		RecallHealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(UBBHealthSet::GetHealthAttribute())
			.AddUObject(this, &ABreachbornePlayerController::OnRecallHealthChanged);
	}

	GetWorldTimerManager().SetTimer(BasecampRecallTimerHandle, this, &ABreachbornePlayerController::RecallTick, 0.1f, true);
	ForceNetUpdate();

	UE_LOG(LogBreachborne, Log, TEXT("BasecampRecall: %s started recall to %s"),
		*PS->GetPlayerName(), *RecallBasecamp->GetName());
}

void ABreachbornePlayerController::RecallTick()
{
	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	APawn* ControlledPawn = GetPawn();
	if (!bBasecampRecallActive || !PS || !ControlledPawn || !ActiveRecallBasecamp)
	{
		CancelBasecampRecall(TEXT("missing recall state"));
		return;
	}

	if (!PS->GetIsAlive())
	{
		CancelBasecampRecall(TEXT("player not alive"));
		return;
	}

	if (!ActiveRecallBasecamp->CanTeamRecallHere(PS->GetTeamID()))
	{
		CancelBasecampRecall(TEXT("basecamp ownership lost"));
		return;
	}

	if (FVector::Dist2D(ControlledPawn->GetActorLocation(), BasecampRecallStartLocation) > BasecampRecallMoveCancelDistance)
	{
		CancelBasecampRecall(TEXT("moved"));
		return;
	}

	BasecampRecallElapsed += 0.1f;
	BasecampRecallProgress = FMath::Clamp(BasecampRecallElapsed / FMath::Max(0.1f, BasecampRecallDuration), 0.0f, 1.0f);
	ForceNetUpdate();

	if (BasecampRecallProgress >= 1.0f)
	{
		CompleteBasecampRecall();
	}
}

void ABreachbornePlayerController::CancelBasecampRecall(const TCHAR* Reason)
{
	if (!bBasecampRecallActive)
	{
		return;
	}

	UE_LOG(LogBreachborne, Log, TEXT("BasecampRecall: cancelled (%s)"), Reason ? Reason : TEXT("unknown"));
	ClearBasecampRecallState();
}

void ABreachbornePlayerController::CompleteBasecampRecall()
{
	ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>();
	APawn* ControlledPawn = GetPawn();
	if (!PS || !ControlledPawn || !ActiveRecallBasecamp || !ActiveRecallBasecamp->CanTeamRecallHere(PS->GetTeamID()))
	{
		CancelBasecampRecall(TEXT("completion invalid"));
		return;
	}

	const FTransform LandingTransform = ActiveRecallBasecamp->GetRecallLandingTransform();
	if (TryTeleportPawnNearTransform(ControlledPawn, LandingTransform))
	{
		UE_LOG(LogBreachborne, Log, TEXT("BasecampRecall: %s completed recall to %s"),
			*PS->GetPlayerName(), *ActiveRecallBasecamp->GetName());
	}
	else
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BasecampRecall: %s recall landing blocked near %s"),
			*PS->GetPlayerName(), *ActiveRecallBasecamp->GetName());
	}

	ClearBasecampRecallState();
}

void ABreachbornePlayerController::ClearBasecampRecallState()
{
	GetWorldTimerManager().ClearTimer(BasecampRecallTimerHandle);

	if (ABreachbornePlayerState* PS = GetPlayerState<ABreachbornePlayerState>())
	{
		if (UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(UBBHealthSet::GetHealthAttribute()).Remove(RecallHealthHandle);
		}
	}

	RecallHealthHandle.Reset();
	bBasecampRecallActive = false;
	BasecampRecallProgress = 0.0f;
	BasecampRecallElapsed = 0.0f;
	BasecampRecallStartLocation = FVector::ZeroVector;
	ActiveRecallBasecamp = nullptr;
	ForceNetUpdate();
}

void ABreachbornePlayerController::OnRecallHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue < Data.OldValue)
	{
		CancelBasecampRecall(TEXT("took damage"));
	}
}

ABBBasecampActor* ABreachbornePlayerController::FindActiveRecallBasecampForTeam(int32 TeamID) const
{
	UWorld* World = GetWorld();
	if (!World || TeamID == -1)
	{
		return nullptr;
	}

	for (TActorIterator<ABBBasecampActor> It(World); It; ++It)
	{
		ABBBasecampActor* Basecamp = *It;
		if (Basecamp && Basecamp->CanTeamRecallHere(TeamID))
		{
			return Basecamp;
		}
	}

	return nullptr;
}

bool ABreachbornePlayerController::TryTeleportPawnNearTransform(APawn* PawnToTeleport, const FTransform& TargetTransform) const
{
	if (!PawnToTeleport)
	{
		return false;
	}

	const FVector BaseLocation = TargetTransform.GetLocation();
	const FRotator BaseRotation = TargetTransform.Rotator();
	static const FVector Offsets[] =
	{
		FVector::ZeroVector,
		FVector(150.0f, 0.0f, 0.0f),
		FVector(-150.0f, 0.0f, 0.0f),
		FVector(0.0f, 150.0f, 0.0f),
		FVector(0.0f, -150.0f, 0.0f),
		FVector(110.0f, 110.0f, 0.0f),
		FVector(-110.0f, -110.0f, 0.0f)
	};

	for (const FVector& Offset : Offsets)
	{
		if (PawnToTeleport->TeleportTo(BaseLocation + Offset, BaseRotation, false, true))
		{
			return true;
		}
	}

	return false;
}
