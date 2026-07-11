#include "BBHealthSet.h"
#include "GameplayEffectExtension.h"
#include "BBGameplayTags.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Characters/GliderComponent.h"

UBBHealthSet::UBBHealthSet()
{
	InitHealth(500.0f);
	InitMaxHealth(500.0f);
	InitShield(0.0f);
	InitMaxShield(200.0f);
	InitArmor(0.0f);
	InitHealingReceivedMultiplier(1.0f);
	InitIncomingDamage(0.0f);
	InitIncomingHealing(0.0f);
}

void UBBHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetArmorAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetHealingReceivedMultiplierAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
	}
}

void UBBHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float DamageAmount = GetIncomingDamage();
		SetIncomingDamage(0.0f);

		if (DamageAmount > 0.0f)
		{
			UE_LOG(LogBreachborne, Verbose, TEXT("HealthSet: Processing %.1f IncomingDamage on %s"),
				DamageAmount, *GetOwningActor()->GetName());

			// Check for spiking: if the target is gliding, trigger spike (damage still applies)
			UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent();
			if (TargetASC && TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Gliding))
			{
				AActor* AvatarActor = TargetASC->GetAvatarActor();
				if (UGliderComponent* Glider = AvatarActor ? AvatarActor->FindComponentByClass<UGliderComponent>() : nullptr)
				{
					// Extract the instigator from the GE source
					AActor* DamageInstigator = Data.EffectSpec.GetEffectContext().GetInstigator();
					Glider->TriggerSpike(DamageInstigator);
				}
			}

			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				FGameplayEventData DamageEvent;
				DamageEvent.Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
				DamageEvent.Target = ASC->GetAvatarActor();
				DamageEvent.EventMagnitude = DamageAmount;
				ASC->HandleGameplayEvent(BBGameplayTags::Event_DamageTaken, &DamageEvent);
			}

			float RemainingDamage = DamageAmount;

			// Shield absorbs first
			const float CurrentShield = GetShield();
			if (CurrentShield > 0.0f)
			{
				const float ShieldAbsorb = FMath::Min(CurrentShield, RemainingDamage);
				SetShield(CurrentShield - ShieldAbsorb);
				RemainingDamage -= ShieldAbsorb;

				UE_LOG(LogBreachborne, Verbose, TEXT("HealthSet: Shield absorbed %.1f (Shield %.0f -> %.0f) | %.1f remaining"),
					ShieldAbsorb, CurrentShield, GetShield(), RemainingDamage);
			}

			// Overflow goes to health
			if (RemainingDamage > 0.0f)
			{
				const float CurrentHealth = GetHealth();
				const float NewHealth = FMath::Max(CurrentHealth - RemainingDamage, 0.0f);
				SetHealth(NewHealth);

				UE_LOG(LogBreachborne, Verbose, TEXT("HealthSet: %s | %.1f total damage | Shield absorbed %.1f | Health %.0f -> %.0f"),
					*GetOwningActor()->GetName(), DamageAmount,
					DamageAmount - RemainingDamage, CurrentHealth, NewHealth);

				if (NewHealth <= 0.0f)
				{
					// Add dead state tag to the owning ASC
					UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
					if (ASC && !ASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
					{
						ASC->AddLooseGameplayTag(BBGameplayTags::State_Dead);

						// Extract killer ASC from the damage effect context
						UAbilitySystemComponent* KillerASC = Data.EffectSpec.GetEffectContext().GetInstigatorAbilitySystemComponent();

						UE_LOG(LogBreachborne, Warning, TEXT("HealthSet: >>> HEALTH DEPLETED on %s <<< (killer ASC owner: %s)"),
							*GetOwningActor()->GetName(),
							KillerASC && KillerASC->GetOwner() ? *KillerASC->GetOwner()->GetName() : TEXT("None"));
						OnHealthDepleted.Broadcast(ASC, KillerASC);
					}
				}
			}
			else
			{
				UE_LOG(LogBreachborne, Verbose, TEXT("HealthSet: %s | %.1f damage fully absorbed by shield"), *GetOwningActor()->GetName(), DamageAmount);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingHealingAttribute())
	{
		const float HealAmount = GetIncomingHealing();
		SetIncomingHealing(0.0f);

		if (HealAmount > 0.0f)
		{
			const float NewHealth = FMath::Min(GetHealth() + HealAmount, GetMaxHealth());
			SetHealth(NewHealth);
		}
	}

	// Explicitly push current health values to the PlayerState's replicated proxy struct.
	// This bypasses the GAS attribute change delegate chain, which may not fire reliably
	// when SetHealth/SetShield is called from within PostGameplayEffectExecute
	// (aggregators may not exist yet for attributes modified via SetNumericAttributeBase).
	if (ABreachbornePlayerState* PS = Cast<ABreachbornePlayerState>(GetOwningActor()))
	{
		PS->UpdateHealthProxy();
	}
}
