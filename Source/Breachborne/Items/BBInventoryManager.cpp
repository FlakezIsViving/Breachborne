#include "BBInventoryManager.h"
#include "BBEquipmentDefinition.h"
#include "BBPowerDefinition.h"
#include "BBConsumableDefinition.h"
#include "BBEquipmentGEHelper.h"
#include "BBInventoryTypes.h"
#include "GameplayEffect.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"
#include "BBItemRegistry.h"

// ============================================================================
// Equipment
// ============================================================================

bool FBBInventoryManager::EquipItem(ABreachbornePlayerState* PS, const UBBEquipmentDefinition* ItemDef)
{
	if (!PS || !ItemDef || ItemDef->SlotType == EEquipmentSlotType::None)
	{
		return false;
	}

	// Unequip existing item in this slot first
	UnequipSlot(PS, ItemDef->SlotType);

	// Set slot state
	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	FEquipmentSlotState& Slot = Inv.GetSlotMutable(ItemDef->SlotType);
	Slot.ItemID = ItemDef->ItemID;
	Slot.UpgradeTier = 0;
	Slot.Evolution = EEvolutionPath::None;

	// Apply stat bonuses
	ApplyEquipmentGE(PS, ItemDef->SlotType, ItemDef);

	UE_LOG(LogBreachborne, Log, TEXT("InventoryManager: %s equipped %s in %s slot"),
		*PS->GetPlayerName(), *ItemDef->ItemID.ToString(),
		*UEnum::GetValueAsString(ItemDef->SlotType));

	return true;
}

bool FBBInventoryManager::UnequipSlot(ABreachbornePlayerState* PS, EEquipmentSlotType SlotType)
{
	if (!PS || SlotType == EEquipmentSlotType::None)
	{
		return false;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	FEquipmentSlotState& Slot = Inv.GetSlotMutable(SlotType);

	if (Slot.IsEmpty())
	{
		return false;
	}

	// Remove GE bonuses
	UBBAbilitySystemComponent* ASC = PS->GetBBAbilitySystemComponent();
	FActiveGameplayEffectHandle& Handle = GetSlotGEHandle(PS, SlotType);
	FBBEquipmentGEHelper::RemoveStatBonuses(ASC, Handle);

	UE_LOG(LogBreachborne, Log, TEXT("InventoryManager: %s unequipped %s from %s slot"),
		*PS->GetPlayerName(), *Slot.ItemID.ToString(),
		*UEnum::GetValueAsString(SlotType));

	Slot.Clear();
	return true;
}

bool FBBInventoryManager::UpgradeEquipment(ABreachbornePlayerState* PS, EEquipmentSlotType SlotType)
{
	if (!PS || SlotType == EEquipmentSlotType::None)
	{
		return false;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	FEquipmentSlotState& Slot = Inv.GetSlotMutable(SlotType);

	if (Slot.IsEmpty())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("InventoryManager: Cannot upgrade — slot %s is empty"),
			*UEnum::GetValueAsString(SlotType));
		return false;
	}

	// Look up the definition to get shard cost (for now, use a flat cost of 5 per tier)
	const int32 ShardCost = 5; // TODO: look up from ItemDef via Registry in 5H
	if (Inv.UpgradeShards < ShardCost)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("InventoryManager: Not enough shards (%d/%d) to upgrade %s"),
			Inv.UpgradeShards, ShardCost, *Slot.ItemID.ToString());
		return false;
	}

	// Deduct shards and increment tier
	Inv.UpgradeShards -= ShardCost;
	Slot.UpgradeTier++;

	// Reapply GE with new tier stats
	// Remove old GE, apply new one with updated stats
	UBBAbilitySystemComponent* ASC = PS->GetBBAbilitySystemComponent();
	FActiveGameplayEffectHandle& Handle = GetSlotGEHandle(PS, SlotType);
	FBBEquipmentGEHelper::RemoveStatBonuses(ASC, Handle);

	// For now, apply scaled base stats (proper definition lookup in 5H)
	FBBStatBonuses ScaledStats;
	const float TierMultiplier = 1.0f + (Slot.UpgradeTier * 0.5f);
	// Placeholder: 10 attack power * tier multiplier for weapons
	if (SlotType == EEquipmentSlotType::Weapon)
	{
		ScaledStats.AttackPower = 10.0f * TierMultiplier;
	}
	else if (SlotType == EEquipmentSlotType::Helmet)
	{
		ScaledStats.MaxHealth = 50.0f * TierMultiplier;
	}
	else if (SlotType == EEquipmentSlotType::Boots)
	{
		ScaledStats.MoveSpeed = 25.0f * TierMultiplier;
	}

	const FGameplayTag& EffectTag = (SlotType == EEquipmentSlotType::Weapon) ? BBGameplayTags::Effect_Equipment_Weapon
		: (SlotType == EEquipmentSlotType::Helmet) ? BBGameplayTags::Effect_Equipment_Helmet
		: BBGameplayTags::Effect_Equipment_Boots;

	Handle = FBBEquipmentGEHelper::ApplyStatBonuses(ASC, ScaledStats, EffectTag,
		FString::Printf(TEXT("%s_Tier%d"), *Slot.ItemID.ToString(), Slot.UpgradeTier));

	UE_LOG(LogBreachborne, Log, TEXT("InventoryManager: %s upgraded %s to tier %d (cost %d shards, %d remaining)"),
		*PS->GetPlayerName(), *Slot.ItemID.ToString(), Slot.UpgradeTier, ShardCost, Inv.UpgradeShards);

	return true;
}

