#include "BBWispHealthSet.h"
#include "GameplayEffectExtension.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Core/BreachbornePlayerState.h"

UBBWispHealthSet::UBBWispHealthSet()
{
	InitWispHP(100.0f);
	InitMaxWispHP(100.0f);
	InitIncomingWispDrain(0.0f);
}

void UBBWispHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetWispHPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxWispHP());
	}
	else if (Attribute == GetMaxWispHPAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}

void UBBWispHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingWispDrainAttribute())
	{
		const float DrainAmount = GetIncomingWispDrain();
		SetIncomingWispDrain(0.0f);

		if (DrainAmount > 0.0f)
		{
			const float CurrentHP = GetWispHP();
			const float NewHP = FMath::Max(CurrentHP - DrainAmount, 0.0f);
			SetWispHP(NewHP);

			if (NewHP <= 0.0f)
			{
				UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
				if (ASC)
				{
					OnWispHealthDepleted.Broadcast(ASC);
				}
			}
		}
	}

	// Push updated wisp HP to the PlayerState's replicated proxy struct
	if (ABreachbornePlayerState* PS = Cast<ABreachbornePlayerState>(GetOwningActor()))
	{
		PS->UpdateWispHealthProxy();
	}
}
