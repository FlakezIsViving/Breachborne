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
#include "Breachborne/Abilities/Hunters/Eluna/GA_Eluna_Shift.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "Breachborne/Combat/BBAllyBot.h"
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

namespace
{
	enum EBBOutcomeSmokeStateBits : int32
	{
		Outcome_Stunned = 1 << 0,
		Outcome_Slowed = 1 << 1,
		Outcome_Grounded = 1 << 2,
		Outcome_AntiHeal = 1 << 3,
		Outcome_Hooked = 1 << 4,
		Outcome_HudsonHooked = 1 << 5,
		Outcome_Reverberation = 1 << 6,
		Outcome_CrystaEmpowered = 1 << 7,
		Outcome_VoidEmpowered = 1 << 8,
		Outcome_VoidSwapping = 1 << 9,
		Outcome_VoidPulled = 1 << 10,
		Outcome_HudsonSpinning = 1 << 11,
		Outcome_HudsonSpunUp = 1 << 12,
		Outcome_HudsonHovering = 1 << 13,
		Outcome_CrystaPrimaryShiftCooldown = 1 << 14,
		Outcome_CrystaSecondaryShiftCooldown = 1 << 15
	};

	int32 BuildOutcomeSmokeStateMask(const UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return 0;
		}

		int32 Mask = 0;
		auto AddIfPresent = [ASC, &Mask](const FGameplayTag& Tag, int32 Bit)
		{
			if (ASC->HasMatchingGameplayTag(Tag))
			{
				Mask |= Bit;
			}
		};
		AddIfPresent(BBGameplayTags::State_Stunned, Outcome_Stunned);
		AddIfPresent(BBGameplayTags::State_Slowed, Outcome_Slowed);
		AddIfPresent(BBGameplayTags::State_Grounded, Outcome_Grounded);
		AddIfPresent(BBGameplayTags::State_AntiHeal, Outcome_AntiHeal);
		AddIfPresent(BBGameplayTags::State_Hooked, Outcome_Hooked);
		AddIfPresent(BBGameplayTags::State_Hudson_Hooked, Outcome_HudsonHooked);
		AddIfPresent(BBGameplayTags::State_Crysta_Reverberation, Outcome_Reverberation);
		AddIfPresent(BBGameplayTags::State_Crysta_EmpoweredLMB, Outcome_CrystaEmpowered);
		AddIfPresent(BBGameplayTags::State_Void_Empowered, Outcome_VoidEmpowered);
		AddIfPresent(BBGameplayTags::State_Void_Swapping, Outcome_VoidSwapping);
		AddIfPresent(BBGameplayTags::State_Void_SingularityPulled, Outcome_VoidPulled);
		AddIfPresent(BBGameplayTags::State_Hudson_Spinning, Outcome_HudsonSpinning);
		AddIfPresent(BBGameplayTags::State_Hudson_SpunUp, Outcome_HudsonSpunUp);
		AddIfPresent(BBGameplayTags::State_Hudson_Hovering, Outcome_HudsonHovering);
		AddIfPresent(BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Primary, Outcome_CrystaPrimaryShiftCooldown);
		AddIfPresent(BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Secondary, Outcome_CrystaSecondaryShiftCooldown);
		return Mask;
	}

	float GetOutcomeCooldownRemaining(const UAbilitySystemComponent* ASC, const FGameplayTag& CooldownTag)
	{
		if (!ASC || !CooldownTag.IsValid())
		{
			return 0.0f;
		}
		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
			FGameplayTagContainer(CooldownTag));
		float MaxRemaining = 0.0f;
		for (const float Remaining : ASC->GetActiveEffectsTimeRemaining(Query))
		{
			MaxRemaining = FMath::Max(MaxRemaining, Remaining);
		}
		return MaxRemaining;
	}

	int32 CountOutcomeSmokeOwnedActors(UWorld* World, const AActor* Owner)
	{
		if (!World || !Owner)
		{
			return 0;
		}

		int32 Count = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (!It->IsActorBeingDestroyed() && It->GetOwner() == Owner)
			{
				++Count;
			}
		}
		return Count;
	}

	const TCHAR* GetOutcomeSmokeStageName(int32 Stage)
	{
		switch (Stage)
		{
		case 0: return TEXT("RMB");
		case 1: return TEXT("SHIFT");
		case 2: return TEXT("Q");
		case 3: return TEXT("R");
		default: return TEXT("UNKNOWN");
		}
	}

	float GetOutcomeSmokeSeparation(int32 HunterID, int32 Stage)
	{
		if (Stage == 1)
		{
			if (HunterID == 6) return 1200.0f;
			if (HunterID == 4) return 650.0f;
			return 300.0f;
		}
		if (Stage == 2)
		{
			switch (HunterID)
			{
			case 2: return 250.0f;
			case 3: return 250.0f;
			case 4: return 900.0f;
			case 5:
			case 6: return 800.0f;
			default: return 500.0f;
			}
		}
		if (Stage == 3)
		{
			switch (HunterID)
			{
			case 3: return 250.0f;
			case 4: return 700.0f;
			case 5:
			case 6: return 800.0f;
			default: return 600.0f;
			}
		}
		return 450.0f;
	}

	float GetOutcomeSmokeAttackerAimX(int32 HunterID, int32 Stage, float Separation)
	{
		// Keep Void's singularity center offset so its pull has measurable travel.
		return Separation * 0.5f + ((HunterID == 6 && Stage == 3) ? 250.0f : 0.0f);
	}

	enum EBBWispRulesSmokeStage : int32
	{
		WispRules_Natural = 0,
		WispRules_Ally,
		WispRules_Contest,
		WispRules_Enemy,
		WispRules_Healing,
		WispRules_HealingLatched,
		WispRules_HealingContested,
		WispRules_CarriedContested,
		WispRules_ElunaShiftPickup,
		WispRules_ElunaPickup,
		WispRules_ElunaCCDrop,
		WispRules_ElunaRRevive,
		WispRules_Count
	};

	const TCHAR* GetWispRulesSmokeStageName(int32 Stage)
	{
		switch (Stage)
		{
		case WispRules_Natural: return TEXT("natural");
		case WispRules_Ally: return TEXT("ally");
		case WispRules_Contest: return TEXT("ally_enemy");
		case WispRules_Enemy: return TEXT("enemy");
		case WispRules_Healing: return TEXT("healing");
		case WispRules_HealingLatched: return TEXT("healing_latched");
		case WispRules_HealingContested: return TEXT("healing_enemy");
		case WispRules_CarriedContested: return TEXT("carried_enemy");
		case WispRules_ElunaShiftPickup: return TEXT("eluna_shift_pickup");
		case WispRules_ElunaPickup: return TEXT("eluna_pickup");
		case WispRules_ElunaCCDrop: return TEXT("eluna_cc_drop");
		case WispRules_ElunaRRevive: return TEXT("eluna_r_revive");
		default: return TEXT("unknown");
		}
	}

	float GetWispRulesSmokeStageDuration(int32 Stage)
	{
		if (Stage == WispRules_Healing)
		{
			return 0.6f;
		}
		if (Stage == WispRules_CarriedContested)
		{
			return 0.4f;
		}
		if (Stage == WispRules_ElunaPickup)
		{
			return 0.3f;
		}
		if (Stage == WispRules_HealingLatched)
		{
			return 0.9f;
		}
		if (Stage == WispRules_ElunaShiftPickup)
		{
			return 0.2f;
		}
		if (Stage == WispRules_ElunaCCDrop)
		{
			return 0.4f;
		}
		if (Stage == WispRules_ElunaRRevive)
		{
			return 4.2f;
		}
		return 0.8f;
	}
}

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