bool FBBInventoryManager::EvolveEquipment(ABreachbornePlayerState* PS, EEquipmentSlotType SlotType, EEvolutionPath Path)
{
	if (!PS || SlotType == EEquipmentSlotType::None || Path == EEvolutionPath::None)
	{
		return false;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	FEquipmentSlotState& Slot = Inv.GetSlotMutable(SlotType);

	if (Slot.IsEmpty())
	{
		UE_LOG(LogBreachborne, Warning, TEXT("InventoryManager: Cannot evolve — slot %s is empty"),
			*UEnum::GetValueAsString(SlotType));
		return false;
	}

	if (Slot.Evolution != EEvolutionPath::None)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("InventoryManager: %s already evolved to %s"),
			*Slot.ItemID.ToString(), *UEnum::GetValueAsString(Slot.Evolution));
		return false;
	}

	// Require minimum tier for evolution (default: tier 2)
	const int32 RequiredTier = 2; // TODO: look up from ItemDef via Registry in 5H
	if (Slot.UpgradeTier < RequiredTier)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("InventoryManager: %s at tier %d, need tier %d to evolve"),
			*Slot.ItemID.ToString(), Slot.UpgradeTier, RequiredTier);
		return false;
	}

	Slot.Evolution = Path;

	// Reapply GE with evolution bonuses
	UBBAbilitySystemComponent* ASC = PS->GetBBAbilitySystemComponent();
	FActiveGameplayEffectHandle& Handle = GetSlotGEHandle(PS, SlotType);
	FBBEquipmentGEHelper::RemoveStatBonuses(ASC, Handle);

	// Placeholder stats with evolution (proper definition lookup in 5H)
	FBBStatBonuses EvolvedStats;
	const float TierMultiplier = 1.0f + (Slot.UpgradeTier * 0.5f);
	if (SlotType == EEquipmentSlotType::Weapon)
	{
		EvolvedStats.AttackPower = 10.0f * TierMultiplier + 15.0f; // evolution bonus
	}
	else if (SlotType == EEquipmentSlotType::Helmet)
	{
		EvolvedStats.MaxHealth = 50.0f * TierMultiplier + 30.0f;
	}
	else if (SlotType == EEquipmentSlotType::Boots)
	{
		EvolvedStats.MoveSpeed = 25.0f * TierMultiplier + 20.0f;
	}

	const FGameplayTag& EffectTag = (SlotType == EEquipmentSlotType::Weapon) ? BBGameplayTags::Effect_Equipment_Weapon
		: (SlotType == EEquipmentSlotType::Helmet) ? BBGameplayTags::Effect_Equipment_Helmet
		: BBGameplayTags::Effect_Equipment_Boots;

	Handle = FBBEquipmentGEHelper::ApplyStatBonuses(ASC, EvolvedStats, EffectTag,
		FString::Printf(TEXT("%s_Tier%d_%s"), *Slot.ItemID.ToString(), Slot.UpgradeTier,
			*UEnum::GetValueAsString(Path)));

	UE_LOG(LogBreachborne, Log, TEXT("InventoryManager: %s evolved %s to %s"),
		*PS->GetPlayerName(), *Slot.ItemID.ToString(), *UEnum::GetValueAsString(Path));

	return true;
}

