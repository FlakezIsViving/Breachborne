#include "BreachbornePlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Abilities/BBCombatSet.h"
#include "Breachborne/Abilities/BBMovementSet.h"

ABreachbornePlayerState::ABreachbornePlayerState()
{
	// Create ASC — lives on PlayerState, survives pawn death/respawn
	AbilitySystemComponent = CreateDefaultSubobject<UBBAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Create attribute sets — ASC auto-discovers them via GetObjectsWithOuter
	HealthSet = CreateDefaultSubobject<UBBHealthSet>(TEXT("HealthSet"));
	CombatSet = CreateDefaultSubobject<UBBCombatSet>(TEXT("CombatSet"));
	MovementSet = CreateDefaultSubobject<UBBMovementSet>(TEXT("MovementSet"));

	// Cache the ASC on each AttributeSet while the association is guaranteed correct.
	HealthSet->SetCachedAbilitySystemComponent(AbilitySystemComponent);
	CombatSet->SetCachedAbilitySystemComponent(AbilitySystemComponent);
	MovementSet->SetCachedAbilitySystemComponent(AbilitySystemComponent);

	// Use the registered subobject list for replication.
	// This prevents UE's default ReplicateSubobjects() from creating child replicators
	// that can misroute property updates between PlayerState instances.
	// All subobjects that need NetGUIDs are registered in PostInitializeComponents().
	bReplicateUsingRegisteredSubObjectList = true;

	// Initialize backpack with 4 empty slots
	Rep_InventoryData.InitBackpack(4);

	// Higher update frequency for GAS responsiveness
	SetNetUpdateFrequency(100.0f);
}

void ABreachbornePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABreachbornePlayerState, TeamID);
	DOREPLIFETIME(ABreachbornePlayerState, HunterID);
	DOREPLIFETIME(ABreachbornePlayerState, Level);
	DOREPLIFETIME(ABreachbornePlayerState, XP);
	DOREPLIFETIME(ABreachbornePlayerState, Kills);
	DOREPLIFETIME(ABreachbornePlayerState, bIsAlive);
	DOREPLIFETIME(ABreachbornePlayerState, bReadyForMatch);
	DOREPLIFETIME(ABreachbornePlayerState, bHunterLocked);
	DOREPLIFETIME(ABreachbornePlayerState, LobbySlotIndex);
	DOREPLIFETIME(ABreachbornePlayerState, bIsLobbySpectator);

	// Proxy attribute replication — replicated on the actor, NOT on AttributeSet subobjects
	DOREPLIFETIME(ABreachbornePlayerState, Rep_HealthData);
	DOREPLIFETIME(ABreachbornePlayerState, Rep_CombatData);
	DOREPLIFETIME(ABreachbornePlayerState, Rep_MovementData);

	// Inventory data — single struct replicated on change
	DOREPLIFETIME(ABreachbornePlayerState, Rep_InventoryData);
}

void ABreachbornePlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Register ALL subobjects that need NetGUIDs for pointer resolution.
	// This must happen here (not in the constructor) because AddReplicatedSubObject
	// fails on CDOs with "Archetypes or CDO's cannot be replicated subobject's."
	//
	// The ASC needs registration for GAS Mixed-mode replication (ActiveGameplayEffects, etc.).
	// The AttributeSets need registration so that the ASC's replicated SpawnedAttributes
	// (TArray<UAttributeSet*>) can resolve the UObject pointers on clients.
	// Without NetGUIDs, SpawnedAttributes resolves to [nullptr, nullptr, nullptr] on clients,
	// causing ShowDebug AbilitySystem and ASC attribute lookups to fail.
	//
	// IMPORTANT: Registering the AttributeSets does NOT replicate their property data —
	// none of their attributes have CPF_Net/Replicated. Only NetGUID resolution is provided.
	// Actual attribute values flow through proxy structs (Rep_HealthData, etc.).
	if (AbilitySystemComponent)
	{
		AddReplicatedSubObject(AbilitySystemComponent);
	}
	if (HealthSet)
	{
		AddReplicatedSubObject(HealthSet);
	}
	if (CombatSet)
	{
		AddReplicatedSubObject(CombatSet);
	}
	if (MovementSet)
	{
		AddReplicatedSubObject(MovementSet);
	}
}

