#include "BBCreepCharacter.h"
#include "BBCreepCamp.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Abilities/BBCombatSet.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBDamageExecution.h"
#include "Breachborne/Items/BBInventoryManager.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Breachborne.h"

ABBCreepCharacter::ABBCreepCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = false; // Only replicate to nearby clients

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	HealthSet  = CreateDefaultSubobject<UBBHealthSet>(TEXT("HealthSet"));
	CombatSet  = CreateDefaultSubobject<UBBCombatSet>(TEXT("CombatSet"));
}

UAbilitySystemComponent* ABBCreepCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABBCreepCharacter::InitForCamp(ABBCreepCamp* InOwningCamp, FVector InHomeLocation)
{
	OwningCamp     = InOwningCamp;
	HomeLocation   = InHomeLocation;
}

void ABBCreepCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// Set max health from our config property
	if (HealthSet)
	{
		// Apply a startup GE to initialize health/maxhealth
		UGameplayEffect* InitGE = NewObject<UGameplayEffect>(this, TEXT("GE_CreepInit"));
		InitGE->DurationPolicy = EGameplayEffectDurationType::Instant;

		auto AddMod = [&](FGameplayAttribute Attr, float Value)
		{
			FGameplayModifierInfo Mod;
			Mod.Attribute           = Attr;
			Mod.ModifierOp          = EGameplayModOp::Override;
			Mod.ModifierMagnitude   = FGameplayEffectModifierMagnitude(FScalableFloat(Value));
			InitGE->Modifiers.Add(Mod);
		};

		AddMod(UBBHealthSet::GetMaxHealthAttribute(), MaxHealth);
		AddMod(UBBHealthSet::GetHealthAttribute(),    MaxHealth);

		FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
		AbilitySystemComponent->ApplyGameplayEffectToSelf(InitGE, 1.0f, Ctx);
	}

	// Monitor health for death
	HealthChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		UBBHealthSet::GetHealthAttribute()).AddUObject(this, &ABBCreepCharacter::OnHealthChanged);

	// Track last damage instigator so we can reward the killer without needing
	// FGameplayEffectModCallbackData (UE5.7 include-chain workaround).
	AbilitySystemComponent->OnGameplayEffectAppliedDelegateToSelf.AddUObject(
		this, &ABBCreepCharacter::OnEffectAppliedToSelf);
}

void ABBCreepCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Start BehaviorTree via the AI controller
	if (AAIController* AIC = Cast<AAIController>(NewController))
	{
		if (UBehaviorTree* BT = Cast<UBehaviorTree>(StaticLoadObject(UBehaviorTree::StaticClass(), nullptr,
			TEXT("/Game/Breachborne/AI/BT_Creep"))))
		{
			AIC->RunBehaviorTree(BT);

			// Seed the blackboard with our home position and leash config
			if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
			{
				BB->SetValueAsVector(FName(TEXT("HomeLocation")),  HomeLocation);
				BB->SetValueAsFloat(FName(TEXT("LeashRadius")),    LeashRadius);
				BB->SetValueAsFloat(FName(TEXT("AttackRange")),    AttackRange);
			}
		}
		else
		{
			UE_LOG(LogBreachborne, Warning, TEXT("BBCreepCharacter: BT_Creep not found — AI will not tick. Create it in Content/Breachborne/AI/"));
		}
	}
}

void ABBCreepCharacter::OnEffectAppliedToSelf(UAbilitySystemComponent* Source,
                                               const FGameplayEffectSpec& SpecApplied,
                                               FActiveGameplayEffectHandle ActiveHandle)
{
	// Cache whoever last applied a GE to us — this will be the killer if health hits 0 next.
	// Using the spec context avoids FGameplayEffectModCallbackData entirely.
	if (Source && Source != AbilitySystemComponent)
	{
		UAbilitySystemComponent* InstigatorASC = SpecApplied.GetContext().GetInstigatorAbilitySystemComponent();
		if (InstigatorASC)
		{
			LastDamageInstigatorASC = InstigatorASC;
		}
	}
}

void ABBCreepCharacter::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue > 0.0f || !HasAuthority())
	{
		return;
	}

	// Tag as dead so storm damage and other systems skip us
	AbilitySystemComponent->AddLooseGameplayTag(BBGameplayTags::State_Dead);

	// Killer was cached by OnEffectAppliedToSelf when the fatal damage GE was applied.
	UAbilitySystemComponent* KillerASC = LastDamageInstigatorASC.Get();

	// Reward the killer
	if (KillerASC)
	{
		ABreachbornePlayerState* KillerPS = Cast<ABreachbornePlayerState>(KillerASC->GetOwnerActor());
		if (KillerPS)
		{
			FBBInventoryManager::AddGold(KillerPS, GoldReward);
			FBBInventoryManager::AddShards(KillerPS, ShardReward);
			UE_LOG(LogBreachborne, Log, TEXT("Creep killed by %s — rewarded %d gold, %d shards"),
				*KillerPS->GetPlayerName(), GoldReward, ShardReward);
		}
	}

	// Notify camp and broadcast the death delegate
	if (OwningCamp)
	{
		OwningCamp->OnCreepDied(this);
	}

	OnCreepKilled.Broadcast(this, KillerASC);

	// Disable collisions, hide, then destroy
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetLifeSpan(2.0f); // Give time for death FX via GameplayCue
}

void ABBCreepCharacter::PerformAttack(AActor* Target)
{
	if (!Target || !AbilitySystemComponent || !HasAuthority())
	{
		return;
	}

	IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(Target);
	if (!TargetASI)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		return;
	}

	// Skip dead targets
	if (TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
	{
		return;
	}

	// Build an instant damage GE using our DamageExecution
	UGameplayEffect* AttackGE = NewObject<UGameplayEffect>(this, TEXT("GE_CreepAttack"));
	AttackGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UBBDamageExecution::StaticClass();
	AttackGE->Executions.Add(ExecDef);

	FGameplayEffectContextHandle Ctx = AbilitySystemComponent->MakeEffectContext();
	Ctx.AddInstigator(this, this);

	FGameplayEffectSpec Spec(AttackGE, Ctx, 1.0f);
	Spec.SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, AttackDamage);

	TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
}