void FBBInventoryManager::ApplyEquipmentGE(ABreachbornePlayerState* PS, EEquipmentSlotType SlotType, const UBBEquipmentDefinition* ItemDef)
{
	if (!PS || !ItemDef)
	{
		return;
	}

	UBBAbilitySystemComponent* ASC = PS->GetBBAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	const FEquipmentSlotState& Slot = Inv.GetSlot(SlotType);
	const FBBStatBonuses Stats = ItemDef->GetTotalStats(Slot.UpgradeTier, Slot.Evolution);

	const FGameplayTag& EffectTag = (SlotType == EEquipmentSlotType::Weapon) ? BBGameplayTags::Effect_Equipment_Weapon
		: (SlotType == EEquipmentSlotType::Helmet) ? BBGameplayTags::Effect_Equipment_Helmet
		: BBGameplayTags::Effect_Equipment_Boots;

	FActiveGameplayEffectHandle& Handle = GetSlotGEHandle(PS, SlotType);
	Handle = FBBEquipmentGEHelper::ApplyStatBonuses(ASC, Stats, EffectTag,
		FString::Printf(TEXT("%s_Tier%d"), *ItemDef->ItemID.ToString(), Slot.UpgradeTier));
}

// ============================================================================
// Powers
// ============================================================================

bool FBBInventoryManager::EquipPower(ABreachbornePlayerState* PS, const UBBPowerDefinition* PowerDef, EPowerSlotIndex Slot)
{
	if (!PS || !PowerDef)
	{
		return false;
	}
	UBBPowerDefinition* MutablePowerDef = const_cast<UBBPowerDefinition*>(PowerDef);
	MutablePowerDef->NormalizeLegacyFields();

	// Unequip existing power in this slot first
	UnequipPower(PS, Slot);

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	FPowerSlotState& PowerSlot = Inv.GetPowerSlotMutable(Slot);
	PowerSlot.PowerID = PowerDef->PowerID;

	UBBAbilitySystemComponent* ASC = PS->GetBBAbilitySystemComponent();

	// Apply stat bonuses if any
	FActiveGameplayEffectHandle& GEHandle = (Slot == EPowerSlotIndex::Power1) ? PS->ActivePower1GEHandle : PS->ActivePower2GEHandle;
	if (!PowerDef->StatBonuses.IsZero())
	{
		GEHandle = FBBEquipmentGEHelper::ApplyStatBonuses(ASC, PowerDef->StatBonuses, BBGameplayTags::Effect_Power,
			FString::Printf(TEXT("Power_%s"), *PowerDef->PowerID.ToString()));
	}

	// Grant ability if specified
	if (PowerDef->GrantedAbilityClass)
	{
		FGameplayAbilitySpecHandle& AbilityHandle = (Slot == EPowerSlotIndex::Power1)
			? PS->GrantedPower1AbilityHandle : PS->GrantedPower2AbilityHandle;
		const FGameplayTag InputTag = PowerDef->PowerInputTagOverride.IsValid()
			? PowerDef->PowerInputTagOverride
			: (Slot == EPowerSlotIndex::Power1 ? BBGameplayTags::InputTag_Power1 : BBGameplayTags::InputTag_Power2);
		AbilityHandle = FBBEquipmentGEHelper::GrantPowerAbility(ASC, PowerDef->GrantedAbilityClass, MutablePowerDef, InputTag);

		if (PowerDef->bAutoActivateOnEquip || PowerDef->ActivationType == EBBPowerActivationType::Passive)
		{
			ASC->TryActivateAbility(AbilityHandle);
		}
	}

	UE_LOG(LogBreachborne, Log, TEXT("PowerInventory: %s equipped power %s in slot %s"),
		*PS->GetPlayerName(), *PowerDef->PowerID.ToString(), *UEnum::GetValueAsString(Slot));

	return true;
}

