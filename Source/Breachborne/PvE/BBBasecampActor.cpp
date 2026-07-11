#include "BBBasecampActor.h"

#include "AbilitySystemComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"

#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Core/BreachbornePlayerController.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Items/BBInventoryManager.h"

#include "EngineUtils.h"

namespace
{
	constexpr float PlatformTickInterval = 0.2f;
	constexpr float StationTickInterval = 0.1f;
	const FName ViveBeanID(TEXT("ViveBean"));
	const FName ViveBrewID(TEXT("ViveBrew"));
}

ABBBasecampActor::ABBBasecampActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BasecampMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BasecampMesh"));
	BasecampMesh->SetupAttachment(SceneRoot);

	PlatformZone = CreateDefaultSubobject<USphereComponent>(TEXT("PlatformZone"));
	PlatformZone->SetSphereRadius(CaptureRadius);
	PlatformZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	PlatformZone->ShapeColor = FColor(120, 255, 80);
	PlatformZone->SetupAttachment(SceneRoot);

	RecallLandingPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RecallLandingPoint"));
	RecallLandingPoint->SetupAttachment(SceneRoot);

	AnvilInteractZone = CreateDefaultSubobject<USphereComponent>(TEXT("AnvilInteractZone"));
	AnvilInteractZone->SetSphereRadius(250.0f);
	AnvilInteractZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	AnvilInteractZone->ShapeColor = FColor(255, 150, 0);
	AnvilInteractZone->SetupAttachment(SceneRoot);
	AnvilInteractZone->SetRelativeLocation(FVector(250.0f, 0.0f, 0.0f));

	CauldronInteractZone = CreateDefaultSubobject<USphereComponent>(TEXT("CauldronInteractZone"));
	CauldronInteractZone->SetSphereRadius(250.0f);
	CauldronInteractZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CauldronInteractZone->ShapeColor = FColor(0, 220, 255);
	CauldronInteractZone->SetupAttachment(SceneRoot);
	CauldronInteractZone->SetRelativeLocation(FVector(-250.0f, 0.0f, 0.0f));

	AnvilEditorLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ANVIL_LABEL"));
	AnvilEditorLabel->SetupAttachment(AnvilInteractZone);
	AnvilEditorLabel->SetText(FText::FromString(TEXT("ANVIL")));
	AnvilEditorLabel->SetTextRenderColor(FColor(255, 150, 0));
	AnvilEditorLabel->SetHorizontalAlignment(EHTA_Center);
	AnvilEditorLabel->SetWorldSize(80.0f);
	AnvilEditorLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f));
	AnvilEditorLabel->SetHiddenInGame(true);

	CauldronEditorLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CAULDRON_LABEL"));
	CauldronEditorLabel->SetupAttachment(CauldronInteractZone);
	CauldronEditorLabel->SetText(FText::FromString(TEXT("CAULDRON")));
	CauldronEditorLabel->SetTextRenderColor(FColor(0, 220, 255));
	CauldronEditorLabel->SetHorizontalAlignment(EHTA_Center);
	CauldronEditorLabel->SetWorldSize(80.0f);
	CauldronEditorLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f));
	CauldronEditorLabel->SetHiddenInGame(true);

	RecallEditorLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("RECALL_LANDING_LABEL"));
	RecallEditorLabel->SetupAttachment(RecallLandingPoint);
	RecallEditorLabel->SetText(FText::FromString(TEXT("RECALL LANDING")));
	RecallEditorLabel->SetTextRenderColor(FColor::White);
	RecallEditorLabel->SetHorizontalAlignment(EHTA_Center);
	RecallEditorLabel->SetWorldSize(64.0f);
	RecallEditorLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 140.0f));
	RecallEditorLabel->SetHiddenInGame(true);

	AnvilPlacementArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("AnvilPlacementArrow"));
	AnvilPlacementArrow->SetupAttachment(AnvilInteractZone);
	AnvilPlacementArrow->ArrowColor = FColor(255, 150, 0);
	AnvilPlacementArrow->ArrowSize = 1.5f;
	AnvilPlacementArrow->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	AnvilPlacementArrow->SetHiddenInGame(true);

	CauldronPlacementArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("CauldronPlacementArrow"));
	CauldronPlacementArrow->SetupAttachment(CauldronInteractZone);
	CauldronPlacementArrow->ArrowColor = FColor(0, 220, 255);
	CauldronPlacementArrow->ArrowSize = 1.5f;
	CauldronPlacementArrow->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	CauldronPlacementArrow->SetHiddenInGame(true);

	RecallPlacementArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("RecallPlacementArrow"));
	RecallPlacementArrow->SetupAttachment(RecallLandingPoint);
	RecallPlacementArrow->ArrowColor = FColor::White;
	RecallPlacementArrow->ArrowSize = 1.25f;
	RecallPlacementArrow->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	RecallPlacementArrow->SetHiddenInGame(true);

	BasecampStateLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("BasecampStateLabel"));
	BasecampStateLabel->SetupAttachment(SceneRoot);
	BasecampStateLabel->SetText(FText::FromString(TEXT("NEUTRAL")));
	BasecampStateLabel->SetTextRenderColor(FColor::White);
	BasecampStateLabel->SetHorizontalAlignment(EHTA_Center);
	BasecampStateLabel->SetWorldSize(72.0f);
	BasecampStateLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 230.0f));

	AnvilPromptLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("AnvilPromptLabel"));
	AnvilPromptLabel->SetupAttachment(AnvilInteractZone);
	AnvilPromptLabel->SetText(FText::FromString(TEXT("E: REPAIR")));
	AnvilPromptLabel->SetTextRenderColor(FColor(255, 150, 0));
	AnvilPromptLabel->SetHorizontalAlignment(EHTA_Center);
	AnvilPromptLabel->SetWorldSize(46.0f);
	AnvilPromptLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));

	CauldronPromptLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CauldronPromptLabel"));
	CauldronPromptLabel->SetupAttachment(CauldronInteractZone);
	CauldronPromptLabel->SetText(FText::FromString(TEXT("E: BREW VIVE")));
	CauldronPromptLabel->SetTextRenderColor(FColor(0, 220, 255));
	CauldronPromptLabel->SetHorizontalAlignment(EHTA_Center);
	CauldronPromptLabel->SetWorldSize(46.0f);
	CauldronPromptLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));
}

