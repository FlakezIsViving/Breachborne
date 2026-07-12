#include "BBVoidAnomalyPuddle.h"

#include "AbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Combat/BBGlideBot.h"
#include "Breachborne/Combat/BBPrimitiveVisuals.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Combat/BBTargetDummy.h"
#include "Breachborne/Combat/BBTestAlly.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	bool ResolveVoidPuddleTarget(AActor* Actor, int32 SourceTeamID, UAbilitySystemComponent*& OutASC)
	{
		OutASC = nullptr;
		int32 TargetTeamID = SourceTeamID;
		if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(Actor))
		{
			if (const ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>())
			{
				TargetTeamID = PS->GetTeamID();
			}
			OutASC = Hunter->GetAbilitySystemComponent();
		}
		else if (ABBTargetDummy* Dummy = Cast<ABBTargetDummy>(Actor))
		{
			TargetTeamID = Dummy->GetTeamID();
			OutASC = Dummy->GetAbilitySystemComponent();
		}
		else if (ABBGlideBot* GlideBot = Cast<ABBGlideBot>(Actor))
		{
			TargetTeamID = GlideBot->GetTeamID();
			OutASC = GlideBot->GetAbilitySystemComponent();
		}
		else if (ABBTestAlly* TestAlly = Cast<ABBTestAlly>(Actor))
		{
			TargetTeamID = TestAlly->GetTeamID();
			OutASC = TestAlly->GetAbilitySystemComponent();
		}
		return OutASC && TargetTeamID != SourceTeamID && !OutASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead);
	}
}

ABBVoidAnomalyPuddle::ABBVoidAnomalyPuddle()
{
	bReplicates = true;

	DamageSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DamageSphere"));
	DamageSphere->InitSphereRadius(Radius);
	DamageSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(DamageSphere);

	PuddleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PuddleMesh"));
	PuddleMesh->SetupAttachment(DamageSphere);
	PuddleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PuddleMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		PuddleMesh->SetStaticMesh(CylinderMesh.Object);
	}
	BBPrimitiveVisuals::ApplyColor(PuddleMesh, FLinearColor(0.48f, 0.02f, 0.85f, 1.0f));
	RefreshVisuals();
}

void ABBVoidAnomalyPuddle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBVoidAnomalyPuddle, Radius);
}

void ABBVoidAnomalyPuddle::InitPuddle(UAbilitySystemComponent* InSourceASC, AActor* InInstigatorActor, TSubclassOf<UGameplayEffect> InDamageGE,
	int32 InTeamID, float InRadius, float InDuration, float InTickDamage, float InBurstDamage)
{
	SourceASC = InSourceASC;
	InstigatorActor = InInstigatorActor;
	DamageEffectClass = InDamageGE;
	SourceTeamID = InTeamID;
	Radius = InRadius;
	TickDamage = InTickDamage;
	BurstDamage = InBurstDamage;
	RefreshVisuals();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(TickHandle, this, &ABBVoidAnomalyPuddle::DamageTick, 0.5f, true, 0.0f);
		GetWorldTimerManager().SetTimer(BurstHandle, this, &ABBVoidAnomalyPuddle::Burst, FMath::Max(0.1f, InDuration), false);
		ForceNetUpdate();
	}
}

void ABBVoidAnomalyPuddle::Burst()
{
	if (!HasAuthority() || bBurst)
	{
		return;
	}
	bBurst = true;
	GetWorldTimerManager().ClearTimer(TickHandle);
	GetWorldTimerManager().ClearTimer(BurstHandle);

	TArray<AActor*> Actors;
	DamageSphere->GetOverlappingActors(Actors);
	TSet<AActor*> HitActors;
	for (AActor* Actor : Actors)
	{
		if (!Actor || HitActors.Contains(Actor))
		{
			continue;
		}
		HitActors.Add(Actor);
		UAbilitySystemComponent* TargetASC = nullptr;
		if (ResolveVoidPuddleTarget(Actor, SourceTeamID, TargetASC))
		{
			ApplyDamage(TargetASC, BurstDamage);
		}
	}
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = GetInstigator();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveBurstActor* BurstVisual = GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
		ABBPrimitiveBurstActor::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params))
	{
		BurstVisual->InitBurst(GetActorLocation(), Radius, 0.32f,
			FLinearColor(1.0f, 0.31f, 0.85f, 1.0f));
	}
	Destroy();
}

void ABBVoidAnomalyPuddle::OnRep_Radius()
{
	RefreshVisuals();
}

void ABBVoidAnomalyPuddle::DamageTick()
{
	TArray<AActor*> Actors;
	DamageSphere->GetOverlappingActors(Actors);
	for (AActor* Actor : Actors)
	{
		UAbilitySystemComponent* TargetASC = nullptr;
		if (ResolveVoidPuddleTarget(Actor, SourceTeamID, TargetASC))
		{
			ApplyDamage(TargetASC, TickDamage);
		}
	}
}

void ABBVoidAnomalyPuddle::RefreshVisuals()
{
	if (DamageSphere)
	{
		DamageSphere->SetSphereRadius(Radius, true);
	}
	if (PuddleMesh)
	{
		const float RadiusScale = Radius / 50.0f;
		PuddleMesh->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, 0.02f));
		BBPrimitiveVisuals::ApplyColor(PuddleMesh, FLinearColor(0.48f, 0.02f, 0.85f, 1.0f));
	}
}

void ABBVoidAnomalyPuddle::ApplyDamage(UAbilitySystemComponent* TargetASC, float Damage) const
{
	if (!SourceASC.IsValid() || !TargetASC || !DamageEffectClass || Damage <= 0.0f)
	{
		return;
	}
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, Damage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}
