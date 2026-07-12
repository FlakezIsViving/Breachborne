#include "BBVoidOrbProjectile.h"

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
#include "Net/UnrealNetwork.h"

namespace
{
	bool ResolveVoidOrbTarget(AActor* Actor, int32 SourceTeamID, UAbilitySystemComponent*& OutASC)
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

ABBVoidOrbProjectile::ABBVoidOrbProjectile()
{
	ApplyOrbVisuals();
}

void ABBVoidOrbProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBVoidOrbProjectile, bCharged);
}

void ABBVoidOrbProjectile::InitVoidOrb(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageGE,
	TSubclassOf<UGameplayEffect> InSlowGE, float InDamage, int32 InTeamID, bool bInCharged)
{
	InitProjectile(InSourceASC, InDamageGE, InDamage, InTeamID);
	SlowEffectClass = InSlowGE;
	bCharged = bInCharged;
	if (bCharged && CollisionSphere)
	{
		CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
		CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	}
	ApplyOrbVisuals();
	ForceNetUpdate();
}

void ABBVoidOrbProjectile::OnRep_Charged()
{
	ApplyOrbVisuals();
}

void ABBVoidOrbProjectile::ApplyOrbVisuals()
{
	ProjectileMesh->SetRelativeScale3D(bCharged ? FVector(0.24f) : FVector(0.15f));
	BBPrimitiveVisuals::ApplyColor(ProjectileMesh, bCharged
		? FLinearColor(0.95f, 0.95f, 1.0f, 1.0f)
		: FLinearColor(0.42f, 0.12f, 1.0f, 1.0f));
}

void ABBVoidOrbProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !OtherActor || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}
	TWeakObjectPtr<AActor> WeakActor(OtherActor);
	if (HitActors.Contains(WeakActor))
	{
		return;
	}
	UAbilitySystemComponent* TargetASC = nullptr;
	if (!ResolveVoidOrbTarget(OtherActor, SourceTeamID, TargetASC))
	{
		return;
	}
	HitActors.Add(WeakActor);
	ApplyDamage(TargetASC);
	if (bCharged)
	{
		ApplySlow(TargetASC);
	}
	EmitDamageEvent(OtherActor);
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = GetInstigator();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveBurstActor* Burst = GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
		ABBPrimitiveBurstActor::StaticClass(), OtherActor->GetActorLocation(), FRotator::ZeroRotator, Params))
	{
		Burst->InitBurst(OtherActor->GetActorLocation(), bCharged ? 62.0f : 40.0f, 0.18f,
			bCharged ? FLinearColor::White : FLinearColor(0.55f, 0.36f, 0.96f, 1.0f));
	}
	Destroy();
}

void ABBVoidOrbProjectile::ApplyDamage(UAbilitySystemComponent* TargetASC) const
{
	if (!SourceASC.IsValid() || !TargetASC || !DamageEffectClass)
	{
		return;
	}
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, BaseDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void ABBVoidOrbProjectile::ApplySlow(UAbilitySystemComponent* TargetASC) const
{
	if (!SourceASC.IsValid() || !TargetASC || !SlowEffectClass)
	{
		return;
	}
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(SlowEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void ABBVoidOrbProjectile::EmitDamageEvent(AActor* TargetActor) const
{
	if (!SourceASC.IsValid())
	{
		return;
	}
	FGameplayEventData EventData;
	EventData.Instigator = GetOwner();
	EventData.Target = TargetActor;
	SourceASC->HandleGameplayEvent(BBGameplayTags::Event_Void_DamageDealt, &EventData);
}