void ABBBasecampActor::BeginPlay()
{
	Super::BeginPlay();

	PlatformZone->SetSphereRadius(CaptureRadius);

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(PlatformTickHandle, this, &ABBBasecampActor::PlatformTick, PlatformTickInterval, true);
	}

	UpdateRuntimeIndicators();
}

void ABBBasecampActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABBBasecampActor, OwningTeamID);
	DOREPLIFETIME(ABBBasecampActor, CaptureTeamID);
	DOREPLIFETIME(ABBBasecampActor, CaptureProgress);
	DOREPLIFETIME(ABBBasecampActor, bContested);
	DOREPLIFETIME(ABBBasecampActor, ActiveInteractionType);
	DOREPLIFETIME(ABBBasecampActor, StationProgress);
	DOREPLIFETIME(ABBBasecampActor, bDestroyed);
	DOREPLIFETIME(ABBBasecampActor, ActiveInteractingPlayer);
}

void ABBBasecampActor::ServerRequestInteract(ABreachbornePlayerController* InteractingController)
{
	if (!HasAuthority() || bDestroyed || !InteractingController || ActiveInteractionType != EBBBasecampInteractionType::None)
	{
		return;
	}

	ABreachbornePlayerState* PlayerState = InteractingController->GetPlayerState<ABreachbornePlayerState>();
	if (!CanPlayerUseStations(PlayerState))
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BasecampStation: [%s] station use rejected - invalid owner/team"), *GetName());
		return;
	}

	EBBBasecampInteractionType NearestType = EBBBasecampInteractionType::None;
	float NearestDistSq = FLT_MAX;
	if (!GetNearestStationForPlayer(PlayerState, NearestType, NearestDistSq))
	{
		return;
	}

	if (NearestType == EBBBasecampInteractionType::RepairArmor)
	{
		const int32 Cost = GetArmorRepairCost(PlayerState);
		const bool bCanRepair = Cost > 0 && PlayerState->GetInventoryData().Gold >= Cost;
		if (!bCanRepair)
		{
			UE_LOG(LogBreachborne, Warning, TEXT("BasecampStation: [%s] anvil rejected for %s (cost=%d, gold=%d)"),
				*GetName(), *PlayerState->GetPlayerName(), Cost, PlayerState->GetInventoryData().Gold);
			return;
		}
	}
	else if (NearestType == EBBBasecampInteractionType::BrewVive)
	{
		if (!FBBInventoryManager::HasConsumable(PlayerState, ViveBeanID, 1))
		{
			UE_LOG(LogBreachborne, Warning, TEXT("BasecampStation: [%s] cauldron rejected for %s (missing ViveBean)"),
				*GetName(), *PlayerState->GetPlayerName());
			return;
		}
	}

	BeginStationInteraction(PlayerState, NearestType);
}

