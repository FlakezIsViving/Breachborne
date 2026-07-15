#include "GA_Kingpin_Passive.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Breachborne.h"

UGA_Kingpin_Passive::UGA_Kingpin_Passive()
{
	bActivateOnInputHeld = false;
	NetExecutionPolicy   = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Kingpin_Passive);
	AssetTags.AddTag(BBGameplayTags::Ability_Category_Passive);
	SetAssetTags(AssetTags);
}

void UGA_Kingpin_Passive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// Don't call CommitAbility — passives don't consume resources

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// Listen for Event_CCApplied dispatched whenever Kingpin lands CC on an enemy
	CCEventHandle = ASC->AddGameplayEventTagContainerDelegate(
		FGameplayTagContainer(BBGameplayTags::Event_CCApplied),
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UGA_Kingpin_Passive::OnCCApplied));

	UE_LOG(LogBreachborne, Log, TEXT("Kingpin Passive (Iron Will): Active"));

	// Do NOT call EndAbility — passive stays active for the match duration
}

void UGA_Kingpin_Passive::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo(); ASC && CCEventHandle.IsValid())
	{
		ASC->RemoveGameplayEventTagContainerDelegate(
			FGameplayTagContainer(BBGameplayTags::Event_CCApplied), CCEventHandle);
		CCEventHandle.Reset();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Kingpin_Passive::OnCCApplied(FGameplayTag Tag, const FGameplayEventData* Payload)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// Iterate over all active gameplay effects that are cooldowns (tagged with Effect_Cooldown)
	// and shorten their remaining duration by CooldownReduction.
	// We do this by applying a negative-duration instant GE that removes time from HasDuration GEs.
	// The canonical approach: remove the GE and reapply with adjusted duration.
	// Simpler and battle-tested in this codebase: apply a tiny HasDuration GE that stacks
	// on existing cooldowns. Since UE doesn't natively allow "reduce duration", we instead
	// enumerate active handles and modify remaining time via FActiveGameplayEffect internals.

	FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(
		FGameplayTagContainer(BBGameplayTags::Effect_Cooldown));

	TArray<FActiveGameplayEffectHandle> CooldownHandles = ASC->GetActiveEffects(CooldownQuery);

	for (const FActiveGameplayEffectHandle& CDHandle : CooldownHandles)
	{
		const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(CDHandle);
		if (!ActiveGE)
		{
			continue;
		}

		// Calculate new remaining duration
		const float WorldTime      = ASC->GetWorld()->GetTimeSeconds();
		const float StartTime      = ActiveGE->StartWorldTime;
		const float OrigDuration   = ActiveGE->GetDuration();
		const float Elapsed        = WorldTime - StartTime;
		const float Remaining      = OrigDuration - Elapsed;
		const float NewRemaining   = FMath::Max(0.0f, Remaining - CooldownReduction);

		if (NewRemaining <= 0.0f)
		{
			// Cooldown would expire — just remove it
			ASC->RemoveActiveGameplayEffect(CDHandle);
		}
		else
		{
			// Shift the start time forward to effectively reduce remaining duration
			// This is safe as of UE 5.x — FActiveGameplayEffect::StartWorldTime is mutable via const_cast
			// in a controlled way here (no network replication side effects since cooldowns replicate their
			// *applied* state, not remaining time directly).
			const_cast<FActiveGameplayEffect*>(ActiveGE)->StartWorldTime = WorldTime - (OrigDuration - NewRemaining);
		}
	}

	if (CooldownHandles.Num() > 0)
	{
		UE_LOG(LogBreachborne, Log, TEXT("Kingpin Passive: CC landed — reduced %d cooldown(s) by %.1fs"),
			CooldownHandles.Num(), CooldownReduction);
	}

	if (AHunterCharacter* Hunter = GetHunterCharacter())
	{
		PlayVisualMontage(BBGameplayTags::Ability_Hunter_Kingpin_Passive, EBBAbilityAnimationPhase::PassivePulse);
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBurstActor* Pulse = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
			ABBPrimitiveBurstActor::StaticClass(), Hunter->GetActorLocation(), FRotator::ZeroRotator, Params))
		{
			Pulse->InitBurst(Hunter->GetActorLocation(), 150.0f, 0.22f,
				FLinearColor(1.0f, 0.69f, 0.13f, 1.0f));
		}
	}
}