bool FBBInventoryManager::EquipPowerByID(ABreachbornePlayerState* PS, FName PowerID, EPowerSlotIndex PreferredSlot, bool bUsePreferredSlot)
{
	if (!PS || PowerID.IsNone())
	{
		return false;
	}

	const UWorld* World = PS->GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UBBItemRegistry* Registry = GameInstance ? GameInstance->GetSubsystem<UBBItemRegistry>() : nullptr;
	const UBBPowerDefinition* PowerDef = Registry ? Registry->FindPower(PowerID) : nullptr;
	if (!PowerDef)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("PowerInventory: %s cannot equip missing power '%s'"),
			*PS->GetPlayerName(), *PowerID.ToString());
		return false;
	}

	EPowerSlotIndex TargetSlot = PreferredSlot;
	if (!bUsePreferredSlot)
	{
		const FRepInventoryData& Inv = PS->GetInventoryData();
		if (Inv.Power1.IsEmpty())
		{
			TargetSlot = EPowerSlotIndex::Power1;
		}
		else if (Inv.Power2.IsEmpty())
		{
			TargetSlot = EPowerSlotIndex::Power2;
		}
		else
		{
			TargetSlot = EPowerSlotIndex::Power1;
			UE_LOG(LogBreachborne, Log, TEXT("PowerInventory: %s replacing Power1 with %s because both slots are full"),
				*PS->GetPlayerName(), *PowerID.ToString());
		}
	}

	return EquipPower(PS, PowerDef, TargetSlot);
}