void ABreachbornePlayerController::PawnLeavingGame()
{
	if (HasAuthority())
	{
		if (ABreachborneGameMode* BBGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABreachborneGameMode>() : nullptr)
		{
			BBGM->HandlePlayerDisconnect(this);
		}
	}

	Super::PawnLeavingGame();
}

void ABreachbornePlayerController::InitializeAbilitySmoke()
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("BBAbilitySmoke")))
	{
		return;
	}

	FParse::Value(FCommandLine::Get(), TEXT("BBAbilitySmokeIndex="), AbilitySmokeIndex);
	FParse::Value(FCommandLine::Get(), TEXT("BBAbilitySmokeHunter="), AbilitySmokeHunterID);
	bFourClientSmokeEnabled = FParse::Param(FCommandLine::Get(), TEXT("BBFourClientSmoke"));
	const int32 MaxSmokeIndex = bFourClientSmokeEnabled ? 4 : 2;
	if (AbilitySmokeIndex < 1 || AbilitySmokeIndex > MaxSmokeIndex
		|| AbilitySmokeHunterID < 1 || AbilitySmokeHunterID > 6)
	{
		UE_LOG(LogBreachborne, Error,
			TEXT("BB_ABILITY_SMOKE|CONFIG_FAIL|index=%d hunter=%d"),
			AbilitySmokeIndex, AbilitySmokeHunterID);
		return;
	}

	bAbilitySmokeEnabled = true;
	bDeathSmokeEnabled = FParse::Param(FCommandLine::Get(), TEXT("BBDeathSmoke"));
	bWispRulesSmokeEnabled = FParse::Param(FCommandLine::Get(), TEXT("BBWispRulesSmoke"));
	bHitSmokeEnabled = FParse::Param(FCommandLine::Get(), TEXT("BBHitSmoke"));
	bOutcomeSmokeEnabled = FParse::Param(FCommandLine::Get(), TEXT("BBOutcomeSmoke"));
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_ABILITY_SMOKE|CONFIG|index=%d hunter=%d four_client=%d death=%d wisp_rules=%d hit=%d outcome=%d"),
		AbilitySmokeIndex, AbilitySmokeHunterID, bFourClientSmokeEnabled ? 1 : 0,
		bDeathSmokeEnabled ? 1 : 0, bWispRulesSmokeEnabled ? 1 : 0,
		bHitSmokeEnabled ? 1 : 0, bOutcomeSmokeEnabled ? 1 : 0);
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
			if (!bDeathSmokeHealRequested && !bWispRulesSmokeEnabled)
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

FGameplayTag ABreachbornePlayerController::GetOutcomeSmokeInputTag(int32 Stage) const
{
	switch (Stage)
	{
	case 0: return BBGameplayTags::InputTag_RMB;
	case 1: return BBGameplayTags::InputTag_Shift;
	case 2: return BBGameplayTags::InputTag_Q;
	case 3: return BBGameplayTags::InputTag_R;
	default: return FGameplayTag();
	}
}

float ABreachbornePlayerController::GetOutcomeSmokeStageDuration(int32 Stage) const
{
	switch (Stage)
	{
	case 0: return 4.5f;
	case 1: return 3.0f;
	case 2: return 4.5f;
	case 3: return 6.0f;
	default: return 0.0f;
	}
}

void ABreachbornePlayerController::UpdateOutcomeSmoke(float DeltaTime, AHunterCharacter* Hunter, UBBAbilitySystemComponent* ASC)
{
	if (!bOutcomeSmokeEnabled || !Hunter || !ASC || bAbilitySmokeComplete)
	{
		return;
	}

	const float Separation = GetOutcomeSmokeSeparation(AbilitySmokeHunterID, OutcomeSmokeStage);
	const float AimX = AbilitySmokeIndex == 1
		? GetOutcomeSmokeAttackerAimX(AbilitySmokeHunterID, OutcomeSmokeStage, Separation)
		: -Separation * 0.5f;
	AbilitySmokeAimAccumulator += DeltaTime;
	if (AbilitySmokeAimAccumulator >= 0.05f)
	{
		AbilitySmokeAimAccumulator = 0.0f;
		const FVector AimLocation(AimX, 0.0f, Hunter->GetActorLocation().Z);
		Hunter->ServerSetAimLocation(AimLocation);
		const FRotator AimRotation = (AimLocation - Hunter->GetActorLocation()).Rotation();
		SetControlRotation(FRotator(0.0f, AimRotation.Yaw, 0.0f));
	}

	// Client 2 is a controlled target. The attacker controller owns preparation and reporting.
	if (AbilitySmokeIndex != 1)
	{
		return;
	}

	if (OutcomeSmokeStage >= 4)
	{
		bAbilitySmokeComplete = true;
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_OUTCOME_SMOKE|CLIENT_COMPLETE|index=1 hunter=%d stages=4 elapsed=%.2f"),
			AbilitySmokeHunterID, AbilitySmokeElapsed);
		return;
	}

	OutcomeSmokeStageElapsed += DeltaTime;
	if (!bOutcomeSmokeStagePrepared)
	{
		if (OutcomeSmokeStageElapsed < 0.0f)
		{
			return;
		}
		bOutcomeSmokeStagePrepared = true;
		bOutcomeSmokeStageConfirmed = false;
		OutcomeSmokeStageElapsed = 0.0f;
		ServerPrepareOutcomeSmoke(OutcomeSmokeStage);
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_OUTCOME_SMOKE|CLIENT_PREPARE|hunter=%d stage=%s"),
			AbilitySmokeHunterID, GetOutcomeSmokeStageName(OutcomeSmokeStage));
		return;
	}

	if (!bOutcomeSmokeStageConfirmed)
	{
		return;
	}

	if (!bOutcomeSmokeStageActivated && OutcomeSmokeStageElapsed >= 1.25f)
	{
		bOutcomeSmokeStageActivated = true;
		OutcomeSmokeStageElapsed = 0.0f;
		const FGameplayTag InputTag = GetOutcomeSmokeInputTag(OutcomeSmokeStage);
		const FString ActionName = FString::Printf(TEXT("OUTCOME_%s"), GetOutcomeSmokeStageName(OutcomeSmokeStage));
		ActivateAbilitySmokeInput(InputTag, *ActionName);
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_OUTCOME_SMOKE|CLIENT_ACTIVATE|hunter=%d stage=%s"),
			AbilitySmokeHunterID, GetOutcomeSmokeStageName(OutcomeSmokeStage));
		return;
	}

	if (!bOutcomeSmokeStageActivated)
	{
		return;
	}

	const FGameplayTag InputTag = GetOutcomeSmokeInputTag(OutcomeSmokeStage);
	const float RmbReleaseTime = AbilitySmokeHunterID == 4 ? 2.0f : 1.35f;
	if (!bOutcomeSmokeInputReleased
		&& ((OutcomeSmokeStage == 0 && OutcomeSmokeStageElapsed >= RmbReleaseTime)
			|| (OutcomeSmokeStage == 1 && OutcomeSmokeStageElapsed >= 1.0f)))
	{
		bOutcomeSmokeInputReleased = true;
		ASC->InputTagReleased(InputTag);
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_OUTCOME_SMOKE|CLIENT_RELEASE|hunter=%d stage=%s"),
			AbilitySmokeHunterID, GetOutcomeSmokeStageName(OutcomeSmokeStage));
	}

	if (!bOutcomeSmokeRepressed && OutcomeSmokeStage == 2 && OutcomeSmokeStageElapsed >= 1.4f
		&& (AbilitySmokeHunterID == 5 || AbilitySmokeHunterID == 6))
	{
		bOutcomeSmokeRepressed = true;
		ActivateAbilitySmokeInput(InputTag, TEXT("OUTCOME_Q_REPRESS"));
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_OUTCOME_SMOKE|CLIENT_REPRESS|hunter=%d stage=Q"), AbilitySmokeHunterID);
	}

	if (!bOutcomeSmokeFollowupActivated && OutcomeSmokeStage == 1
		&& AbilitySmokeHunterID == 5 && OutcomeSmokeStageElapsed >= 0.45f)
	{
		bOutcomeSmokeFollowupActivated = true;
		const bool bActivated = ActivateAbilitySmokeInput(InputTag, TEXT("OUTCOME_SHIFT_SECOND_CHARGE"));
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_OUTCOME_SMOKE|CLIENT_FOLLOWUP|hunter=5 stage=SHIFT action=SECOND_CHARGE success=%d"),
			bActivated ? 1 : 0);
	}

	if (OutcomeSmokeStageElapsed < GetOutcomeSmokeStageDuration(OutcomeSmokeStage))
	{
		return;
	}

	ASC->InputTagReleased(InputTag);
	ServerReportOutcomeSmoke(OutcomeSmokeStage);
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_OUTCOME_SMOKE|CLIENT_REPORT|hunter=%d stage=%s"),
		AbilitySmokeHunterID, GetOutcomeSmokeStageName(OutcomeSmokeStage));
	++OutcomeSmokeStage;
	OutcomeSmokeStageElapsed = 0.0f;
	bOutcomeSmokeStagePrepared = false;
	bOutcomeSmokeStageConfirmed = false;
	bOutcomeSmokeStageActivated = false;
	bOutcomeSmokeInputReleased = false;
	bOutcomeSmokeRepressed = false;
	bOutcomeSmokeFollowupActivated = false;
}