FTransform ABBBasecampActor::GetRecallLandingTransform() const
{
	return RecallLandingPoint ? RecallLandingPoint->GetComponentTransform() : GetActorTransform();
}

bool ABBBasecampActor::CanTeamRecallHere(int32 TeamID) const
{
	return !bDestroyed && TeamID != -1 && OwningTeamID == TeamID;
}

void ABBBasecampActor::ClearOwnershipForTeam(int32 TeamID)
{
	if (!HasAuthority() || TeamID == -1 || OwningTeamID != TeamID)
	{
		return;
	}

	OwningTeamID = -1;
	CaptureTeamID = -1;
	CaptureProgress = 0.0f;
	bContested = false;
	CancelStationInteraction(TEXT("ownership replaced"));
	UpdateRuntimeIndicators();
	ForceNetUpdate();

	UE_LOG(LogBreachborne, Log, TEXT("BasecampCapture: [%s] ownership cleared for team %d"), *GetName(), TeamID);
}

bool ABBBasecampActor::IsActorInsideAnyStation(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	const FVector ActorLocation = Actor->GetActorLocation();
	const bool bInsideAnvil = AnvilInteractZone
		&& FVector::Dist2D(ActorLocation, AnvilInteractZone->GetComponentLocation()) <= AnvilInteractZone->GetScaledSphereRadius();
	const bool bInsideCauldron = CauldronInteractZone
		&& FVector::Dist2D(ActorLocation, CauldronInteractZone->GetComponentLocation()) <= CauldronInteractZone->GetScaledSphereRadius();

	return bInsideAnvil || bInsideCauldron;
}

FVector ABBBasecampActor::GetPlatformWorldLocation() const
{
	return PlatformZone ? PlatformZone->GetComponentLocation() : GetActorLocation();
}

FVector ABBBasecampActor::GetAnvilWorldLocation() const
{
	return AnvilInteractZone ? AnvilInteractZone->GetComponentLocation() : GetActorLocation();
}

FVector ABBBasecampActor::GetCauldronWorldLocation() const
{
	return CauldronInteractZone ? CauldronInteractZone->GetComponentLocation() : GetActorLocation();
}

float ABBBasecampActor::GetPlatformRadius() const
{
	return PlatformZone ? PlatformZone->GetScaledSphereRadius() : CaptureRadius;
}

float ABBBasecampActor::GetAnvilRadius() const
{
	return AnvilInteractZone ? AnvilInteractZone->GetScaledSphereRadius() : 0.0f;
}

float ABBBasecampActor::GetCauldronRadius() const
{
	return CauldronInteractZone ? CauldronInteractZone->GetScaledSphereRadius() : 0.0f;
}