void ABreachbornePlayerState::BeginPlay()
{
	Super::BeginPlay();

	// Initialize ASC actor info early — BEFORE any attribute replication arrives on clients.
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	// Server: bind to attribute change delegates so proxy structs stay in sync
	if (HasAuthority())
	{
		BindAttributeChangeCallbacks();

		// Diagnostic: verify that AttributeSet properties do NOT have CPF_Net.
		if (HealthSet)
		{
			FProperty* HealthProp = FindFProperty<FProperty>(UBBHealthSet::StaticClass(), GET_MEMBER_NAME_CHECKED(UBBHealthSet, Health));
			if (HealthProp && HealthProp->HasAnyPropertyFlags(CPF_Net))
			{
				UE_LOG(LogBreachborne, Error, TEXT("REPLICATION WARNING: HealthSet.Health still has CPF_Net! UHT did not regenerate. Subobject list blocks this, but a clean rebuild is recommended."));
			}
			else
			{
				UE_LOG(LogBreachborne, Log, TEXT("REPLICATION OK: HealthSet.Health does NOT have CPF_Net. Subobject replication properly disabled."));
			}
		}
	}
}

UAbilitySystemComponent* ABreachbornePlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABreachbornePlayerState::SetTeamID(int32 NewTeamID)
{
	if (HasAuthority())
	{
		TeamID = NewTeamID;
	}
}

void ABreachbornePlayerState::SetHunterID(int32 NewHunterID)
{
	if (HasAuthority())
	{
		HunterID = NewHunterID;
	}
}

void ABreachbornePlayerState::SetReadyForMatch(bool bReady)
{
	if (HasAuthority())
	{
		bReadyForMatch = bReady;
	}
}

void ABreachbornePlayerState::SetHunterLocked(bool bLocked)
{
	if (HasAuthority())
	{
		bHunterLocked = bLocked;
	}
}

void ABreachbornePlayerState::SetLobbySlotIndex(int32 NewSlotIndex)
{
	if (HasAuthority())
	{
		LobbySlotIndex = NewSlotIndex;
	}
}

void ABreachbornePlayerState::SetIsSpectator(bool bSpectator)
{
	if (HasAuthority())
	{
		bIsLobbySpectator = bSpectator;
	}
}

void ABreachbornePlayerState::SetLevel(int32 NewLevel)
{
	if (HasAuthority())
	{
		Level = NewLevel;
	}
}

void ABreachbornePlayerState::AddXP(int32 Amount)
{
	if (HasAuthority())
	{
		XP += Amount;
	}
}

void ABreachbornePlayerState::AddKill()
{
	if (HasAuthority())
	{
		Kills++;
	}
}

void ABreachbornePlayerState::SetIsAlive(bool bAlive)
{
	if (HasAuthority())
	{
		bIsAlive = bAlive;
	}
}

void ABreachbornePlayerState::RestoreMatchProgress(
	int32 NewLevel, int32 NewXP, int32 NewKills, const FRepInventoryData& NewInventory)
{
	if (!HasAuthority())
	{
		return;
	}

	Level = FMath::Max(1, NewLevel);
	XP = FMath::Max(0, NewXP);
	Kills = FMath::Max(0, NewKills);
	Rep_InventoryData = NewInventory;
}

void ABreachbornePlayerState::OnRep_TeamID()
{
	UE_LOG(LogBreachborne, Verbose, TEXT("PlayerState %s: TeamID replicated to %d"), *GetPlayerName(), TeamID);
}

void ABreachbornePlayerState::OnRep_IsAlive()
{
	UE_LOG(LogBreachborne, Verbose, TEXT("PlayerState %s: bIsAlive replicated to %s"), *GetPlayerName(), bIsAlive ? TEXT("true") : TEXT("false"));
}

void ABreachbornePlayerState::OnRep_ReadyForMatch()
{
	UE_LOG(LogBreachborne, Verbose, TEXT("PlayerState %s: ready replicated to %s"), *GetPlayerName(), bReadyForMatch ? TEXT("true") : TEXT("false"));
}

void ABreachbornePlayerState::OnRep_HunterLocked()
{
	UE_LOG(LogBreachborne, Verbose, TEXT("PlayerState %s: hunter locked replicated to %s"), *GetPlayerName(), bHunterLocked ? TEXT("true") : TEXT("false"));
}

