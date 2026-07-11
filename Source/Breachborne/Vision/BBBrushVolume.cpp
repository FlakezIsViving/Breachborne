#include "BBBrushVolume.h"
#include "Components/BrushComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"

ABBBrushVolume::ABBBrushVolume()
{
	// Server overlap events only — client renders brush as a visual zone
	bReplicates = false;
	GetBrushComponent()->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	GetBrushComponent()->SetGenerateOverlapEvents(true);
}

void ABBBrushVolume::BeginPlay()
{
	Super::BeginPlay();
}

void ABBBrushVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (HasAuthority() && !bDestroyed)
	{
		SetBrushTag(OtherActor, true);
	}
}

void ABBBrushVolume::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);

	if (HasAuthority() && !bDestroyed)
	{
		SetBrushTag(OtherActor, false);
	}
}

void ABBBrushVolume::ApplyPowerDestruction_Implementation(const FBBPowerDestructionContext& Context)
{
	if (!HasAuthority() || bDestroyed)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		SetBrushTag(Actor, false);
	}

	bDestroyed = true;
	GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetBrushComponent()->SetGenerateOverlapEvents(false);
	SetActorHiddenInGame(true);

	UE_LOG(LogBreachborne, Log, TEXT("BrushVolume: %s destroyed by %s"), *GetName(), *GetNameSafe(Context.InstigatorActor));
}

void ABBBrushVolume::SetBrushTag(AActor* Actor, bool bInBrush)
{
	if (!Actor)
	{
		return;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor);
	if (!ASI)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (bInBrush)
	{
		ASC->AddLooseGameplayTag(BBGameplayTags::State_InBrush);
		UE_LOG(LogBreachborne, Verbose, TEXT("BrushVolume: %s entered brush"), *Actor->GetName());
	}
	else
	{
		ASC->RemoveLooseGameplayTag(BBGameplayTags::State_InBrush);
		UE_LOG(LogBreachborne, Verbose, TEXT("BrushVolume: %s left brush"), *Actor->GetName());
	}
}
