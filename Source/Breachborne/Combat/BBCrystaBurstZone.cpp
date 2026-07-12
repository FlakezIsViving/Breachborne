#include "BBCrystaBurstZone.h"

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
	bool ResolveCrystaZoneTarget(AActor* Actor, int32 SourceTeamID, UAbilitySystemComponent*& OutASC)
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

ABBCrystaBurstZone::ABBCrystaBurstZone()
{
	bReplicates = true;

	BurstSphere = CreateDefaultSubobject<USphereComponent>(TEXT("BurstSphere"));
	BurstSphere->InitSphereRadius(Radius);
	BurstSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BurstSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	BurstSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(BurstSphere);

	ZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneMesh"));
	ZoneMesh->SetupAttachment(BurstSphere);
	ZoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZoneMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		ZoneMesh->SetStaticMesh(CylinderMesh.Object);
	}
	BBPrimitiveVisuals::ApplyColor(ZoneMesh, FLinearColor(0.0f, 0.75f, 1.0f, 1.0f));
	RefreshVisuals();
}

void ABBCrystaBurstZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBCrystaBurstZone, Radius);
}

void ABBCrystaBurstZone::InitBurstZone(AHunterCharacter* InSourceHunter, UAbilitySystemComponent* InSourceASC,
	TSubclassOf<UGameplayEffect> InDamageGE, TSubclassOf<UGameplayEffect> InMarkGE,
	int32 InTeamID, float InRadius, float InDuration, float InBurstDamage)
{
	SourceHunter = InSourceHunter;
	SourceASC = InSourceASC;
	DamageEffectClass = InDamageGE;
	MarkEffectClass = InMarkGE;
	SourceTeamID = InTeamID;
	Radius = InRadius;
	BurstDamage = InBurstDamage;
	RefreshVisuals();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(MarkTickHandle, this, &ABBCrystaBurstZone::MarkTick, 0.25f, true, 0.0f);
		GetWorldTimerManager().SetTimer(DetonateTimerHandle, this, &ABBCrystaBurstZone::Detonate, FMath::Max(0.1f, InDuration), false);
		ForceNetUpdate();
	}
}

void ABBCrystaBurstZone::Detonate()
{
	if (!HasAuthority() || bDetonated)
	{
		return;
	}
	bDetonated = true;
	GetWorldTimerManager().ClearTimer(MarkTickHandle);
	GetWorldTimerManager().ClearTimer(DetonateTimerHandle);

	TArray<AActor*> Overlaps;
	BurstSphere->GetOverlappingActors(Overlaps);
	TSet<AActor*> HitActors;
	for (AActor* Actor : Overlaps)
	{
		if (!Actor || HitActors.Contains(Actor))
		{
			continue;
		}
		HitActors.Add(Actor);
		UAbilitySystemComponent* TargetASC = nullptr;
		if (ResolveCrystaZoneTarget(Actor, SourceTeamID, TargetASC))
		{
			ApplyDamage(TargetASC, BurstDamage);
		}
	}
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = GetInstigator();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveBurstActor* Burst = GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
		ABBPrimitiveBurstActor::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params))
	{
		Burst->InitBurst(GetActorLocation(), Radius, 0.3f, FLinearColor(0.94f, 0.27f, 0.82f, 1.0f));
	}

	Destroy();
}

void ABBCrystaBurstZone::OnRep_Radius()
{
	RefreshVisuals();
}

void ABBCrystaBurstZone::MarkTick()
{
	TArray<AActor*> Overlaps;
	BurstSphere->GetOverlappingActors(Overlaps);
	for (AActor* Actor : Overlaps)
	{
		UAbilitySystemComponent* TargetASC = nullptr;
		if (ResolveCrystaZoneTarget(Actor, SourceTeamID, TargetASC))
		{
			ApplyMark(TargetASC);
		}
	}
}

void ABBCrystaBurstZone::RefreshVisuals()
{
	if (BurstSphere)
	{
		BurstSphere->SetSphereRadius(Radius, true);
	}
	if (ZoneMesh)
	{
		const float RadiusScale = Radius / 50.0f;
		ZoneMesh->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, 0.025f));
		BBPrimitiveVisuals::ApplyColor(ZoneMesh, FLinearColor(0.0f, 0.75f, 1.0f, 1.0f));
	}
}

void ABBCrystaBurstZone::ApplyMark(UAbilitySystemComponent* TargetASC) const
{
	if (!SourceASC.IsValid() || !TargetASC || !MarkEffectClass)
	{
		return;
	}

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(MarkEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void ABBCrystaBurstZone::ApplyDamage(UAbilitySystemComponent* TargetASC, float Damage) const
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