bool FBBInventoryManager::UnequipPower(ABreachbornePlayerState* PS, EPowerSlotIndex Slot)
{
	if (!PS)
	{
		return false;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	FPowerSlotState& PowerSlot = Inv.GetPowerSlotMutable(Slot);

	if (PowerSlot.IsEmpty())
	{
		return false;
	}

	UBBAbilitySystemComponent* ASC = PS->GetBBAbilitySystemComponent();

	// Remove stat bonuses
	FActiveGameplayEffectHandle& GEHandle = (Slot == EPowerSlotIndex::Power1) ? PS->ActivePower1GEHandle : PS->ActivePower2GEHandle;
	FBBEquipmentGEHelper::RemoveStatBonuses(ASC, GEHandle);

	// Remove granted ability
	FGameplayAbilitySpecHandle& AbilityHandle = (Slot == EPowerSlotIndex::Power1)
		? PS->GrantedPower1AbilityHandle : PS->GrantedPower2AbilityHandle;
	FBBEquipmentGEHelper::RemovePowerAbility(ASC, AbilityHandle);

	UE_LOG(LogBreachborne, Log, TEXT("PowerInventory: %s unequipped power from slot %s"),
		*PS->GetPlayerName(), *UEnum::GetValueAsString(Slot));

	PowerSlot.Clear();
	return true;
}

// ============================================================================
// Armor
// ============================================================================

bool FBBInventoryManager::SetArmorTier(ABreachbornePlayerState* PS, EArmorTier NewTier)
{
	if (!PS)
	{
		return false;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	if (Inv.Armor.Tier == NewTier)
	{
		return false;
	}

	// Remove old armor GE
	UBBAbilitySystemComponent* ASC = PS->GetBBAbilitySystemComponent();
	FBBEquipmentGEHelper::RemoveStatBonuses(ASC, PS->ActiveArmorGEHandle);

	Inv.Armor.Tier = NewTier;
	Inv.Armor.CurrentHP = GetArmorMaxHP(NewTier);

	RefreshArmorAttribute(PS);

	UE_LOG(LogBreachborne, Log, TEXT("BasecampArmor: %s armor set to %s (HP=%.0f, ArmorValue=%.0f)"),
		*PS->GetPlayerName(), *UEnum::GetValueAsString(NewTier),
		Inv.Armor.CurrentHP, GetArmorValueForTier(NewTier));

	return true;
}

float FBBInventoryManager::GetArmorValueForTier(EArmorTier Tier)
{
	switch (Tier)
	{
	case EArmorTier::White:  return 5.0f;
	case EArmorTier::Green:  return 10.0f;
	case EArmorTier::Blue:   return 20.0f;
	case EArmorTier::Purple: return 35.0f;
	default: return 0.0f;
	}
}

float FBBInventoryManager::GetArmorMaxHP(EArmorTier Tier)
{
	switch (Tier)
	{
	case EArmorTier::White:  return 50.0f;
	case EArmorTier::Green:  return 100.0f;
	case EArmorTier::Blue:   return 150.0f;
	case EArmorTier::Purple: return 200.0f;
	default: return 0.0f;
	}
}

// ============================================================================
// Economy
// ============================================================================

void FBBInventoryManager::AddGold(ABreachbornePlayerState* PS, int32 Amount)
{
	if (!PS)
	{
		return;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	Inv.Gold = FMath::Max(0, Inv.Gold + Amount);

	UE_LOG(LogBreachborne, Log, TEXT("BasecampInventory: %s gold %s%d (total: %d)"),
		*PS->GetPlayerName(), Amount >= 0 ? TEXT("+") : TEXT(""), Amount, Inv.Gold);
}

bool FBBInventoryManager::TrySpendGold(ABreachbornePlayerState* PS, int32 Amount)
{
	if (!PS || Amount < 0)
	{
		return false;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	if (Inv.Gold < Amount)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BasecampInventory: %s cannot spend %dg (has %d)"),
			*PS->GetPlayerName(), Amount, Inv.Gold);
		return false;
	}

	Inv.Gold -= Amount;
	PS->ForceNetUpdate();

	UE_LOG(LogBreachborne, Log, TEXT("BasecampInventory: %s spent %dg (remaining: %d)"),
		*PS->GetPlayerName(), Amount, Inv.Gold);
	return true;
}

void FBBInventoryManager::AddShards(ABreachbornePlayerState* PS, int32 Amount)
{
	if (!PS)
	{
		return;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	Inv.UpgradeShards = FMath::Max(0, Inv.UpgradeShards + Amount);

	UE_LOG(LogBreachborne, Log, TEXT("InventoryManager: %s shards %s%d (total: %d)"),
		*PS->GetPlayerName(), Amount >= 0 ? TEXT("+") : TEXT(""), Amount, Inv.UpgradeShards);
}

// ============================================================================
// Consumables
// ============================================================================

bool FBBInventoryManager::AddConsumable(ABreachbornePlayerState* PS, const UBBConsumableDefinition* ConsumableDef, int32 Count)
{
	if (!PS || !ConsumableDef || Count <= 0)
	{
		return false;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	int32 SlotIndex = Inv.FindConsumableSlot(ConsumableDef->ConsumableID);

	if (SlotIndex == INDEX_NONE)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("InventoryManager: %s backpack full, cannot add %s"),
			*PS->GetPlayerName(), *ConsumableDef->ConsumableID.ToString());
		return false;
	}

	FConsumableStack& Stack = Inv.Backpack[SlotIndex];
	if (Stack.IsEmpty())
	{
		// New stack
		Stack.ConsumableID = ConsumableDef->ConsumableID;
		Stack.Count = FMath::Min(Count, ConsumableDef->MaxStackSize);
	}
	else
	{
		// Add to existing stack, capped at max
		Stack.Count = FMath::Min(Stack.Count + Count, ConsumableDef->MaxStackSize);
	}

	UE_LOG(LogBreachborne, Log, TEXT("InventoryManager: %s added %d %s (slot %d, total %d)"),
		*PS->GetPlayerName(), Count, *ConsumableDef->ConsumableID.ToString(),
		SlotIndex, Stack.Count);

	return true;
}

bool FBBInventoryManager::AddConsumableStack(ABreachbornePlayerState* PS, FName ConsumableID, int32 Count, int32 MaxStackSize)
{
	if (!PS || ConsumableID.IsNone() || Count <= 0 || MaxStackSize <= 0)
	{
		return false;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	int32 Remaining = Count;

	for (FConsumableStack& Stack : Inv.Backpack)
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (Stack.ConsumableID == ConsumableID && Stack.Count > 0 && Stack.Count < MaxStackSize)
		{
			const int32 Added = FMath::Min(Remaining, MaxStackSize - Stack.Count);
			Stack.Count += Added;
			Remaining -= Added;
		}
	}

	while (Remaining > 0)
	{
		const int32 EmptyIndex = Inv.FindEmptyBackpackSlot();
		if (EmptyIndex == INDEX_NONE)
		{
			UE_LOG(LogBreachborne, Warning, TEXT("InventoryManager: %s backpack full, cannot add %d %s"),
				*PS->GetPlayerName(), Remaining, *ConsumableID.ToString());
			PS->ForceNetUpdate();
			return Remaining < Count;
		}

		FConsumableStack& Stack = Inv.Backpack[EmptyIndex];
		Stack.ConsumableID = ConsumableID;
		Stack.Count = FMath::Min(Remaining, MaxStackSize);
		Remaining -= Stack.Count;
	}

	PS->ForceNetUpdate();

	UE_LOG(LogBreachborne, Log, TEXT("InventoryManager: %s added %d %s"),
		*PS->GetPlayerName(), Count, *ConsumableID.ToString());
	return true;
}

bool FBBInventoryManager::HasConsumable(const ABreachbornePlayerState* PS, FName ConsumableID, int32 Count)
{
	if (!PS || ConsumableID.IsNone() || Count <= 0)
	{
		return false;
	}

	int32 Found = 0;
	const FRepInventoryData& Inv = PS->GetInventoryData();
	for (const FConsumableStack& Stack : Inv.Backpack)
	{
		if (Stack.ConsumableID == ConsumableID && Stack.Count > 0)
		{
			Found += Stack.Count;
			if (Found >= Count)
			{
				return true;
			}
		}
	}

	return false;
}

bool FBBInventoryManager::ConsumeConsumable(ABreachbornePlayerState* PS, FName ConsumableID, int32 Count)
{
	if (!PS || ConsumableID.IsNone() || Count <= 0 || !HasConsumable(PS, ConsumableID, Count))
	{
		return false;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	int32 Remaining = Count;
	for (FConsumableStack& Stack : Inv.Backpack)
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (Stack.ConsumableID == ConsumableID && Stack.Count > 0)
		{
			const int32 Removed = FMath::Min(Remaining, Stack.Count);
			Stack.Count -= Removed;
			Remaining -= Removed;
			if (Stack.Count <= 0)
			{
				Stack.Clear();
			}
		}
	}

	PS->ForceNetUpdate();

	UE_LOG(LogBreachborne, Log, TEXT("InventoryManager: %s consumed %d %s"),
		*PS->GetPlayerName(), Count, *ConsumableID.ToString());
	return true;
}

bool FBBInventoryManager::UseConsumable(ABreachbornePlayerState* PS, int32 BackpackSlotIndex)
{
	if (!PS)
	{
		return false;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	if (BackpackSlotIndex < 0 || BackpackSlotIndex >= Inv.Backpack.Num())
	{
		return false;
	}

	FConsumableStack& Stack = Inv.Backpack[BackpackSlotIndex];
	if (Stack.IsEmpty())
	{
		return false;
	}

	const FName ConsumableID = Stack.ConsumableID;

	// Consume one
	Stack.Count--;

	UE_LOG(LogBreachborne, Log, TEXT("InventoryManager: %s used consumable %s from slot %d (%d remaining)"),
		*PS->GetPlayerName(), *ConsumableID.ToString(), BackpackSlotIndex, Stack.Count);

	// Clear the slot if empty
	if (Stack.Count <= 0)
	{
		Stack.Clear();
	}

	// Apply consumable effect based on ID prefix convention:
	// "Food_*" = instant heal, "ArmorShard_*" = armor repair, "ViveBean" = small instant heal, "ViveBrew" = HoT
	// Full definition lookup via ItemRegistry will replace this in 5H.
	UBBAbilitySystemComponent* ASC = PS->GetBBAbilitySystemComponent();
	const FString IDStr = ConsumableID.ToString();
	const auto ApplyHealingGE = [ASC](AActor* SourceObject, float HealAmount, float Duration, float Period, const TCHAR* EffectName) -> bool
	{
		if (!ASC || HealAmount <= 0.0f)
		{
			return false;
		}

		UGameplayEffect* HealGE = NewObject<UGameplayEffect>(ASC->GetOwner(), EffectName);
		HealGE->DurationPolicy = Duration > 0.0f
			? EGameplayEffectDurationType::HasDuration
			: EGameplayEffectDurationType::Instant;

		if (Duration > 0.0f)
		{
			HealGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));
			HealGE->Period = FMath::Max(Period, 0.1f);
			HealGE->bExecutePeriodicEffectOnApplication = true;
		}

		FGameplayModifierInfo& Mod = HealGE->Modifiers.AddDefaulted_GetRef();
		Mod.Attribute = UBBHealthSet::GetIncomingHealingAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(HealAmount));

		FInheritedTagContainer TagContainer;
		TagContainer.Added.AddTag(BBGameplayTags::Effect_Consumable_Heal);
		HealGE->InheritableOwnedTagsContainer = TagContainer;

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(SourceObject);
		FGameplayEffectSpec Spec(HealGE, Context, 1.0f);
		ASC->ApplyGameplayEffectSpecToSelf(Spec);
		return true;
	};

	if (IDStr.StartsWith(TEXT("Food")))
	{
		if (ApplyHealingGE(ASC ? ASC->GetOwner() : nullptr, 100.0f, 0.0f, 0.0f, TEXT("GE_ConsumableHeal")))
		{
			UE_LOG(LogBreachborne, Log, TEXT("InventoryManager: Applied 100 HP heal from %s"), *ConsumableID.ToString());
		}
	}
	else if (IDStr.StartsWith(TEXT("ArmorShard")))
	{
		RepairArmorDurability(PS, 0.25f);
	}
	else if (IDStr.Equals(TEXT("ViveBean"), ESearchCase::IgnoreCase))
	{
		if (ApplyHealingGE(ASC ? ASC->GetOwner() : nullptr, 50.0f, 0.0f, 0.0f, TEXT("GE_ViveBeanHeal")))
		{
			UE_LOG(LogBreachborne, Log, TEXT("BasecampInventory: Applied ViveBean instant heal from %s"), *ConsumableID.ToString());
		}
	}
	else if (IDStr.StartsWith(TEXT("ViveBrew")) || IDStr.Equals(TEXT("ViveBrew"), ESearchCase::IgnoreCase))
	{
		if (ApplyHealingGE(ASC ? ASC->GetOwner() : nullptr, 20.0f, 5.0f, 1.0f, TEXT("GE_ViveBrewHealOverTime")))
		{
			UE_LOG(LogBreachborne, Log, TEXT("BasecampInventory: Applied ViveBrew heal-over-time from %s"), *ConsumableID.ToString());
		}
	}

	return true;
}