void ABBBasecampActor::PlatformTick()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World || bDestroyed)
	{
		return;
	}

	TMap<int32, TArray<ABreachbornePlayerState*>> TeamsInside;

	for (TActorIterator<ABreachbornePlayerState> It(World); It; ++It)
	{
		ABreachbornePlayerState* PlayerState = *It;
		if (!PlayerState || !PlayerState->GetIsAlive() || PlayerState->GetTeamID() == -1)
		{
			continue;
		}

		if (IsPlayerInsideZone(PlayerState, PlatformZone))
		{
			TeamsInside.FindOrAdd(PlayerState->GetTeamID()).Add(PlayerState);
		}
	}

	bContested = TeamsInside.Num() > 1;
	if (bContested)
	{
		UpdateRuntimeIndicators();
		UE_LOG(LogBreachborne, Verbose, TEXT("BasecampCapture: [%s] contested"), *GetName());
		return;
	}

	if (TeamsInside.IsEmpty())
	{
		CaptureProgress = FMath::Max(0.0f, CaptureProgress - 0.05f);
		if (CaptureProgress <= 0.0f)
		{
			CaptureTeamID = -1;
		}
		UpdateRuntimeIndicators();
		return;
	}

	const int32 TeamID = TeamsInside.begin()->Key;
	const TArray<ABreachbornePlayerState*>& Players = TeamsInside.begin()->Value;

	if (TeamID == OwningTeamID)
	{
		for (ABreachbornePlayerState* PlayerState : Players)
		{
			ApplyPlatformHeal(PlayerState, PlatformTickInterval);
		}
		UpdateRuntimeIndicators();
		return;
	}

	if (CaptureTeamID != TeamID)
	{
		CaptureTeamID = TeamID;
		CaptureProgress = 0.0f;
	}

	const float TickProgress = (PlatformTickInterval / CaptureDuration) * FMath::Min(Players.Num(), 2);
	CaptureProgress = FMath::Clamp(CaptureProgress + TickProgress, 0.0f, 1.0f);

	if (CaptureProgress >= 1.0f)
	{
		ClaimOwnershipForTeam(TeamID);

		UE_LOG(LogBreachborne, Log, TEXT("BasecampCapture: [%s] captured by team %d"), *GetName(), OwningTeamID);
		OnBasecampCaptured.Broadcast(this, OwningTeamID);
	}
	else
	{
		UpdateRuntimeIndicators();
	}
}

void ABBBasecampActor::StationTick()
{
	if (!HasAuthority() || !ActiveInteractingPlayer)
	{
		CancelStationInteraction(TEXT("missing interactor"));
		return;
	}

	if (!CanPlayerUseStations(ActiveInteractingPlayer))
	{
		CancelStationInteraction(TEXT("invalid owner/team"));
		return;
	}

	USphereComponent* RequiredZone = ActiveInteractionType == EBBBasecampInteractionType::RepairArmor
		? AnvilInteractZone
		: CauldronInteractZone;

	if (!IsPlayerInsideZone(ActiveInteractingPlayer, RequiredZone, StationCancelLeeway))
	{
		CancelStationInteraction(TEXT("left station radius"));
		return;
	}

	APawn* Pawn = ActiveInteractingPlayer->GetPawn();
	if (!Pawn || FVector::Dist2D(Pawn->GetActorLocation(), StationStartLocation) > 80.0f)
	{
		CancelStationInteraction(TEXT("moved during channel"));
		return;
	}

	if (bContested)
	{
		CancelStationInteraction(TEXT("basecamp contested"));
		return;
	}

	StationElapsed += StationTickInterval;
	StationProgress = FMath::Clamp(StationElapsed / GetChannelDuration(ActiveInteractionType), 0.0f, 1.0f);
	UpdateRuntimeIndicators();

	if (StationProgress >= 1.0f)
	{
		CompleteStationInteraction();
	}
}

void ABBBasecampActor::OnRep_BasecampState()
{
	ApplyDestroyedPresentation();
	UpdateRuntimeIndicators();
}

