#include "GA_Ghost_R.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Combat/BBNapalmZone.h"
#include "Breachborne/Combat/BBDamageEffect.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Breachborne/Breachborne.h"

UGA_Ghost_R::UGA_Ghost_R()
{
	AbilityInputTag = BBGameplayTags::InputTag_R;
	bActivateOnInputHeld = false;

	// Ultimate is ServerInitiated — no client prediction, server drives the AoE zone
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Ghost_R);
	SetAssetTags(AssetTags);

	CooldownTagContainer.AddTag(BBGameplayTags::Cooldown_Hunter_Ghost_R);

	DamageEffectClass = UBBDamageEffect::StaticClass();
	NapalmZoneClass = ABBNapalmZone::StaticClass();
}

bool UGA_Ghost_R::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// Level gate: ultimate requires Level >= 5 (unless dev bypass is active)
	if (!bIgnoreLevelGate && ActorInfo && ActorInfo->OwnerActor.IsValid())
	{
		const ABreachbornePlayerState* BBPS = Cast<ABreachbornePlayerState>(ActorInfo->OwnerActor.Get());
		if (BBPS && BBPS->GetHunterLevel() < RequiredLevel)
		{
			UE_LOG(LogBreachborne, Warning, TEXT("Ghost R blocked: Level %d < required %d"), BBPS->GetHunterLevel(), RequiredLevel);
			return false;
		}
	}

	return true;
}

void UGA_Ghost_R::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter || !Hunter->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FVector SpawnLocation = GetAimLocation();
	SpawnLocation.Z = Hunter->GetActorLocation().Z;
	PlayVisualMontage(BBGameplayTags::Ability_Hunter_Ghost_R, EBBAbilityAnimationPhase::Fire);
	ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Ghost_R_Warning, SpawnLocation, FVector::UpVector);

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const ABreachbornePlayerState* ShooterPS = GetBBPlayerState();

	if (SourceASC && ShooterPS && NapalmZoneClass)
	{
		// Spawn zone at cursor world position, flattened to hunter's Z
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Hunter;
		SpawnParams.Instigator = Hunter;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ABBNapalmZone* Zone = Hunter->GetWorld()->SpawnActor<ABBNapalmZone>(
			NapalmZoneClass,
			SpawnLocation,
			FRotator::ZeroRotator,
			SpawnParams
		);

		if (Zone)
		{
			Zone->InitZone(SourceASC, DamageEffectClass, DamagePerTick, ShooterPS->GetTeamID());

			UE_LOG(LogBreachborne, Log, TEXT("Ghost R: Spawned Napalm Zone at %s"), *SpawnLocation.ToString());
		}
	}

	// Zone manages its own lifetime — ability ends immediately
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

const FGameplayTagContainer* UGA_Ghost_R::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void UGA_Ghost_R::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = NewObject<UGameplayEffect>(GetTransientPackage());
	CooldownGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
	CooldownGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(CooldownDuration));
	UTargetTagsGameplayEffectComponent& TagComp = CooldownGE->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(BBGameplayTags::Cooldown_Hunter_Ghost_R);
	TagComp.SetAndApplyTargetTagChanges(TagContainer);

	ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CooldownGE, GetAbilityLevel());
}