void ABreachbornePlayerController::ClientConfirmOutcomeSmokePrepared_Implementation(int32 Stage, bool bSuccess)
{
	if (!bOutcomeSmokeEnabled || AbilitySmokeIndex != 1 || Stage != OutcomeSmokeStage)
	{
		return;
	}

	if (bSuccess)
	{
		bOutcomeSmokeStageConfirmed = true;
		OutcomeSmokeStageElapsed = 0.0f;
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_OUTCOME_SMOKE|CLIENT_PREPARED|hunter=%d stage=%s"),
			AbilitySmokeHunterID, GetOutcomeSmokeStageName(Stage));
		return;
	}

	bOutcomeSmokeStagePrepared = false;
	bOutcomeSmokeStageConfirmed = false;
	OutcomeSmokeStageElapsed = -0.5f;
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_OUTCOME_SMOKE|CLIENT_PREPARE_RETRY|hunter=%d stage=%s"),
		AbilitySmokeHunterID, GetOutcomeSmokeStageName(Stage));
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
		AbilitySmokeFullLobbyElapsed = 0.0f;
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_ABILITY_SMOKE|PHASE|index=%d hunter=%d phase=%d"),
			AbilitySmokeIndex, AbilitySmokeHunterID, static_cast<int32>(Phase));
	}

	if (Phase == EMatchPhase::WaitingForPlayers)
	{
		const int32 DesiredTeam = bFourClientSmokeEnabled
			? (AbilitySmokeIndex - 1) / 2
			: AbilitySmokeIndex - 1;
		const int32 DesiredSlot = bFourClientSmokeEnabled
			? (AbilitySmokeIndex - 1) % 2
			: 0;
		const int32 RequiredPlayers = bFourClientSmokeEnabled ? 4 : 2;
		if (GS->GetLobbyActivePlayerCount() >= RequiredPlayers)
		{
			AbilitySmokeFullLobbyElapsed += DeltaTime;
		}
		else
		{
			AbilitySmokeFullLobbyElapsed = 0.0f;
		}
		if (AbilitySmokeLobbyRetry <= 0.0f)
		{
			if (PS->GetTeamID() != DesiredTeam || PS->GetLobbySlotIndex() != DesiredSlot)
			{
				RequestJoinLobbySlot(DesiredTeam, DesiredSlot);
			}
			else if (AbilitySmokeIndex == 1 && AbilitySmokePhaseElapsed >= 2.5f
				&& AbilitySmokeFullLobbyElapsed >= 1.5f)
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

	if (bOutcomeSmokeEnabled)
	{
		UpdateOutcomeSmoke(DeltaTime, Hunter, ASC);
		return;
	}

	AbilitySmokeAimAccumulator += DeltaTime;
	if (AbilitySmokeAimAccumulator >= 0.05f)
	{
		AbilitySmokeAimAccumulator = 0.0f;
		// Fire away from the opposing team so enemy CC cannot invalidate later activation checks.
		const bool bTeamZero = bFourClientSmokeEnabled
			? AbilitySmokeIndex <= 2
			: AbilitySmokeIndex == 1;
		const float TargetX = bTeamZero ? -1700.0f : 1700.0f;
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
	const bool bFourClientSmoke = FParse::Param(FCommandLine::Get(), TEXT("BBFourClientSmoke"));
	const int32 MaxSmokeIndex = bFourClientSmoke ? 4 : 2;
	if (!FParse::Param(FCommandLine::Get(), TEXT("BBAbilitySmoke"))
		|| SmokeIndex < 1 || SmokeIndex > MaxSmokeIndex)
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

	const bool bTeamZero = bFourClientSmoke ? SmokeIndex <= 2 : SmokeIndex == 1;
	const float X = bTeamZero ? -350.0f : 350.0f;
	const float Y = bFourClientSmoke ? ((SmokeIndex % 2) == 1 ? -250.0f : 250.0f) : 0.0f;
	const FVector TraceStart(X, Y, 5000.0f);
	const FVector TraceEnd(X, Y, -5000.0f);
	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BBAbilitySmokeGround), false, Hunter);
	const bool bGroundHit = GetWorld()->LineTraceSingleByChannel(
		GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	const float GroundZ = bGroundHit ? GroundHit.ImpactPoint.Z + 110.0f : 200.0f;
	const FVector TargetLocation(X, Y, GroundZ);
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
	if (UBBAbilitySystemComponent* ASC = PS->GetBBAbilitySystemComponent())
	{
		ASC->RefreshAbilitySpecReplication();
	}
	Hunter->ForceNetUpdate();
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_ABILITY_SMOKE|SERVER_PREPARED|index=%d hunter=%d team=%d slot=%d location=%s ground=%d"),
		SmokeIndex, PS->GetHunterID(), PS->GetTeamID(), PS->GetLobbySlotIndex(),
		*TargetLocation.ToCompactString(), bGroundHit ? 1 : 0);
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

void ABreachbornePlayerController::ServerPrepareOutcomeSmoke_Implementation(int32 Stage)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("BBAbilitySmoke"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("BBOutcomeSmoke"))
		|| AbilitySmokeServerIndex != 1 || Stage < 0 || Stage >= 4 || !GetWorld())
	{
		return;
	}

	ABreachbornePlayerController* TargetController = nullptr;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABreachbornePlayerController* Candidate = Cast<ABreachbornePlayerController>(It->Get());
		if (Candidate && Candidate->AbilitySmokeServerIndex == 2)
		{
			TargetController = Candidate;
			break;
		}
	}

	AHunterCharacter* SourceHunter = Cast<AHunterCharacter>(GetPawn());
	AHunterCharacter* TargetHunter = TargetController ? Cast<AHunterCharacter>(TargetController->GetPawn()) : nullptr;
	ABreachbornePlayerState* SourcePS = GetPlayerState<ABreachbornePlayerState>();
	ABreachbornePlayerState* TargetPS = TargetController
		? TargetController->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	UBBAbilitySystemComponent* SourceASC = GetBBASC();
	UBBAbilitySystemComponent* TargetASC = TargetController ? TargetController->GetBBASC() : nullptr;
	if (!SourceHunter || !TargetHunter || !SourcePS || !TargetPS || !SourceASC || !TargetASC
		|| !SourcePS->GetIsAlive() || !TargetPS->GetIsAlive())
	{
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_OUTCOME_SMOKE|SERVER_PREPARE_WAIT|stage=%s reason=missing_player"),
			GetOutcomeSmokeStageName(Stage));
		ClientConfirmOutcomeSmokePrepared(Stage, false);
		return;
	}

	GetWorldTimerManager().ClearTimer(OutcomeSmokeSampleTimerHandle);
	bOutcomeSmokeServerSampling = false;
	if (Stage > 0)
	{
		SourceASC->CancelAbilityByInputTag(GetOutcomeSmokeInputTag(Stage - 1));
		TArray<AActor*> OwnedActorsToDestroy;
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			if (!It->IsActorBeingDestroyed() && It->GetOwner() == SourceHunter)
			{
				OwnedActorsToDestroy.Add(*It);
			}
		}
		for (AActor* OwnedActor : OwnedActorsToDestroy)
		{
			OwnedActor->Destroy();
		}
	}

	FGameplayTagContainer TransientTargetTags;
	TransientTargetTags.AddTag(BBGameplayTags::State_Stunned);
	TransientTargetTags.AddTag(BBGameplayTags::State_Slowed);
	TransientTargetTags.AddTag(BBGameplayTags::State_Grounded);
	TransientTargetTags.AddTag(BBGameplayTags::State_AntiHeal);
	TransientTargetTags.AddTag(BBGameplayTags::State_Hooked);
	TransientTargetTags.AddTag(BBGameplayTags::State_Hudson_Hooked);
	TransientTargetTags.AddTag(BBGameplayTags::State_Crysta_Reverberation);
	TransientTargetTags.AddTag(BBGameplayTags::State_Void_Swapping);
	TransientTargetTags.AddTag(BBGameplayTags::State_Void_SingularityPulled);
	TargetASC->RemoveActiveEffectsWithGrantedTags(TransientTargetTags);
	for (const FGameplayTag& Tag : TransientTargetTags)
	{
		TargetASC->SetLooseGameplayTagCount(Tag, 0);
	}

	const int32 HunterID = SourcePS->GetHunterID();
	const bool bAlliedTarget = HunterID == 3 && (Stage == 2 || Stage == 3);
	TargetPS->SetTeamID(bAlliedTarget ? SourcePS->GetTeamID() : (SourcePS->GetTeamID() == 0 ? 1 : 0));

	const float Separation = GetOutcomeSmokeSeparation(HunterID, Stage);
	auto ResolveGroundLocation = [this](AHunterCharacter* Hunter, float X)
	{
		const FVector TraceStart(X, 0.0f, 5000.0f);
		const FVector TraceEnd(X, 0.0f, -5000.0f);
		FHitResult GroundHit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BBOutcomeSmokeGround), false, Hunter);
		const bool bGroundHit = GetWorld()->LineTraceSingleByChannel(
			GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
		return FVector(X, 0.0f, bGroundHit ? GroundHit.ImpactPoint.Z + 110.0f : 200.0f);
	};

	const FVector SourceLocation = ResolveGroundLocation(SourceHunter, Separation * -0.5f);
	FVector TargetLocation = ResolveGroundLocation(TargetHunter, Separation * 0.5f);
	TargetLocation.Z = SourceLocation.Z;
	const FVector SourceAimLocation(
		GetOutcomeSmokeAttackerAimX(HunterID, Stage, Separation), 0.0f, SourceLocation.Z);
	SourceHunter->ResetTransientCombatState();
	TargetHunter->ResetTransientCombatState();
	SourceASC->SetLooseGameplayTagCount(BBGameplayTags::State_Gliding, 0);
	TargetASC->SetLooseGameplayTagCount(BBGameplayTags::State_Gliding, 0);
	SourceHunter->SetActorLocation(SourceLocation, false, nullptr, ETeleportType::TeleportPhysics);
	TargetHunter->SetActorLocation(TargetLocation, false, nullptr, ETeleportType::TeleportPhysics);
	SourceHunter->SetActorRotation(FRotator::ZeroRotator);
	TargetHunter->SetActorRotation(FRotator(0.0f, 180.0f, 0.0f));
	SetControlRotation(FRotator::ZeroRotator);
	TargetController->SetControlRotation(FRotator(0.0f, 180.0f, 0.0f));
	SourceHunter->ServerSetAimLocation(SourceAimLocation);
	TargetHunter->ServerSetAimLocation(SourceLocation);
	for (AHunterCharacter* Hunter : { SourceHunter, TargetHunter })
	{
		if (UCharacterMovementComponent* Movement = Hunter->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Walking);
		}
		Hunter->ForceNetUpdate();
	}

	if (UBBHealthSet* SourceHealth = SourcePS->GetHealthSet())
	{
		SourceHealth->InitMaxHealth(50000.0f);
		SourceHealth->SetHealth(50000.0f);
		SourceHealth->SetShield(0.0f);
		SourcePS->UpdateHealthProxy();
	}
	float TargetInitialHealth = bAlliedTarget ? 40000.0f : 50000.0f;
	if (HunterID == 4 && Stage == 3)
	{
		TargetInitialHealth = 10000.0f;
	}
	if (UBBHealthSet* TargetHealth = TargetPS->GetHealthSet())
	{
		TargetHealth->InitMaxHealth(50000.0f);
		TargetHealth->SetHealth(TargetInitialHealth);
		TargetHealth->SetShield(0.0f);
		TargetPS->UpdateHealthProxy();
	}
	SourcePS->ForceNetUpdate();
	TargetPS->ForceNetUpdate();

	OutcomeSmokeServerStage = Stage;
	OutcomeSmokeTargetController = TargetController;
	OutcomeSmokeSourceStart = SourceLocation;
	OutcomeSmokeTargetStart = TargetLocation;
	OutcomeSmokeInitialTargetHealth = TargetInitialHealth;
	OutcomeSmokeMinTargetHealth = TargetInitialHealth;
	OutcomeSmokeMaxTargetHealth = TargetInitialHealth;
	OutcomeSmokeMaxSourceDisplacement = 0.0f;
	OutcomeSmokeMaxTargetDisplacement = 0.0f;
	OutcomeSmokeSourceStateMask = 0;
	OutcomeSmokeTargetStateMask = 0;
	OutcomeSmokeOwnedActorBaseline = CountOutcomeSmokeOwnedActors(GetWorld(), SourceHunter);
	OutcomeSmokeOwnedActorPeak = OutcomeSmokeOwnedActorBaseline;
	bOutcomeSmokeServerSampling = true;
	SampleOutcomeSmoke();
	GetWorldTimerManager().SetTimer(
		OutcomeSmokeSampleTimerHandle, this, &ABreachbornePlayerController::SampleOutcomeSmoke,
		0.05f, true, 0.05f);

	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_OUTCOME_SMOKE|SERVER_PREPARED|hunter=%d stage=%s separation=%.0f allied=%d target_health=%.1f owned_baseline=%d"),
		HunterID, GetOutcomeSmokeStageName(Stage), Separation, bAlliedTarget ? 1 : 0,
		TargetInitialHealth, OutcomeSmokeOwnedActorBaseline);
	ClientConfirmOutcomeSmokePrepared(Stage, true);
}