void ABBBasecampActor::BeginStationInteraction(ABreachbornePlayerState* PlayerState, EBBBasecampInteractionType InteractionType)
{
	if (!PlayerState || InteractionType == EBBBasecampInteractionType::None)
	{
		return;
	}

	APawn* Pawn = PlayerState->GetPawn();
	if (!Pawn)
	{
		return;
	}

	ActiveInteractingPlayer = PlayerState;
	ActiveInteractionType = InteractionType;
	StationStartLocation = Pawn->GetActorLocation();
	StationElapsed = 0.0f;
	StationProgress = 0.0f;

	if (UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTag(BBGameplayTags::State_Vulnerable);
		InteractorHealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(UBBHealthSet::GetHealthAttribute())
			.AddUObject(this, &ABBBasecampActor::OnInteractorHealthChanged);
	}

	GetWorldTimerManager().SetTimer(StationTickHandle, this, &ABBBasecampActor::StationTick, StationTickInterval, true);

	UE_LOG(LogBreachborne, Log, TEXT("BasecampStation: [%s] %s started %s"),
		*GetName(),
		*PlayerState->GetPlayerName(),
		InteractionType == EBBBasecampInteractionType::RepairArmor ? TEXT("anvil repair") : TEXT("vive brewing"));
	UpdateRuntimeIndicators();
}

void ABBBasecampActor::CancelStationInteraction(const TCHAR* Reason)
{
	if (!HasAuthority() || ActiveInteractionType == EBBBasecampInteractionType::None)
	{
		return;
	}

	UE_LOG(LogBreachborne, Log, TEXT("BasecampStation: [%s] station interaction cancelled (%s)"), *GetName(), Reason ? Reason : TEXT("unknown"));
	ClearStationInteractionState();
}

void ABBBasecampActor::CompleteStationInteraction()
{
	if (!HasAuthority() || !ActiveInteractingPlayer)
	{
		ClearStationInteractionState();
		return;
	}

	if (ActiveInteractionType == EBBBasecampInteractionType::RepairArmor)
	{
		const int32 Cost = GetArmorRepairCost(ActiveInteractingPlayer);
		if (Cost > 0 && FBBInventoryManager::TrySpendGold(ActiveInteractingPlayer, Cost))
		{
			if (FBBInventoryManager::RepairArmorDurability(ActiveInteractingPlayer, ArmorRepairFraction))
			{
				UE_LOG(LogBreachborne, Log, TEXT("BasecampStation: [%s] %s completed anvil repair for %dg"),
					*GetName(), *ActiveInteractingPlayer->GetPlayerName(), Cost);
			}
			else
			{
				FBBInventoryManager::AddGold(ActiveInteractingPlayer, Cost);
				UE_LOG(LogBreachborne, Warning, TEXT("BasecampStation: [%s] anvil repair failed on completion; refunded %dg"),
					*GetName(), Cost);
			}
		}
		else
		{
			UE_LOG(LogBreachborne, Warning, TEXT("BasecampStation: [%s] anvil repair failed on completion"), *GetName());
		}
	}
	else if (ActiveInteractionType == EBBBasecampInteractionType::BrewVive)
	{
		if (FBBInventoryManager::ConsumeConsumable(ActiveInteractingPlayer, ViveBeanID, 1))
		{
			if (FBBInventoryManager::AddConsumableStack(ActiveInteractingPlayer, ViveBrewID, 1))
			{
				UE_LOG(LogBreachborne, Log, TEXT("BasecampStation: [%s] %s brewed ViveBrew"),
					*GetName(), *ActiveInteractingPlayer->GetPlayerName());
			}
			else
			{
				FBBInventoryManager::AddConsumableStack(ActiveInteractingPlayer, ViveBeanID, 1);
				UE_LOG(LogBreachborne, Warning, TEXT("BasecampStation: [%s] vive brew failed on completion; refunded ViveBean"), *GetName());
			}
		}
		else
		{
			UE_LOG(LogBreachborne, Warning, TEXT("BasecampStation: [%s] vive brew failed on completion"), *GetName());
		}
	}

	ClearStationInteractionState();
}

void ABBBasecampActor::ClearStationInteractionState()
{
	GetWorldTimerManager().ClearTimer(StationTickHandle);

	if (ActiveInteractingPlayer)
	{
		if (UAbilitySystemComponent* ASC = ActiveInteractingPlayer->GetAbilitySystemComponent())
		{
			ASC->RemoveLooseGameplayTag(BBGameplayTags::State_Vulnerable);
			ASC->GetGameplayAttributeValueChangeDelegate(UBBHealthSet::GetHealthAttribute()).Remove(InteractorHealthHandle);
		}
	}

	InteractorHealthHandle.Reset();
	ActiveInteractingPlayer = nullptr;
	ActiveInteractionType = EBBBasecampInteractionType::None;
	StationProgress = 0.0f;
	StationElapsed = 0.0f;
	StationStartLocation = FVector::ZeroVector;
	UpdateRuntimeIndicators();
}

