#include "GA_Eluna_Passive.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBHealEffect.h"
#include "Breachborne/Wisp/BBWispPawn.h"
#include "Breachborne/Breachborne.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_Eluna_Passive::UGA_Eluna_Passive()
{
	AbilityInputTag = FGameplayTag();
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(BBGameplayTags::Ability_Hunter_Eluna_Passive);
	SetAssetTags(AssetTags);

	HealEffectClass = UBBHealEffect::StaticClass();
}

void UGA_Eluna_Passive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{

	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}


	Hunter->GetWorldTimerManager().SetTimer(
		HealTimerHandle,
		FTimerDelegate::CreateUObject(this, &UGA_Eluna_Passive::TickHeal),
		HealInterval,
		true
	);

	Hunter->GetWorldTimerManager().SetTimer(
		WispPickupTimerHandle,
		FTimerDelegate::CreateUObject(this, &UGA_Eluna_Passive::TickWispPickup),
		0.5f,
		true
	);
}

void UGA_Eluna_Passive::TickHeal()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		return;
	}

	ABreachbornePlayerState* SourcePS = GetBBPlayerState();
	if (!SourcePS)
	{
		return;
	}

	UBBAbilitySystemComponent* ASC = GetBBAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const FVector SourceLoc = Hunter->GetActorLocation();
	const int32 SourceTeam = SourcePS->GetTeamID();

	// Find all allies in radius
	TArray<AActor*> OverlappingActors;
	UKismetSystemLibrary::SphereOverlapActors(
		Hunter->GetWorld(),
		SourceLoc,
		HealRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>{ UEngineTypes::ConvertToObjectType(ECC_Pawn) },
		AHunterCharacter::StaticClass(),
		TArray<AActor*>{ Hunter },
		OverlappingActors
	);

	bool bAppliedHeal = false;
	for (AActor* Actor : OverlappingActors)
	{
		AHunterCharacter* Ally = Cast<AHunterCharacter>(Actor);
		if (!Ally || Ally == Hunter)
		{
			continue;
		}

		ABreachbornePlayerState* AllyPS = Ally->GetPlayerState<ABreachbornePlayerState>();
		if (AllyPS && AllyPS->GetTeamID() == SourceTeam)
		{
			// Apply heal effect to ally
			if (HealEffectClass)
			{
				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(HealEffectClass, 1.0f, ASC->MakeEffectContext());
				if (SpecHandle.IsValid())
				{
					SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_HealAmount.GetTag(), HealAmount);
					ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), Ally->GetAbilitySystemComponent());
					bAppliedHeal = true;
				}
			}
		}
	}

	if (bAppliedHeal)
	{
		PlayVisualMontage(BBGameplayTags::Ability_Hunter_Eluna_Passive, EBBAbilityAnimationPhase::PassivePulse);
	}
}

void UGA_Eluna_Passive::TickWispPickup()
{
	AHunterCharacter* Hunter = GetHunterCharacter();
	if (!Hunter)
	{
		return;
	}

	ABreachbornePlayerState* SourcePS = GetBBPlayerState();
	if (!SourcePS)
	{
		return;
	}

	const FVector SourceLoc = Hunter->GetActorLocation();
	const int32 SourceTeam = SourcePS->GetTeamID();

	// Check if currently carrying a wisp — drop if too far
	if (CarriedWisp.IsValid())
	{
		if (FVector::Dist(SourceLoc, CarriedWisp->GetActorLocation()) > WispDropDistance)
		{
			CarriedWisp->SetCarrier(nullptr);
			CarriedWisp = nullptr;

		}
		return;
	}

	// Find nearby ally wisps
	TArray<AActor*> OverlappingActors;
	UKismetSystemLibrary::SphereOverlapActors(
		Hunter->GetWorld(),
		SourceLoc,
		WispPickupRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>{ UEngineTypes::ConvertToObjectType(ECC_Pawn) },
		ABBWispPawn::StaticClass(),
		TArray<AActor*>{ Hunter },
		OverlappingActors
	);

	for (AActor* Actor : OverlappingActors)
	{
		ABBWispPawn* Wisp = Cast<ABBWispPawn>(Actor);
		if (!Wisp || Wisp->GetCarrier())
		{
			continue;
		}

		ABreachbornePlayerState* WispOwnerPS = Wisp->GetOwningPlayerState();
		if (WispOwnerPS && WispOwnerPS->GetTeamID() == SourceTeam)
		{
			// Pick up the wisp
			Wisp->SetCarrier(Hunter);
			CarriedWisp = Wisp;
			PlayVisualMontage(BBGameplayTags::Ability_Hunter_Eluna_Passive, EBBAbilityAnimationPhase::PassivePulse);
			ExecuteVisualCue(BBGameplayTags::GameplayCue_Hunter_Eluna_Passive_Carry, Hunter->GetActorLocation(), FVector::UpVector);
			UE_LOG(LogBreachborne, Log, TEXT("Eluna Passive: Picked up wisp of %s"), *WispOwnerPS->GetPlayerName());
			break; // Only carry one wisp at a time
		}
	}
}

void UGA_Eluna_Passive::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AHunterCharacter* Hunter = GetHunterCharacter())
	{
		Hunter->GetWorldTimerManager().ClearTimer(HealTimerHandle);
		Hunter->GetWorldTimerManager().ClearTimer(WispPickupTimerHandle);
	}

	// Drop any carried wisp
	if (CarriedWisp.IsValid())
	{
		CarriedWisp->SetCarrier(nullptr);
		CarriedWisp = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
