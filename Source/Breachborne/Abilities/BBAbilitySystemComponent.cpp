#include "BBAbilitySystemComponent.h"
#include "BBGameplayAbility.h"
#include "Net/UnrealNetwork.h"
#include "Breachborne/Breachborne.h"

UBBAbilitySystemComponent::UBBAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void UBBAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Disable SpawnedAttributes replication from the parent ASC.
	// SpawnedAttributes is private in UAbilitySystemComponent, so we can't use
	// DOREPLIFETIME_CONDITION. Instead, find it via reflection and remove from the rep list.
	//
	// Why: The parent replicates SpawnedAttributes (TArray<UAttributeSet*>) which causes
	// UObject pointer misrouting between PlayerState instances on clients.
	// Each client discovers its own AttributeSets locally via InitAbilityActorInfo().
	// Actual attribute values flow through proxy structs on PlayerState
	// (Rep_HealthData, Rep_CombatData, Rep_MovementData).
	FProperty* SpawnedAttrProp = UAbilitySystemComponent::StaticClass()->FindPropertyByName(
		FName(TEXT("SpawnedAttributes")));
	if (SpawnedAttrProp && SpawnedAttrProp->HasAnyPropertyFlags(CPF_Net))
	{
		OutLifetimeProps.RemoveAll([RepIdx = SpawnedAttrProp->RepIndex](const FLifetimeProperty& Prop)
		{
			return Prop.RepIndex == RepIdx;
		});
	}
}

bool UBBAbilitySystemComponent::TryActivateAbilityByInputTag(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return false;
	}

	bool bFoundMatchingAbility = false;
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (const UBBGameplayAbility* BBAbility = Cast<UBBGameplayAbility>(Spec.Ability))
		{
			if (Spec.DynamicAbilityTags.HasTagExact(InputTag) || BBAbility->GetAbilityInputTag().MatchesTagExact(InputTag))
			{
				bFoundMatchingAbility = true;
				if (Spec.IsActive())
				{
					// Ability is already active — route as InputPressed (e.g., Q re-press to toss)
					AbilitySpecInputPressed(const_cast<FGameplayAbilitySpec&>(Spec));
					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Spec.ActivationInfo.GetActivationPredictionKey());
					UE_LOG(LogBreachborne, Verbose, TEXT("TryActivateAbilityByInputTag: %s -> %s (Result: INPUT_PRESSED)"),
						*InputTag.ToString(), *BBAbility->GetClass()->GetName());
					return true;
				}
				else
				{
					const bool bSuccess = TryActivateAbility(Spec.Handle);
					UE_LOG(LogBreachborne, Verbose, TEXT("TryActivateAbilityByInputTag: %s -> %s (Result: %s)"),
						*InputTag.ToString(), *BBAbility->GetClass()->GetName(), bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
					if (bSuccess)
					{
						return true;
					}
				}
			}
		}
	}

	UE_LOG(LogBreachborne, Warning, TEXT("TryActivateAbilityByInputTag: No activatable ability succeeded for tag %s (FoundMatching: %d, Activatable: %d)"),
		*InputTag.ToString(), bFoundMatchingAbility ? 1 : 0, GetActivatableAbilities().Num());
	return false;
}

void UBBAbilitySystemComponent::InputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.IsActive())
		{
			if (const UBBGameplayAbility* BBAbility = Cast<UBBGameplayAbility>(Spec.Ability))
			{
				if (Spec.DynamicAbilityTags.HasTagExact(InputTag) || BBAbility->GetAbilityInputTag().MatchesTagExact(InputTag))
				{
					// Notify all active instances of this ability that input was released
					TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
					for (UGameplayAbility* Instance : Instances)
					{
						if (UBBGameplayAbility* BBInstance = Cast<UBBGameplayAbility>(Instance))
						{
							BBInstance->OnInputReleased();
						}
					}
				}
			}
		}
	}
}

void UBBAbilitySystemComponent::CancelAbilityByInputTag(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.IsActive())
		{
			if (const UBBGameplayAbility* BBAbility = Cast<UBBGameplayAbility>(Spec.Ability))
			{
				if (Spec.DynamicAbilityTags.HasTagExact(InputTag) || BBAbility->GetAbilityInputTag().MatchesTagExact(InputTag))
				{
					CancelAbilityHandle(Spec.Handle);
				}
			}
		}
	}
}