bool ABBBasecampActor::CanPlayerUseStations(const ABreachbornePlayerState* PlayerState) const
{
	return !bDestroyed
		&& PlayerState
		&& PlayerState->GetIsAlive()
		&& PlayerState->GetTeamID() != -1
		&& PlayerState->GetTeamID() == OwningTeamID;
}

bool ABBBasecampActor::IsPlayerInsideZone(const ABreachbornePlayerState* PlayerState, const USphereComponent* Zone, float RadiusScale) const
{
	if (!PlayerState || !Zone)
	{
		return false;
	}

	const APawn* Pawn = PlayerState->GetPawn();
	if (!Pawn)
	{
		return false;
	}

	const float Radius = Zone->GetScaledSphereRadius() * RadiusScale;
	return FVector::Dist2D(Pawn->GetActorLocation(), Zone->GetComponentLocation()) <= Radius;
}

bool ABBBasecampActor::GetNearestStationForPlayer(const ABreachbornePlayerState* PlayerState, EBBBasecampInteractionType& OutType, float& OutDistSq) const
{
	OutType = EBBBasecampInteractionType::None;
	OutDistSq = FLT_MAX;

	if (!PlayerState || !PlayerState->GetPawn())
	{
		return false;
	}

	const FVector PlayerLocation = PlayerState->GetPawn()->GetActorLocation();
	if (IsPlayerInsideZone(PlayerState, AnvilInteractZone))
	{
		const float DistSq = FVector::DistSquared2D(PlayerLocation, AnvilInteractZone->GetComponentLocation());
		if (DistSq < OutDistSq)
		{
			OutDistSq = DistSq;
			OutType = EBBBasecampInteractionType::RepairArmor;
		}
	}

	if (IsPlayerInsideZone(PlayerState, CauldronInteractZone))
	{
		const float DistSq = FVector::DistSquared2D(PlayerLocation, CauldronInteractZone->GetComponentLocation());
		if (DistSq < OutDistSq)
		{
			OutDistSq = DistSq;
			OutType = EBBBasecampInteractionType::BrewVive;
		}
	}

	return OutType != EBBBasecampInteractionType::None;
}

float ABBBasecampActor::GetChannelDuration(EBBBasecampInteractionType InteractionType) const
{
	switch (InteractionType)
	{
	case EBBBasecampInteractionType::RepairArmor:
		return FMath::Max(0.1f, AnvilChannelDuration);
	case EBBBasecampInteractionType::BrewVive:
		return FMath::Max(0.1f, CauldronChannelDuration);
	default:
		return 1.0f;
	}
}

int32 ABBBasecampActor::GetArmorRepairCost(const ABreachbornePlayerState* PlayerState) const
{
	if (!PlayerState || !PlayerState->GetInventoryData().Armor.HasArmor())
	{
		return 0;
	}

	switch (PlayerState->GetInventoryData().Armor.Tier)
	{
	case EArmorTier::White:
		return 100;
	case EArmorTier::Green:
		return 200;
	case EArmorTier::Blue:
		return 500;
	case EArmorTier::Purple:
		return 1000;
	default:
		return 0;
	}
}

void ABBBasecampActor::ApplyPlatformHeal(ABreachbornePlayerState* PlayerState, float DeltaSeconds)
{
	if (!PlayerState || OwnedHealPerSecond <= 0.0f)
	{
		return;
	}

	UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	UGameplayEffect* HealGE = NewObject<UGameplayEffect>(this, TEXT("GE_BasecampPlatformHeal"));
	HealGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& Mod = HealGE->Modifiers.AddDefaulted_GetRef();
	Mod.Attribute = UBBHealthSet::GetIncomingHealingAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(OwnedHealPerSecond * DeltaSeconds));

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	ASC->ApplyGameplayEffectToSelf(HealGE, 1.0f, Context);
}

