#include "BBBossCharacter.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Items/BBWorldItem.h"
#include "Breachborne/Breachborne.h"

ABBBossCharacter::ABBBossCharacter()
{
	// Boss defaults — BP subclass should tune these
	MaxHealth     = 2000.0f;
	AttackDamage  = 35.0f;
	AttackCooldown = 1.5f;
	LeashRadius   = 2500.0f;
	ShardReward   = 15;
	GoldReward    = 40;
}

void ABBBossCharacter::BeginPlay()
{
	Super::BeginPlay(); // Initializes ASC, attribute sets, health monitoring

	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	// Subscribe to health changes to trigger enrage at threshold
	BossHealthChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UBBHealthSet::GetHealthAttribute()).AddUObject(this, &ABBBossCharacter::OnBossHealthChanged);
}

void ABBBossCharacter::OnBossHealthChanged(const FOnAttributeChangeData& Data)
{
	if (bEnraged || !HasAuthority())
	{
		return;
	}

	if (Data.NewValue <= 0.0f)
	{
		// Death — spawn loot
		SpawnLootDrop();
		return;
	}

	// Enrage check
	const float MaxHP = AbilitySystemComponent->GetNumericAttribute(UBBHealthSet::GetMaxHealthAttribute());
	if (MaxHP > 0.0f && (Data.NewValue / MaxHP) <= EnrageThreshold)
	{
		bEnraged = true;
		AttackDamage *= EnrageDamageMultiplier;
		// Halve the attack cooldown (double attack rate)
		AttackCooldown = FMath::Max(0.3f, AttackCooldown * 0.5f);

		UE_LOG(LogBreachborne, Warning, TEXT("Boss [%s]: ENRAGED — damage %.0f, cooldown %.2fs"),
			*GetName(), AttackDamage, AttackCooldown);
	}
}

void ABBBossCharacter::SpawnLootDrop()
{
	if (!ItemDropClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector DropLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ABBWorldItem* Drop = World->SpawnActor<ABBWorldItem>(ItemDropClass, DropLocation, FRotator::ZeroRotator, Params);
	if (Drop)
	{
		Drop->SetPickupItemID(DropItemID);
		UE_LOG(LogBreachborne, Log, TEXT("Boss [%s]: Spawned loot drop '%s' at boss location"),
			*GetName(), *DropItemID.ToString());
	}
}