void ABreachbornePlayerController::SampleOutcomeSmoke()
{
	if (!bOutcomeSmokeServerSampling || !HasAuthority() || !GetWorld())
	{
		return;
	}

	AHunterCharacter* SourceHunter = Cast<AHunterCharacter>(GetPawn());
	ABreachbornePlayerController* TargetController = OutcomeSmokeTargetController.Get();
	AHunterCharacter* TargetHunter = TargetController ? Cast<AHunterCharacter>(TargetController->GetPawn()) : nullptr;
	ABreachbornePlayerState* TargetPS = TargetController
		? TargetController->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	UBBAbilitySystemComponent* SourceASC = GetBBASC();
	UBBAbilitySystemComponent* TargetASC = TargetController ? TargetController->GetBBASC() : nullptr;
	const UBBHealthSet* TargetHealth = TargetPS ? TargetPS->GetHealthSet() : nullptr;
	if (!SourceHunter || !TargetHunter || !TargetHealth)
	{
		return;
	}

	const float Health = TargetHealth->GetHealth();
	OutcomeSmokeMinTargetHealth = FMath::Min(OutcomeSmokeMinTargetHealth, Health);
	OutcomeSmokeMaxTargetHealth = FMath::Max(OutcomeSmokeMaxTargetHealth, Health);
	OutcomeSmokeMaxSourceDisplacement = FMath::Max(
		OutcomeSmokeMaxSourceDisplacement,
		FVector::Dist2D(OutcomeSmokeSourceStart, SourceHunter->GetActorLocation()));
	OutcomeSmokeMaxTargetDisplacement = FMath::Max(
		OutcomeSmokeMaxTargetDisplacement,
		FVector::Dist2D(OutcomeSmokeTargetStart, TargetHunter->GetActorLocation()));
	OutcomeSmokeSourceStateMask |= BuildOutcomeSmokeStateMask(SourceASC);
	OutcomeSmokeTargetStateMask |= BuildOutcomeSmokeStateMask(TargetASC);
	OutcomeSmokeOwnedActorPeak = FMath::Max(
		OutcomeSmokeOwnedActorPeak,
		CountOutcomeSmokeOwnedActors(GetWorld(), SourceHunter));
}