// ============================================================================
// Inventory
// ============================================================================

FRepInventoryData& ABreachbornePlayerState::GetInventoryDataMutable()
{
	return Rep_InventoryData;
}

void ABreachbornePlayerState::OnRep_InventoryData()
{
	UE_LOG(LogBreachborne, Verbose, TEXT("PlayerState %s: Inventory replicated (Gold=%d, Shards=%d, Weapon=%s, Helmet=%s, Boots=%s)"),
		*GetPlayerName(),
		Rep_InventoryData.Gold,
		Rep_InventoryData.UpgradeShards,
		*Rep_InventoryData.Weapon.ItemID.ToString(),
		*Rep_InventoryData.Helmet.ItemID.ToString(),
		*Rep_InventoryData.Boots.ItemID.ToString());
}

// ============================================================================
// Proxy Attribute Replication
// ============================================================================

void ABreachbornePlayerState::BindAttributeChangeCallbacks()
{
	if (!AbilitySystemComponent) return;

	// Health attributes
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBHealthSet::GetHealthAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBHealthSet::GetMaxHealthAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBHealthSet::GetShieldAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBHealthSet::GetMaxShieldAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBHealthSet::GetArmorAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);

	// Combat attributes
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBCombatSet::GetAttackPowerAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBCombatSet::GetAbilityPowerAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBCombatSet::GetCooldownReductionAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBCombatSet::GetCritChanceAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBCombatSet::GetCritMultiplierAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);

	// Movement attributes
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBMovementSet::GetMoveSpeedAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBMovementSet::GetGlideSpeedAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBBMovementSet::GetDashDistanceAttribute())
		.AddUObject(this, &ABreachbornePlayerState::OnAnyAttributeChanged);
}

void ABreachbornePlayerState::UpdateHealthProxy()
{
	if (!HasAuthority() || !HealthSet) return;

	Rep_HealthData.Health = HealthSet->GetHealth();
	Rep_HealthData.MaxHealth = HealthSet->GetMaxHealth();
	Rep_HealthData.Shield = HealthSet->GetShield();
	Rep_HealthData.MaxShield = HealthSet->GetMaxShield();
	Rep_HealthData.Armor = HealthSet->GetArmor();
}

void ABreachbornePlayerState::OnAnyAttributeChanged(const FOnAttributeChangeData& Data)
{
	// Server only: copy the new attribute value into the matching proxy struct field.
	// The proxy struct is replicated to clients via normal actor property replication.
	// NOTE: For health attributes, UpdateHealthProxy() is the primary path (called from PostGameplayEffectExecute).
	// This delegate path is a backup for health AND the primary path for combat/movement attributes.

	// Health
	if (Data.Attribute == UBBHealthSet::GetHealthAttribute())
		Rep_HealthData.Health = Data.NewValue;
	else if (Data.Attribute == UBBHealthSet::GetMaxHealthAttribute())
		Rep_HealthData.MaxHealth = Data.NewValue;
	else if (Data.Attribute == UBBHealthSet::GetShieldAttribute())
		Rep_HealthData.Shield = Data.NewValue;
	else if (Data.Attribute == UBBHealthSet::GetMaxShieldAttribute())
		Rep_HealthData.MaxShield = Data.NewValue;
	else if (Data.Attribute == UBBHealthSet::GetArmorAttribute())
		Rep_HealthData.Armor = Data.NewValue;

	// Combat
	else if (Data.Attribute == UBBCombatSet::GetAttackPowerAttribute())
		Rep_CombatData.AttackPower = Data.NewValue;
	else if (Data.Attribute == UBBCombatSet::GetAbilityPowerAttribute())
		Rep_CombatData.AbilityPower = Data.NewValue;
	else if (Data.Attribute == UBBCombatSet::GetCooldownReductionAttribute())
		Rep_CombatData.CooldownReduction = Data.NewValue;
	else if (Data.Attribute == UBBCombatSet::GetCritChanceAttribute())
		Rep_CombatData.CritChance = Data.NewValue;
	else if (Data.Attribute == UBBCombatSet::GetCritMultiplierAttribute())
		Rep_CombatData.CritMultiplier = Data.NewValue;

	// Movement
	else if (Data.Attribute == UBBMovementSet::GetMoveSpeedAttribute())
		Rep_MovementData.MoveSpeed = Data.NewValue;
	else if (Data.Attribute == UBBMovementSet::GetGlideSpeedAttribute())
		Rep_MovementData.GlideSpeed = Data.NewValue;
	else if (Data.Attribute == UBBMovementSet::GetDashDistanceAttribute())
		Rep_MovementData.DashDistance = Data.NewValue;
}