bool FBBInventoryManager::RepairArmorDurability(ABreachbornePlayerState* PS, float MaxDurabilityFraction)
{
	if (!PS || MaxDurabilityFraction <= 0.0f)
	{
		return false;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	if (!Inv.Armor.HasArmor())
	{
		return false;
	}

	const float MaxHP = GetArmorMaxHP(Inv.Armor.Tier);
	if (MaxHP <= 0.0f || Inv.Armor.CurrentHP >= MaxHP)
	{
		return false;
	}

	const float RepairAmount = MaxHP * MaxDurabilityFraction;
	const float OldHP = Inv.Armor.CurrentHP;
	Inv.Armor.CurrentHP = FMath::Min(MaxHP, Inv.Armor.CurrentHP + RepairAmount);
	RefreshArmorAttribute(PS);
	PS->ForceNetUpdate();

	UE_LOG(LogBreachborne, Log, TEXT("BasecampArmor: %s repaired armor %.0f -> %.0f / %.0f"),
		*PS->GetPlayerName(), OldHP, Inv.Armor.CurrentHP, MaxHP);
	return true;
}

bool FBBInventoryManager::ApplyArmorDurabilityDamage(ABreachbornePlayerState* PS, float DamageAmount)
{
	if (!PS || DamageAmount <= 0.0f)
	{
		return false;
	}

	FRepInventoryData& Inv = PS->GetInventoryDataMutable();
	if (!Inv.Armor.HasArmor() || Inv.Armor.CurrentHP <= 0.0f)
	{
		return false;
	}

	const float OldHP = Inv.Armor.CurrentHP;
	Inv.Armor.CurrentHP = FMath::Max(0.0f, Inv.Armor.CurrentHP - DamageAmount);
	RefreshArmorAttribute(PS);
	PS->ForceNetUpdate();

	UE_LOG(LogBreachborne, Log, TEXT("BasecampArmor: %s armor durability %.0f -> %.0f after %.1f damage"),
		*PS->GetPlayerName(), OldHP, Inv.Armor.CurrentHP, DamageAmount);
	return true;
}

void FBBInventoryManager::RefreshArmorAttribute(ABreachbornePlayerState* PS)
{
	if (!PS)
	{
		return;
	}

	const FRepInventoryData& Inv = PS->GetInventoryData();
	const float ArmorValue = Inv.Armor.HasArmor() && Inv.Armor.CurrentHP > 0.0f
		? GetArmorValueForTier(Inv.Armor.Tier)
		: 0.0f;

	if (UBBHealthSet* HealthSet = PS->GetHealthSet())
	{
		HealthSet->SetArmor(ArmorValue);
		PS->UpdateHealthProxy();
	}
}

// ============================================================================
// Helpers
// ============================================================================

FActiveGameplayEffectHandle& FBBInventoryManager::GetSlotGEHandle(ABreachbornePlayerState* PS, EEquipmentSlotType SlotType)
{
	switch (SlotType)
	{
	case EEquipmentSlotType::Weapon: return PS->ActiveWeaponGEHandle;
	case EEquipmentSlotType::Helmet: return PS->ActiveHelmetGEHandle;
	case EEquipmentSlotType::Boots:  return PS->ActiveBootsGEHandle;
	default:
		// Should never happen — callers check for None
		static FActiveGameplayEffectHandle DummyHandle;
		DummyHandle.Invalidate();
		return DummyHandle;
	}
}