bool ABreachbornePlayerController::EvaluateOutcomeSmoke(int32 HunterID, int32 Stage) const
{
	const bool bDamage = OutcomeSmokeInitialTargetHealth - OutcomeSmokeMinTargetHealth > 0.5f;
	const bool bHeal = OutcomeSmokeMaxTargetHealth - OutcomeSmokeInitialTargetHealth > 0.5f;
	const bool bSourceMoved = OutcomeSmokeMaxSourceDisplacement > 100.0f;
	const bool bTargetMoved = OutcomeSmokeMaxTargetDisplacement > 40.0f;
	const bool bOwnedActor = OutcomeSmokeOwnedActorPeak > OutcomeSmokeOwnedActorBaseline;
	const bool bStunned = (OutcomeSmokeTargetStateMask & Outcome_Stunned) != 0;
	const bool bSlowed = (OutcomeSmokeTargetStateMask & Outcome_Slowed) != 0;
	const bool bGrounded = (OutcomeSmokeTargetStateMask & Outcome_Grounded) != 0;
	const bool bAntiHeal = (OutcomeSmokeTargetStateMask & Outcome_AntiHeal) != 0;
	const bool bHooked = (OutcomeSmokeTargetStateMask & (Outcome_Hooked | Outcome_HudsonHooked)) != 0;
	const bool bMarked = (OutcomeSmokeTargetStateMask & Outcome_Reverberation) != 0;
	const bool bCrystaEmpowered = (OutcomeSmokeSourceStateMask & Outcome_CrystaEmpowered) != 0;
	const bool bCrystaPrimaryShiftCooldown = (OutcomeSmokeSourceStateMask & Outcome_CrystaPrimaryShiftCooldown) != 0;
	const bool bCrystaSecondaryShiftCooldown = (OutcomeSmokeSourceStateMask & Outcome_CrystaSecondaryShiftCooldown) != 0;
	const bool bHudsonSpinning = (OutcomeSmokeSourceStateMask & Outcome_HudsonSpinning) != 0;
	const bool bHudsonSpunUp = (OutcomeSmokeSourceStateMask & Outcome_HudsonSpunUp) != 0;
	const bool bVoidPulled = (OutcomeSmokeTargetStateMask & Outcome_VoidPulled) != 0;

	switch (HunterID)
	{
	case 1:
		return Stage == 1 ? bSourceMoved : bDamage;
	case 2:
		if (Stage == 0) return bHooked && bTargetMoved;
		if (Stage == 1) return bSourceMoved && bDamage;
		if (Stage == 2) return bDamage && bStunned;
		return bDamage && bAntiHeal;
	case 3:
		if (Stage == 0) return bDamage && bStunned;
		if (Stage == 1) return bSourceMoved;
		return bHeal && bOwnedActor;
	case 4:
		if (Stage == 0) return bHudsonSpinning && bHudsonSpunUp;
		if (Stage == 1) return bSourceMoved;
		if (Stage == 2) return bDamage && bSlowed;
		return bHooked && bTargetMoved;
	case 5:
		if (Stage == 0) return bDamage && (bGrounded || bTargetMoved);
		if (Stage == 1) return bSourceMoved && bCrystaEmpowered
			&& bCrystaPrimaryShiftCooldown && bCrystaSecondaryShiftCooldown;
		return bDamage && bMarked;
	case 6:
		if (Stage == 0) return bDamage && bStunned;
		if (Stage == 1) return OutcomeSmokeMaxSourceDisplacement > 500.0f
			&& OutcomeSmokeMaxTargetDisplacement > 500.0f;
		if (Stage == 2) return bDamage;
		return bStunned && (bVoidPulled || bTargetMoved);
	default:
		return false;
	}
}

