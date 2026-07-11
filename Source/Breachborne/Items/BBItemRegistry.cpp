#include "BBItemRegistry.h"
#include "BBEquipmentDefinition.h"
#include "BBPowerDefinition.h"
#include "BBConsumableDefinition.h"
#include "Engine/AssetManager.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Abilities/Powers/GA_Power_BungeeShot.h"
#include "Breachborne/Abilities/Powers/GA_Power_EmergencyPlatform.h"
#include "Breachborne/Abilities/Powers/GA_Power_GrapplingHook.h"
#include "Breachborne/Abilities/Powers/GA_Power_RegenerativeArmor.h"
#include "Breachborne/Abilities/Powers/GA_Power_TacticalNuke.h"

void UBBItemRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ScanForDataAssets();
	RegisterBuiltInPowerDefinitions();

	UE_LOG(LogBreachborne, Log, TEXT("ItemRegistry: Initialized with %d equipment, %d powers, %d consumables"),
		EquipmentMap.Num(), PowerMap.Num(), ConsumableMap.Num());
}

void UBBItemRegistry::ScanForDataAssets()
{
	UAssetManager& AssetManager = UAssetManager::Get();

	// Scan for equipment definitions
	TArray<FPrimaryAssetId> EquipmentAssets;
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("BBEquipment")), EquipmentAssets);
	for (const FPrimaryAssetId& AssetId : EquipmentAssets)
	{
		FSoftObjectPath Path = AssetManager.GetPrimaryAssetPath(AssetId);
		if (UBBEquipmentDefinition* Def = Cast<UBBEquipmentDefinition>(Path.TryLoad()))
		{
			EquipmentMap.Add(Def->ItemID, Def);
		}
	}

	// Scan for power definitions
	TArray<FPrimaryAssetId> PowerAssets;
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("BBPower")), PowerAssets);
	for (const FPrimaryAssetId& AssetId : PowerAssets)
	{
		FSoftObjectPath Path = AssetManager.GetPrimaryAssetPath(AssetId);
		if (UBBPowerDefinition* Def = Cast<UBBPowerDefinition>(Path.TryLoad()))
		{
			PowerMap.Add(Def->PowerID, Def);
		}
	}

	// Scan for consumable definitions
	TArray<FPrimaryAssetId> ConsumableAssets;
	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("BBConsumable")), ConsumableAssets);
	for (const FPrimaryAssetId& AssetId : ConsumableAssets)
	{
		FSoftObjectPath Path = AssetManager.GetPrimaryAssetPath(AssetId);
		if (UBBConsumableDefinition* Def = Cast<UBBConsumableDefinition>(Path.TryLoad()))
		{
			ConsumableMap.Add(Def->ConsumableID, Def);
		}
	}
}

const UBBEquipmentDefinition* UBBItemRegistry::FindEquipment(FName ItemID) const
{
	const TObjectPtr<UBBEquipmentDefinition>* Found = EquipmentMap.Find(ItemID);
	return Found ? Found->Get() : nullptr;
}

const UBBPowerDefinition* UBBItemRegistry::FindPower(FName PowerID) const
{
	const TObjectPtr<UBBPowerDefinition>* Found = PowerMap.Find(PowerID);
	return Found ? Found->Get() : nullptr;
}

const UBBConsumableDefinition* UBBItemRegistry::FindConsumable(FName ConsumableID) const
{
	const TObjectPtr<UBBConsumableDefinition>* Found = ConsumableMap.Find(ConsumableID);
	return Found ? Found->Get() : nullptr;
}

const UBBEquipmentDefinition* UBBItemRegistry::GetRandomEquipmentDrop() const
{
	if (EquipmentMap.IsEmpty())
	{
		return nullptr;
	}

	TArray<FName> Keys;
	EquipmentMap.GetKeys(Keys);
	const int32 Index = FMath::RandRange(0, Keys.Num() - 1);
	return FindEquipment(Keys[Index]);
}

const UBBPowerDefinition* UBBItemRegistry::GetRandomPowerDrop() const
{
	if (PowerMap.IsEmpty())
	{
		return nullptr;
	}

	TArray<FName> Keys;
	PowerMap.GetKeys(Keys);
	const int32 Index = FMath::RandRange(0, Keys.Num() - 1);
	return FindPower(Keys[Index]);
}

void UBBItemRegistry::RegisterEquipment(UBBEquipmentDefinition* Def)
{
	if (Def)
	{
		EquipmentMap.Add(Def->ItemID, Def);
	}
}

void UBBItemRegistry::RegisterPower(UBBPowerDefinition* Def)
{
	if (Def)
	{
		PowerMap.Add(Def->PowerID, Def);
	}
}

void UBBItemRegistry::RegisterConsumable(UBBConsumableDefinition* Def)
{
	if (Def)
	{
		ConsumableMap.Add(Def->ConsumableID, Def);
	}
}

void UBBItemRegistry::RegisterBuiltInPowerDefinitions()
{
	auto AddPower = [this](FName PowerID, const TCHAR* DisplayName, EItemRarity Rarity, EBBPowerActivationType ActivationType,
		TSubclassOf<UGameplayAbility> AbilityClass, float CooldownSeconds)
	{
		if (PowerMap.Contains(PowerID))
		{
			return;
		}

		UBBPowerDefinition* Def = NewObject<UBBPowerDefinition>(this);
		Def->PowerID = PowerID;
		Def->DisplayName = FText::FromString(DisplayName);
		Def->Description = FText::FromString(DisplayName);
		Def->Rarity = Rarity;
		Def->ActivationType = ActivationType;
		Def->GrantedAbilityClass = AbilityClass;
		Def->CooldownSeconds = CooldownSeconds;
		Def->bAutoActivateOnEquip = ActivationType == EBBPowerActivationType::Passive || ActivationType == EBBPowerActivationType::Toggle;
		Def->bSoulbound = true;
		Def->bDropOnDeath = false;
		Def->NormalizeLegacyFields();
		RegisterPower(Def);
	};

	AddPower(FName(TEXT("BungeeShot")), TEXT("Bungee Shot"), EItemRarity::Rare, EBBPowerActivationType::Active,
		UGA_Power_BungeeShot::StaticClass(), 0.0f);
	AddPower(FName(TEXT("GrapplingHook")), TEXT("Grappling Hook"), EItemRarity::Rare, EBBPowerActivationType::Active,
		UGA_Power_GrapplingHook::StaticClass(), 14.0f);
	AddPower(FName(TEXT("EmergencyPlatform")), TEXT("Emergency Platform"), EItemRarity::Uncommon, EBBPowerActivationType::Active,
		UGA_Power_EmergencyPlatform::StaticClass(), 20.0f);
	AddPower(FName(TEXT("RegenerativeArmor")), TEXT("Regenerative Armor"), EItemRarity::Uncommon, EBBPowerActivationType::Passive,
		UGA_Power_RegenerativeArmor::StaticClass(), 0.0f);
	AddPower(FName(TEXT("TacticalNuke")), TEXT("Tactical Nuke"), EItemRarity::Rare, EBBPowerActivationType::Active,
		UGA_Power_TacticalNuke::StaticClass(), 45.0f);
}