void ABreachbornePlayerState::OnRep_HealthData()
{
	if (!HealthSet || !AbilitySystemComponent) return;

	// Push each replicated value to the AttributeSet member and ASC aggregator.
	// This replaces what GAMEPLAYATTRIBUTE_REPNOTIFY would have done on the subobject.
	auto Push = [this](FGameplayAttributeData& AttrData, float NewValue, const FGameplayAttribute& Attr)
	{
		const float OldValue = AttrData.GetCurrentValue();
		if (OldValue != NewValue)
		{
			AttrData.SetBaseValue(NewValue);
			AttrData.SetCurrentValue(NewValue);
			AbilitySystemComponent->SetBaseAttributeValueFromReplication(Attr, NewValue, OldValue);
		}
	};

	Push(HealthSet->Health, Rep_HealthData.Health, UBBHealthSet::GetHealthAttribute());
	Push(HealthSet->MaxHealth, Rep_HealthData.MaxHealth, UBBHealthSet::GetMaxHealthAttribute());
	Push(HealthSet->Shield, Rep_HealthData.Shield, UBBHealthSet::GetShieldAttribute());
	Push(HealthSet->MaxShield, Rep_HealthData.MaxShield, UBBHealthSet::GetMaxShieldAttribute());
	Push(HealthSet->Armor, Rep_HealthData.Armor, UBBHealthSet::GetArmorAttribute());
}

void ABreachbornePlayerState::OnRep_CombatData()
{
	if (!CombatSet || !AbilitySystemComponent) return;

	auto Push = [this](FGameplayAttributeData& AttrData, float NewValue, const FGameplayAttribute& Attr)
	{
		const float OldValue = AttrData.GetCurrentValue();
		if (OldValue != NewValue)
		{
			AttrData.SetBaseValue(NewValue);
			AttrData.SetCurrentValue(NewValue);
			AbilitySystemComponent->SetBaseAttributeValueFromReplication(Attr, NewValue, OldValue);
		}
	};

	Push(CombatSet->AttackPower, Rep_CombatData.AttackPower, UBBCombatSet::GetAttackPowerAttribute());
	Push(CombatSet->AbilityPower, Rep_CombatData.AbilityPower, UBBCombatSet::GetAbilityPowerAttribute());
	Push(CombatSet->CooldownReduction, Rep_CombatData.CooldownReduction, UBBCombatSet::GetCooldownReductionAttribute());
	Push(CombatSet->CritChance, Rep_CombatData.CritChance, UBBCombatSet::GetCritChanceAttribute());
	Push(CombatSet->CritMultiplier, Rep_CombatData.CritMultiplier, UBBCombatSet::GetCritMultiplierAttribute());
}

void ABreachbornePlayerState::OnRep_MovementData()
{
	if (!MovementSet || !AbilitySystemComponent) return;

	auto Push = [this](FGameplayAttributeData& AttrData, float NewValue, const FGameplayAttribute& Attr)
	{
		const float OldValue = AttrData.GetCurrentValue();
		if (OldValue != NewValue)
		{
			AttrData.SetBaseValue(NewValue);
			AttrData.SetCurrentValue(NewValue);
			AbilitySystemComponent->SetBaseAttributeValueFromReplication(Attr, NewValue, OldValue);
		}
	};

	Push(MovementSet->MoveSpeed, Rep_MovementData.MoveSpeed, UBBMovementSet::GetMoveSpeedAttribute());
	Push(MovementSet->GlideSpeed, Rep_MovementData.GlideSpeed, UBBMovementSet::GetGlideSpeedAttribute());
	Push(MovementSet->DashDistance, Rep_MovementData.DashDistance, UBBMovementSet::GetDashDistanceAttribute());
}
