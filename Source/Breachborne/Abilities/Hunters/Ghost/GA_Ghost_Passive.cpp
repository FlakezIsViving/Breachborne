#include "GA_Ghost_Passive.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Breachborne/Breachborne.h"

UGA_Ghost_Passive::UGA_Ghost_Passive()
{
	// No input tag — passive is activated by the server after grant
	bActivateOnInputHeld = false;

	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Ghost_Passive);
	SetAssetTags(AssetTags);
	PulseVisualClass = ABBPrimitiveBurstActor::StaticClass();
}

void UGA_Ghost_Passive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Add the passive identifying tag to the ASC
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->AddLooseGameplayTag(BBGameplayTags::Ability_Hunter_Ghost_Passive);
	}

	UE_LOG(LogBreachborne, Log, TEXT("Ghost Passive activated — listening for kill events"));

	// Start listening for kill events
	WaitForNextKill();

	// Passive stays active — do NOT call EndAbility
}

void UGA_Ghost_Passive::OnKillEvent(FGameplayEventData Payload)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// Find and remove the active Shift cooldown GE
	// The cooldown GE grants its tag via InheritableOwnedTagsContainer, so query OwningTagQuery
	FGameplayTagContainer ShiftCooldownTags;
	ShiftCooldownTags.AddTag(BBGameplayTags::Cooldown_Hunter_Ghost_Shift);

	FGameplayEffectQuery Query;
	Query.OwningTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(ShiftCooldownTags);

	TArray<FActiveGameplayEffectHandle> ActiveCooldowns = ASC->GetActiveEffects(Query);

	if (ActiveCooldowns.Num() > 0)
	{
		for (const FActiveGameplayEffectHandle& Handle : ActiveCooldowns)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}

		UE_LOG(LogBreachborne, Log, TEXT("Ghost Passive: Kill detected — Shift cooldown RESET (%d effects removed)"), ActiveCooldowns.Num());
	}
	else
	{
		UE_LOG(LogBreachborne, Verbose, TEXT("Ghost Passive: Kill detected but Shift not on cooldown"));
	}

	if (AHunterCharacter* Hunter = GetHunterCharacter())
	{
		PlayVisualMontage(BBGameplayTags::Ability_Hunter_Ghost_Passive, EBBAbilityAnimationPhase::PassivePulse);
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Ghost_Passive_Pulse, Hunter->GetActorLocation(), FVector::UpVector);
		if (Hunter->HasAuthority() && PulseVisualClass)
		{
			const FVector PulseLocation = Hunter->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f);
			FActorSpawnParameters VisualParams;
			VisualParams.Owner = Hunter;
			VisualParams.Instigator = Hunter;
			VisualParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ABBPrimitiveBurstActor* Pulse = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
				PulseVisualClass, PulseLocation, FRotator::ZeroRotator, VisualParams))
			{
				Pulse->InitBurst(PulseLocation, 85.0f, 0.32f, FLinearColor(0.90f, 1.0f, 1.0f, 1.0f));
			}
		}
	}

	// Re-register to listen for the next kill (WaitGameplayEvent is one-shot)
	WaitForNextKill();
}

void UGA_Ghost_Passive::WaitForNextKill()
{
	UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		BBGameplayTags::Event_Kill
	);

	if (WaitTask)
	{
		WaitTask->EventReceived.AddDynamic(this, &UGA_Ghost_Passive::OnKillEvent);
		WaitTask->ReadyForActivation();
	}
}