void ABBBasecampActor::OnInteractorHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue < Data.OldValue)
	{
		CancelStationInteraction(TEXT("took damage"));
	}
}

void ABBBasecampActor::ClaimOwnershipForTeam(int32 TeamID)
{
	if (!HasAuthority() || TeamID == -1)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		for (TActorIterator<ABBBasecampActor> It(World); It; ++It)
		{
			ABBBasecampActor* OtherBasecamp = *It;
			if (OtherBasecamp && OtherBasecamp != this)
			{
				OtherBasecamp->ClearOwnershipForTeam(TeamID);
			}
		}
	}

	OwningTeamID = TeamID;
	CaptureProgress = 1.0f;
	CaptureTeamID = TeamID;
	bContested = false;
	UpdateRuntimeIndicators();
	ForceNetUpdate();
}

void ABBBasecampActor::UpdateRuntimeIndicators()
{
	if (BasecampStateLabel)
	{
		FString StateText;
		FColor StateColor = FColor::White;
		if (bDestroyed)
		{
			StateText = TEXT("DESTROYED");
			StateColor = FColor::Red;
		}
		else if (bContested)
		{
			StateText = TEXT("CONTESTED");
			StateColor = FColor::Red;
		}
		else if (OwningTeamID != -1)
		{
			StateText = FString::Printf(TEXT("TEAM %d CAMP"), OwningTeamID);
			StateColor = FColor(120, 255, 80);
		}
		else if (CaptureTeamID != -1 && CaptureProgress > 0.0f)
		{
			StateText = FString::Printf(TEXT("CAPTURE T%d %.0f%%"), CaptureTeamID, CaptureProgress * 100.0f);
			StateColor = FColor::Yellow;
		}
		else
		{
			StateText = TEXT("NEUTRAL CAMP");
		}

		if (ActiveInteractionType != EBBBasecampInteractionType::None)
		{
			const TCHAR* InteractionName = ActiveInteractionType == EBBBasecampInteractionType::RepairArmor
				? TEXT("REPAIR")
				: TEXT("BREW");
			StateText += FString::Printf(TEXT("\n%s %.0f%%"), InteractionName, StationProgress * 100.0f);
		}

		BasecampStateLabel->SetText(FText::FromString(StateText));
		BasecampStateLabel->SetTextRenderColor(StateColor);
	}

	if (AnvilPromptLabel)
	{
		AnvilPromptLabel->SetText(FText::FromString(TEXT("E: REPAIR")));
	}

	if (CauldronPromptLabel)
	{
		CauldronPromptLabel->SetText(FText::FromString(TEXT("E: BREW VIVE")));
	}
}

void ABBBasecampActor::ApplyPowerDestruction_Implementation(const FBBPowerDestructionContext& Context)
{
	if (!HasAuthority() || bDestroyed)
	{
		return;
	}

	bDestroyed = true;
	OwningTeamID = -1;
	CaptureTeamID = -1;
	CaptureProgress = 0.0f;
	bContested = false;
	CancelStationInteraction(TEXT("basecamp destroyed"));
	GetWorldTimerManager().ClearTimer(PlatformTickHandle);

	ApplyDestroyedPresentation();

	UpdateRuntimeIndicators();
	ForceNetUpdate();
	UE_LOG(LogBreachborne, Log, TEXT("BasecampCapture: [%s] destroyed by %s"), *GetName(), *GetNameSafe(Context.InstigatorActor));
}

void ABBBasecampActor::ApplyDestroyedPresentation()
{
	if (!bDestroyed)
	{
		return;
	}

	if (PlatformZone)
	{
		PlatformZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (AnvilInteractZone)
	{
		AnvilInteractZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (CauldronInteractZone)
	{
		CauldronInteractZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (BasecampMesh)
	{
		BasecampMesh->SetVisibility(false, true);
		BasecampMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (AnvilPromptLabel)
	{
		AnvilPromptLabel->SetVisibility(false, true);
	}
	if (CauldronPromptLabel)
	{
		CauldronPromptLabel->SetVisibility(false, true);
	}
}
