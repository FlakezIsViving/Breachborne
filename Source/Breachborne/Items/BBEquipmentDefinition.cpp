#include "BBEquipmentDefinition.h"

const FEquipmentEvolution& UBBEquipmentDefinition::GetEvolution(EEvolutionPath Path) const
{
	switch (Path)
	{
	case EEvolutionPath::PathA: return EvolutionA;
	case EEvolutionPath::PathB: return EvolutionB;
	case EEvolutionPath::PathC: return EvolutionC;
	default:
		// Return A as fallback — callers should check for EEvolutionPath::None before calling
		return EvolutionA;
	}
}

int32 UBBEquipmentDefinition::GetUpgradeCost(int32 CurrentTier) const
{
	if (CurrentTier < 0 || CurrentTier >= ShardCostPerTier.Num())
	{
		return -1; // Already at max tier
	}
	return ShardCostPerTier[CurrentTier];
}

FBBStatBonuses UBBEquipmentDefinition::GetTotalStats(int32 Tier, EEvolutionPath Evolution) const
{
	FBBStatBonuses Total = BaseStats;

	// Scale base stats by tier (simple linear scaling: each tier adds 50% of base)
	if (Tier > 0)
	{
		const float TierMultiplier = 1.0f + (Tier * 0.5f);
		Total.AttackPower *= TierMultiplier;
		Total.AbilityPower *= TierMultiplier;
		Total.MaxHealth *= TierMultiplier;
		Total.MaxShield *= TierMultiplier;
		Total.MoveSpeed *= TierMultiplier;
		Total.CritChance *= TierMultiplier;
		Total.CooldownReduction *= TierMultiplier;
	}

	// Add evolution bonuses if evolved
	if (Evolution != EEvolutionPath::None)
	{
		const FEquipmentEvolution& Evo = GetEvolution(Evolution);
		Total.AttackPower += Evo.BonusStats.AttackPower;
		Total.AbilityPower += Evo.BonusStats.AbilityPower;
		Total.MaxHealth += Evo.BonusStats.MaxHealth;
		Total.MaxShield += Evo.BonusStats.MaxShield;
		Total.MoveSpeed += Evo.BonusStats.MoveSpeed;
		Total.CritChance += Evo.BonusStats.CritChance;
		Total.CooldownReduction += Evo.BonusStats.CooldownReduction;
	}

	return Total;
}

FPrimaryAssetId UBBEquipmentDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("BBEquipment"), ItemID);
}
