#include "GA_Eluna_Passive.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Abilities/BBHealthSet.h"
#include "Breachborne/Combat/BBHealEffect.h"
#include "Breachborne/Combat/BBPrimitiveBeamActor.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Wisp/BBWispPawn.h"
#include "Breachborne/Breachborne.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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
		0.1f,
		true
	);

	// Evaluate once on grant so the passive does not appear to wait for another ability.
	TickHeal();
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
		const UBBHealthSet* AllyHealth = AllyPS ? AllyPS->GetHealthSet() : nullptr;
		if (AllyPS && AllyPS->GetTeamID() == SourceTeam && AllyHealth
			&& AllyHealth->GetHealth() < AllyHealth->GetMaxHealth())
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

		// A small floating plus reads as healing without covering Eluna or the ground.
		const FVector IndicatorCenter = SourceLoc + FVector(0.0f, 0.0f, 125.0f);
		FActorSpawnParameters Params;
		Params.Owner = Hunter;
		Params.Instigator = Hunter;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBeamActor* Horizontal = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
			ABBPrimitiveBeamActor::StaticClass(), IndicatorCenter, FRotator::ZeroRotator, Params))
		{
			Horizontal->InitBeam(
				IndicatorCenter - FVector(32.0f, 0.0f, 0.0f),
				IndicatorCenter + FVector(32.0f, 0.0f, 0.0f),
				4.0f, 0.35f, FLinearColor(0.35f, 1.0f, 0.45f));
		}
		if (ABBPrimitiveBeamActor* Vertical = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBeamActor>(
			ABBPrimitiveBeamActor::StaticClass(), IndicatorCenter, FRotator::ZeroRotator, Params))
		{
			Vertical->InitBeam(
				IndicatorCenter - FVector(0.0f, 32.0f, 0.0f),
				IndicatorCenter + FVector(0.0f, 32.0f, 0.0f),
				4.0f, 0.35f, FLinearColor(0.35f, 1.0f, 0.45f));
		}
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
	const UBBAbilitySystemComponent* ASC = GetBBAbilitySystemComponent();
	const bool bCarrierCrowdControlled = ASC
		&& (ASC->HasMatchingGameplayTag(BBGameplayTags::State_Stunned)
			|| ASC->HasMatchingGameplayTag(BBGameplayTags::State_Spiked)
			|| ASC->HasMatchingGameplayTag(BBGameplayTags::State_Dazed)
			|| ASC->HasMatchingGameplayTag(BBGameplayTags::State_Hooked)
			|| ASC->HasMatchingGameplayTag(BBGameplayTags::State_Hudson_Hooked)
			|| ASC->HasMatchingGameplayTag(BBGameplayTags::State_Void_SingularityPulled));

	// Check if currently carrying a wisp — drop if too far
	if (CarriedWisp.IsValid())
	{
		if (CarriedWisp->GetCarrier() != Hunter)
		{
			// Another authoritative system dropped it; clear stale passive state.
			CarriedWisp = nullptr;
		}
		else
		{
			if (bCarrierCrowdControlled
				|| FVector::Dist(SourceLoc, CarriedWisp->GetActorLocation()) > WispDropDistance)
			{
				UE_LOG(LogBreachborne, Log, TEXT("Eluna Passive: Dropped carried wisp (CC=%d)"),
					bCarrierCrowdControlled ? 1 : 0);
				CarriedWisp->SetCarrier(nullptr);
				CarriedWisp = nullptr;
				FActorSpawnParameters Params;
				Params.Owner = Hunter;
				Params.Instigator = Hunter;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				if (ABBPrimitiveBurstActor* Pulse = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
					ABBPrimitiveBurstActor::StaticClass(), SourceLoc, FRotator::ZeroRotator, Params))
				{
					Pulse->InitBurst(SourceLoc, WispPickupRadius, 0.2f, FLinearColor(1.0f, 0.48f, 0.78f, 1.0f));
				}
			}
			return;
		}
	}
	if (bCarrierCrowdControlled)
	{
		return;
	}

	// A dash can collect the wisp between passive polling intervals. Adopt that
	// authoritative carry here so later CC/drop handling remains owned by passive.
	for (TActorIterator<ABBWispPawn> It(Hunter->GetWorld()); It; ++It)
	{
		if (It->GetCarrier() == Hunter)
		{
			CarriedWisp = *It;
			return;
		}
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
			FActorSpawnParameters Params;
			Params.Owner = Hunter;
			Params.Instigator = Hunter;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ABBPrimitiveBurstActor* Pulse = Hunter->GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
				ABBPrimitiveBurstActor::StaticClass(), SourceLoc, FRotator::ZeroRotator, Params))
			{
				Pulse->InitBurst(SourceLoc, WispPickupRadius, 0.25f, FLinearColor::White);
			}
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
