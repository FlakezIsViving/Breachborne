#include "GA_Crysta_Passive.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"

UGA_Crysta_Passive::UGA_Crysta_Passive()
{
	bActivateOnInputHeld = false;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Crysta_Passive);
	AssetTags.AddTag(BBGameplayTags::Ability_Category_Passive);
	SetAssetTags(AssetTags);
}

void UGA_Crysta_Passive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}
	DetonationEventHandle = ASC->AddGameplayEventTagContainerDelegate(
		FGameplayTagContainer(BBGameplayTags::Event_Crysta_ReverberationDetonated),
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UGA_Crysta_Passive::OnReverberationDetonated));
}

void UGA_Crysta_Passive::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (DetonationEventHandle.IsValid())
		{
			ASC->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(BBGameplayTags::Event_Crysta_ReverberationDetonated), DetonationEventHandle);
		}
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Crysta_Passive::OnReverberationDetonated(FGameplayTag Tag, const FGameplayEventData* Payload)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	ReduceActiveCooldownTag(ASC, BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Primary, ShiftCooldownReduction);
	ReduceActiveCooldownTag(ASC, BBGameplayTags::Cooldown_Hunter_Crysta_Shift_Secondary, ShiftCooldownReduction);
	if (AHunterCharacter* Hunter = GetHunterCharacter())
	{
		ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Crysta_Passive_Detonate, Hunter->GetActorLocation());
	}
}

bool UGA_Crysta_Passive::ReduceActiveCooldownTag(UAbilitySystemComponent* ASC, FGameplayTag CooldownTag, float ReductionSeconds)
{
	if (!ASC || !CooldownTag.IsValid() || ReductionSeconds <= 0.0f)
	{
		return false;
	}

	bool bReducedAny = false;
	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(CooldownTag));
	for (const FActiveGameplayEffectHandle& Handle : ASC->GetActiveEffects(Query))
	{
		const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(Handle);
		if (!ActiveGE)
		{
			continue;
		}
		const float WorldTime = ASC->GetWorld()->GetTimeSeconds();
		const float Remaining = ActiveGE->GetDuration() - (WorldTime - ActiveGE->StartWorldTime);
		const float NewRemaining = FMath::Max(0.0f, Remaining - ReductionSeconds);
		if (NewRemaining <= 0.0f)
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
		else
		{
			ASC->ModifyActiveEffectStartTime(Handle, -ReductionSeconds);
		}
		bReducedAny = true;
	}
	return bReducedAny;
}