void ABreachbornePlayerController::ServerReportOutcomeSmoke_Implementation(int32 Stage)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("BBAbilitySmoke"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("BBOutcomeSmoke"))
		|| AbilitySmokeServerIndex != 1 || Stage != OutcomeSmokeServerStage)
	{
		return;
	}

	SampleOutcomeSmoke();
	bOutcomeSmokeServerSampling = false;
	GetWorldTimerManager().ClearTimer(OutcomeSmokeSampleTimerHandle);
	const ABreachbornePlayerState* SourcePS = GetPlayerState<ABreachbornePlayerState>();
	const int32 HunterID = SourcePS ? SourcePS->GetHunterID() : -1;
	const float Damage = FMath::Max(0.0f, OutcomeSmokeInitialTargetHealth - OutcomeSmokeMinTargetHealth);
	const float Healing = FMath::Max(0.0f, OutcomeSmokeMaxTargetHealth - OutcomeSmokeInitialTargetHealth);
	const int32 OwnedActorDelta = FMath::Max(0, OutcomeSmokeOwnedActorPeak - OutcomeSmokeOwnedActorBaseline);
	UBBAbilitySystemComponent* SourceASC = GetBBASC();
	float PrimaryCooldownBefore = 0.0f;
	float PrimaryCooldownAfter = 0.0f;
	float SecondaryCooldownBefore = 0.0f;
	float SecondaryCooldownAfter = 0.0f;
	bool bPassiveReductionSucceeded = true;
	if (HunterID == 5 && Stage == 1 && SourceASC)
	{
		PrimaryCooldownBefore = GetOutcomeCooldownRemaining(SourceASC, BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Primary);
		SecondaryCooldownBefore = GetOutcomeCooldownRemaining(SourceASC, BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Secondary);
		FGameplayEventData DetonationEvent;
		DetonationEvent.Instigator = GetPawn();
		SourceASC->HandleGameplayEvent(BBGameplayTags::Event_Crysta_ReverberationDetonated, &DetonationEvent);
		PrimaryCooldownAfter = GetOutcomeCooldownRemaining(SourceASC, BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Primary);
		SecondaryCooldownAfter = GetOutcomeCooldownRemaining(SourceASC, BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Secondary);
		bPassiveReductionSucceeded = PrimaryCooldownBefore > 1.5f && SecondaryCooldownBefore > 1.5f
			&& PrimaryCooldownBefore - PrimaryCooldownAfter >= 1.35f
			&& SecondaryCooldownBefore - SecondaryCooldownAfter >= 1.35f;
	}
	const bool bSuccess = EvaluateOutcomeSmoke(HunterID, Stage) && bPassiveReductionSucceeded;
	const FVector SourceEnd = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
	const ABreachbornePlayerController* TargetController = OutcomeSmokeTargetController.Get();
	const FVector TargetEnd = TargetController && TargetController->GetPawn()
		? TargetController->GetPawn()->GetActorLocation() : FVector::ZeroVector;
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_OUTCOME_SMOKE|SERVER_REPORT|hunter=%d stage=%s damage=%.2f healing=%.2f source_move=%.1f target_move=%.1f source_end=(%.1f,%.1f) target_end=(%.1f,%.1f) source_states=%d target_states=%d owned_peak=%d crysta_shift_cd=(%.2f,%.2f)->(%.2f,%.2f) passive_reduction=%d success=%d"),
		HunterID, GetOutcomeSmokeStageName(Stage), Damage, Healing,
		OutcomeSmokeMaxSourceDisplacement, OutcomeSmokeMaxTargetDisplacement,
		SourceEnd.X, SourceEnd.Y, TargetEnd.X, TargetEnd.Y,
		OutcomeSmokeSourceStateMask, OutcomeSmokeTargetStateMask, OwnedActorDelta,
		PrimaryCooldownBefore, SecondaryCooldownBefore, PrimaryCooldownAfter, SecondaryCooldownAfter,
		bPassiveReductionSucceeded ? 1 : 0, bSuccess ? 1 : 0);
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
	AHunterCharacter* VictimHunter = VictimController
		? Cast<AHunterCharacter>(VictimController->GetPawn()) : nullptr;
	ABreachbornePlayerState* VictimPS = VictimController
		? VictimController->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	UBBHealthSet* VictimHealth = VictimPS ? VictimPS->GetHealthSet() : nullptr;
	if (!KillerASC || !VictimASC || !VictimHunter || !VictimPS || !VictimHealth || !VictimPS->GetIsAlive())
	{
		UE_LOG(LogBreachborne, Error,
			TEXT("BB_DEATH_SMOKE|SERVER_FAIL|killer=%s victim=%s asc=%s health=%s alive=%d"),
			*GetNameSafe(this), *GetNameSafe(VictimController), *GetNameSafe(VictimASC),
			*GetNameSafe(VictimHealth), VictimPS && VictimPS->GetIsAlive() ? 1 : 0);
		return;
	}

	FActorSpawnParameters SentinelParams;
	SentinelParams.Owner = VictimHunter;
	SentinelParams.Instigator = VictimHunter;
	SentinelParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* CleanupSentinel = GetWorld()->SpawnActor<AActor>(
		AActor::StaticClass(), VictimHunter->GetActorLocation(), FRotator::ZeroRotator, SentinelParams);
	if (CleanupSentinel)
	{
		CleanupSentinel->SetLifeSpan(30.0f);
	}
	VictimASC->AddLooseGameplayTag(BBGameplayTags::State_Crysta_EmpoweredLMB);
	VictimASC->AddLooseGameplayTag(BBGameplayTags::State_Void_Empowered);

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
	const bool bOwnedActorCleaned = CleanupSentinel && CleanupSentinel->IsActorBeingDestroyed();
	const bool bTransientStatesCleaned =
		!VictimASC->HasMatchingGameplayTag(BBGameplayTags::State_Crysta_EmpoweredLMB)
		&& !VictimASC->HasMatchingGameplayTag(BBGameplayTags::State_Void_Empowered);
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_DEATH_SMOKE|SERVER_RESULT|victim=%s health_before=%.1f health_after=%.1f alive=%d pawn=%s wisp=%d cleanup=%d states=%d"),
		*GetNameSafe(VictimPS), HealthBefore, VictimHealth->GetHealth(), VictimPS->GetIsAlive() ? 1 : 0,
		*GetNameSafe(VictimController->GetPawn()), Wisp ? 1 : 0,
		bOwnedActorCleaned ? 1 : 0, bTransientStatesCleaned ? 1 : 0);

	if (Wisp && FParse::Param(FCommandLine::Get(), TEXT("BBWispRulesSmoke")))
	{
		StartWispRulesSmoke(VictimController, Wisp);
	}
}

void ABreachbornePlayerController::StartWispRulesSmoke(
	ABreachbornePlayerController* VictimController, ABBWispPawn* Wisp)
{
	if (!HasAuthority() || !VictimController || !Wisp || !GetWorld())
	{
		UE_LOG(LogBreachborne, Error, TEXT("BB_WISP_RULES|SERVER_FAIL|reason=invalid_start_state"));
		return;
	}

	const ABreachbornePlayerState* SourcePS = GetPlayerState<ABreachbornePlayerState>();
	if (!SourcePS || SourcePS->GetHunterID() != 3)
	{
		UE_LOG(LogBreachborne, Error,
			TEXT("BB_WISP_RULES|SERVER_FAIL|reason=eluna_required hunter=%d"),
			SourcePS ? SourcePS->GetHunterID() : -1);
		return;
	}

	WispRulesVictimController = VictimController;
	WispRulesWisp = Wisp;
	WispRulesSmokeStage = WispRules_Natural;
	WispRulesSmokePassed = 0;
	WispRulesNaturalHPDrain = 0.0f;
	ConfigureWispRulesSmokeStage();
	if (WispRulesSmokeStage == INDEX_NONE)
	{
		return;
	}
	GetWorldTimerManager().SetTimer(
		WispRulesSmokeTimerHandle, this, &ABreachbornePlayerController::UpdateWispRulesSmoke,
		0.1f, true, 0.1f);
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_WISP_RULES|SERVER_START|wisp=%s victim=%s scenarios=%d"),
		*GetNameSafe(Wisp), *GetNameSafe(VictimController), WispRules_Count);
}

void ABreachbornePlayerController::ConfigureWispRulesSmokeStage()
{
	ABBWispPawn* Wisp = WispRulesWisp.Get();
	ABreachbornePlayerController* VictimController = WispRulesVictimController.Get();
	AHunterCharacter* Eluna = Cast<AHunterCharacter>(GetPawn());
	ABreachbornePlayerState* VictimPS = VictimController
		? VictimController->GetPlayerState<ABreachbornePlayerState>() : nullptr;
	ABreachbornePlayerState* ElunaPS = GetPlayerState<ABreachbornePlayerState>();
	if (!Wisp || !VictimController || !VictimPS || !Eluna || !ElunaPS)
	{
		UE_LOG(LogBreachborne, Error,
			TEXT("BB_WISP_RULES|SERVER_FAIL|reason=stage_setup_missing stage=%s"),
			GetWispRulesSmokeStageName(WispRulesSmokeStage));
		FinishWispRulesSmoke();
		return;
	}

	auto MoveEluna = [Eluna](const FVector& Location)
	{
		Eluna->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
		if (UCharacterMovementComponent* Movement = Eluna->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(MOVE_Walking);
		}
		Eluna->ForceNetUpdate();
	};
	auto DestroyAllyBot = [this]()
	{
		if (ABBAllyBot* AllyBot = WispRulesAllyBot.Get())
		{
			AllyBot->Destroy();
		}
		WispRulesAllyBot.Reset();
	};

	const FVector WispLocation = Wisp->GetActorLocation();
	const FVector NearLocation = WispLocation + FVector(75.0f, 0.0f, 0.0f);
	const FVector FarLocation = WispLocation + FVector(1200.0f, 0.0f, 0.0f);
	UBBAbilitySystemComponent* ElunaASC = GetBBASC();

	switch (WispRulesSmokeStage)
	{
	case WispRules_Natural:
		Wisp->SetCarrier(nullptr);
		DestroyAllyBot();
		MoveEluna(FarLocation);
		break;
	case WispRules_Ally:
	{
		MoveEluna(FarLocation);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ABBAllyBot* AllyBot = GetWorld()->SpawnActor<ABBAllyBot>(
			ABBAllyBot::StaticClass(), WispLocation + FVector(0.0f, 75.0f, 0.0f),
			FRotator::ZeroRotator, Params);
		if (AllyBot)
		{
			AllyBot->SetTeamID(VictimPS->GetTeamID());
			AllyBot->ForceNetUpdate();
			WispRulesAllyBot = AllyBot;
		}
		break;
	}
	case WispRules_Contest:
		MoveEluna(NearLocation);
		break;
	case WispRules_Enemy:
		DestroyAllyBot();
		MoveEluna(NearLocation);
		break;
	case WispRules_Healing:
		MoveEluna(FarLocation);
		break;
	case WispRules_HealingLatched:
		MoveEluna(FarLocation);
		Wisp->ApplyHeal(0.01f);
		break;
	case WispRules_HealingContested:
		MoveEluna(NearLocation);
		break;
	case WispRules_CarriedContested:
		MoveEluna(NearLocation);
		Wisp->SetCarrier(Eluna);
		break;
	case WispRules_ElunaShiftPickup:
	{
		Wisp->SetCarrier(nullptr);
		VictimPS->SetTeamID(ElunaPS->GetTeamID());
		VictimPS->ForceNetUpdate();
		const FVector DashStart = WispLocation - FVector(500.0f, 0.0f, 0.0f);
		MoveEluna(DashStart);

		bool bCollected = false;
		if (ElunaASC)
		{
			for (FGameplayAbilitySpec& Spec : ElunaASC->GetActivatableAbilities())
			{
				UGA_Eluna_GroundDash* DashAbility = Cast<UGA_Eluna_GroundDash>(Spec.GetPrimaryInstance());
				if (DashAbility)
				{
					const FGameplayTagContainer* CooldownTags = DashAbility->GetCooldownTags();
					WispRulesShiftRefundTag = FGameplayTag();
					if (CooldownTags)
					{
						for (const FGameplayTag& CooldownTag : *CooldownTags)
						{
							WispRulesShiftRefundTag = CooldownTag;
							break;
						}
					}
					if (WispRulesShiftRefundTag.IsValid())
					{
						FGameplayTagContainer GrantedTags;
						GrantedTags.AddTag(WispRulesShiftRefundTag);
						GrantedTags.AddTag(BBGameplayTags::Cooldown_Dash);
						FGameplayEffectSpecHandle CooldownSpec = UBBGameplayAbility::BuildBBCooldownSpec(
							ElunaASC, 30.0f, GrantedTags);
						if (CooldownSpec.IsValid())
						{
							ElunaASC->ApplyGameplayEffectSpecToSelf(*CooldownSpec.Data.Get());
						}
					}
					bCollected = DashAbility->TryCollectWispAlongPath(
						DashStart, DashStart + FVector(700.0f, 0.0f, 0.0f));
					break;
				}
			}
		}
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_WISP_RULES|SERVER_ACTION|scenario=eluna_shift_pickup collected=%d"),
			bCollected ? 1 : 0);
		break;
	}
	case WispRules_ElunaPickup:
		Wisp->SetCarrier(nullptr);
		VictimPS->SetTeamID(ElunaPS->GetTeamID());
		VictimPS->ForceNetUpdate();
		if (ElunaASC)
		{
			ElunaASC->SetLooseGameplayTagCount(BBGameplayTags::State_Stunned, 0);
		}
		MoveEluna(NearLocation);
		break;
	case WispRules_ElunaCCDrop:
		if (ElunaASC)
		{
			ElunaASC->AddLooseGameplayTag(BBGameplayTags::State_Stunned);
		}
		break;
	case WispRules_ElunaRRevive:
		Wisp->SetCarrier(nullptr);
		VictimPS->SetTeamID(ElunaPS->GetTeamID());
		VictimPS->ForceNetUpdate();
		if (ElunaASC)
		{
			ElunaASC->SetLooseGameplayTagCount(BBGameplayTags::State_Stunned, 0);
			FGameplayTagContainer ElunaRCooldownTag;
			ElunaRCooldownTag.AddTag(BBGameplayTags::Cooldown_Hunter_Eluna_R);
			ElunaASC->RemoveActiveEffectsWithGrantedTags(ElunaRCooldownTag);
		}
		MoveEluna(WispLocation + FVector(600.0f, 0.0f, 0.0f));
		break;
	default:
		FinishWispRulesSmoke();
		return;
	}

	WispRulesSmokeStageElapsed = 0.0f;
	bWispRulesElunaRActivationRequested = false;
	WispRulesSmokeStartHP = Wisp->GetCurrentWispHP();
	WispRulesSmokeStartRez = Wisp->GetRezBarProgress();
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_WISP_RULES|SERVER_STAGE|scenario=%s hp=%.2f rez=%.3f"),
		GetWispRulesSmokeStageName(WispRulesSmokeStage),
		WispRulesSmokeStartHP, WispRulesSmokeStartRez);
}

void ABreachbornePlayerController::UpdateWispRulesSmoke()
{
	ABBWispPawn* Wisp = WispRulesWisp.Get();
	if (!HasAuthority() || WispRulesSmokeStage < 0 || WispRulesSmokeStage >= WispRules_Count)
	{
		UE_LOG(LogBreachborne, Error, TEXT("BB_WISP_RULES|SERVER_FAIL|reason=invalid_tick_state"));
		FinishWispRulesSmoke();
		return;
	}

	WispRulesSmokeStageElapsed += 0.1f;
	if (WispRulesSmokeStage == WispRules_ElunaRRevive)
	{
		if (!bWispRulesElunaRActivationRequested && WispRulesSmokeStageElapsed >= 0.3f)
		{
			bWispRulesElunaRActivationRequested = true;
			ClientActivateWispRulesElunaR();
		}

		ABreachbornePlayerController* VictimController = WispRulesVictimController.Get();
		const ABreachbornePlayerState* VictimPS = VictimController
			? VictimController->GetPlayerState<ABreachbornePlayerState>() : nullptr;
		const bool bRevived = VictimPS && VictimPS->GetIsAlive()
			&& Cast<AHunterCharacter>(VictimController->GetPawn()) != nullptr;
		if (bRevived)
		{
			++WispRulesSmokePassed;
			UE_LOG(LogBreachborne, Warning,
				TEXT("BB_WISP_RULES|SERVER_RESULT|scenario=eluna_r_revive revived=1 pawn=%s success=1"),
				*GetNameSafe(VictimController->GetPawn()));
			++WispRulesSmokeStage;
			FinishWispRulesSmoke();
			return;
		}

		if (WispRulesSmokeStageElapsed < GetWispRulesSmokeStageDuration(WispRulesSmokeStage))
		{
			return;
		}
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_WISP_RULES|SERVER_RESULT|scenario=eluna_r_revive revived=0 pawn=%s success=0"),
			*GetNameSafe(VictimController ? VictimController->GetPawn() : nullptr));

		++WispRulesSmokeStage;
		FinishWispRulesSmoke();
		return;
	}

	if (!Wisp)
	{
		UE_LOG(LogBreachborne, Error, TEXT("BB_WISP_RULES|SERVER_FAIL|reason=wisp_lost stage=%s"),
			GetWispRulesSmokeStageName(WispRulesSmokeStage));
		FinishWispRulesSmoke();
		return;
	}
	if (WispRulesSmokeStage == WispRules_Healing
		|| (WispRulesSmokeStage == WispRules_HealingContested
			&& WispRulesSmokeStageElapsed < GetWispRulesSmokeStageDuration(WispRulesSmokeStage) - 0.2f))
	{
		Wisp->ApplyHeal(20.0f);
	}

	if (WispRulesSmokeStageElapsed < GetWispRulesSmokeStageDuration(WispRulesSmokeStage))
	{
		return;
	}

	const float HPAfter = Wisp->GetCurrentWispHP();
	const float RezAfter = Wisp->GetRezBarProgress();
	const bool bSuccess = EvaluateWispRulesSmokeStage(HPAfter, RezAfter);
	if (WispRulesSmokeStage == WispRules_Natural)
	{
		WispRulesNaturalHPDrain = FMath::Max(0.0f, WispRulesSmokeStartHP - HPAfter);
	}
	if (bSuccess)
	{
		++WispRulesSmokePassed;
	}
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_WISP_RULES|SERVER_RESULT|scenario=%s hp_before=%.2f hp_after=%.2f rez_before=%.3f rez_after=%.3f carrier=%s success=%d"),
		GetWispRulesSmokeStageName(WispRulesSmokeStage),
		WispRulesSmokeStartHP, HPAfter, WispRulesSmokeStartRez, RezAfter,
		*GetNameSafe(Wisp->GetCarrier()), bSuccess ? 1 : 0);

	++WispRulesSmokeStage;
	if (WispRulesSmokeStage >= WispRules_Count)
	{
		FinishWispRulesSmoke();
		return;
	}
	ConfigureWispRulesSmokeStage();
}

void ABreachbornePlayerController::ClientActivateWispRulesElunaR_Implementation()
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("BBAbilitySmoke"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("BBWispRulesSmoke")))
	{
		return;
	}

	UBBAbilitySystemComponent* ASC = GetBBASC();
	const bool bActivated = ASC && ASC->TryActivateAbilityByInputTag(BBGameplayTags::InputTag_R);
	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_WISP_RULES|CLIENT_ACTIVATE|scenario=eluna_r_revive success=%d"),
		bActivated ? 1 : 0);
}

bool ABreachbornePlayerController::EvaluateWispRulesSmokeStage(float HPAfter, float RezAfter) const
{
	const ABBWispPawn* Wisp = WispRulesWisp.Get();
	const AHunterCharacter* Eluna = Cast<AHunterCharacter>(GetPawn());
	const UBBAbilitySystemComponent* ElunaASC = GetBBASC();
	const float HPDrain = WispRulesSmokeStartHP - HPAfter;
	const float RezGain = RezAfter - WispRulesSmokeStartRez;

	switch (WispRulesSmokeStage)
	{
	case WispRules_Natural:
		return HPDrain > 0.5f && RezGain <= 0.02f;
	case WispRules_Ally:
		return FMath::Abs(HPDrain) < 0.7f && RezGain > 0.05f;
	case WispRules_Contest:
		return HPDrain > 1.0f && RezGain <= 0.02f;
	case WispRules_Enemy:
		return HPDrain > 1.0f && HPDrain > WispRulesNaturalHPDrain * 2.0f && RezGain <= 0.02f;
	case WispRules_Healing:
		return FMath::Abs(HPDrain) < 0.7f && RezGain > 0.10f;
	case WispRules_HealingLatched:
		return FMath::Abs(HPDrain) < 0.7f && RezGain > 0.15f;
	case WispRules_HealingContested:
		return HPDrain > 1.0f && RezGain <= 0.02f;
	case WispRules_CarriedContested:
		return Wisp && Wisp->GetCarrier() == Eluna && FMath::Abs(HPDrain) < 0.7f && RezGain > 0.10f;
	case WispRules_ElunaShiftPickup:
		return Wisp && Wisp->GetCarrier() == Eluna
			&& WispRulesShiftRefundTag.IsValid()
			&& ElunaASC
			&& !ElunaASC->HasMatchingGameplayTag(WispRulesShiftRefundTag);
	case WispRules_ElunaPickup:
		return Wisp && Wisp->GetCarrier() == Eluna;
	case WispRules_ElunaCCDrop:
		return Wisp && Wisp->GetCarrier() == nullptr;
	default:
		return false;
	}
}

void ABreachbornePlayerController::FinishWispRulesSmoke()
{
	GetWorldTimerManager().ClearTimer(WispRulesSmokeTimerHandle);
	if (ABBAllyBot* AllyBot = WispRulesAllyBot.Get())
	{
		AllyBot->Destroy();
	}
	WispRulesAllyBot.Reset();

	ABBWispPawn* Wisp = WispRulesWisp.Get();
	ABreachbornePlayerController* VictimController = WispRulesVictimController.Get();
	AHunterCharacter* Eluna = Cast<AHunterCharacter>(GetPawn());
	if (Wisp)
	{
		Wisp->SetCarrier(nullptr);
	}
	if (UBBAbilitySystemComponent* ElunaASC = GetBBASC())
	{
		ElunaASC->SetLooseGameplayTagCount(BBGameplayTags::State_Stunned, 0);
	}
	if (Wisp && Eluna)
	{
		Eluna->SetActorLocation(
			Wisp->GetActorLocation() + FVector(1200.0f, 0.0f, 0.0f),
			false, nullptr, ETeleportType::TeleportPhysics);
		if (UCharacterMovementComponent* Movement = Eluna->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}
		Eluna->ForceNetUpdate();
	}

	UE_LOG(LogBreachborne, Warning,
		TEXT("BB_WISP_RULES|SERVER_COMPLETE|passed=%d total=%d"),
		WispRulesSmokePassed, WispRules_Count);

	if (Wisp && VictimController
		&& !GetWorldTimerManager().IsTimerActive(VictimController->DeathSmokeHealTimerHandle))
	{
		Wisp->ApplyHeal(20.0f);
		GetWorldTimerManager().SetTimer(
			VictimController->DeathSmokeHealTimerHandle,
			VictimController,
			&ABreachbornePlayerController::ApplyWispHealSmokeTick,
			0.2f, true, 0.0f);
		UE_LOG(LogBreachborne, Warning,
			TEXT("BB_DEATH_SMOKE|SERVER_HEAL_STARTED|controller=%s rules=1"),
			*VictimController->GetName());
	}

	WispRulesSmokeStage = INDEX_NONE;
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

void ABreachbornePlayerController::RequestSetStormEnabled(bool bEnabled)
{
	if (HasAuthority())
	{
		ServerRequestSetStormEnabled_Implementation(bEnabled);
	}
	else
	{
		ServerRequestSetStormEnabled(bEnabled);
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

void ABreachbornePlayerController::ServerRequestSetStormEnabled_Implementation(bool bEnabled)
{
	if (ABreachborneGameMode* BBGM = GetWorld() ? GetWorld()->GetAuthGameMode<ABreachborneGameMode>() : nullptr)
	{
		BBGM->RequestSetStormEnabled(this, bEnabled);
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
