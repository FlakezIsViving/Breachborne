#include "BBCrystaLMBProjectile.h"

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
	bool ResolveCrystaProjectileTarget(AActor* Actor, int32 SourceTeamID, UAbilitySystemComponent*& OutASC)
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

ABBCrystaLMBProjectile::ABBCrystaLMBProjectile()
{
	ApplyShotVisuals();
}

void ABBCrystaLMBProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBCrystaLMBProjectile, bEmpoweredShot);
}

void ABBCrystaLMBProjectile::InitCrystaProjectile(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageGE,
	TSubclassOf<UGameplayEffect> InMarkGE, float InDamage, float InDetonationDamage, int32 InTeamID,
	bool bInCanDetonate, bool bInAppliesMark, bool bInEmpoweredShot)
{
	InitProjectile(InSourceASC, InDamageGE, InDamage, InTeamID);
	MarkEffectClass = InMarkGE;
	DetonationDamage = InDetonationDamage;
	bCanDetonate = bInCanDetonate;
	bAppliesMark = bInAppliesMark;
	bEmpoweredShot = bInEmpoweredShot;
	ApplyShotVisuals();
	ForceNetUpdate();
}

void ABBCrystaLMBProjectile::OnRep_EmpoweredShot()
{
	ApplyShotVisuals();
}

#if WITH_AUTOMATION_TESTS
void ABBCrystaLMBProjectile::ProcessOverlapForAutomation(AActor* OtherActor)
{
	OnProjectileOverlap(CollisionSphere.Get(), OtherActor, nullptr, 0, false, FHitResult());
}
#endif

void ABBCrystaLMBProjectile::ApplyShotVisuals()
{
	ProjectileMesh->SetRelativeScale3D(bEmpoweredShot ? FVector(0.24f) : FVector(0.15f));
	BBPrimitiveVisuals::ApplyColor(ProjectileMesh, bEmpoweredShot
		? FLinearColor(1.0f, 0.95f, 0.15f, 1.0f)
		: FLinearColor(0.05f, 0.9f, 1.0f, 1.0f));
}

void ABBCrystaLMBProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
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
	if (!ResolveCrystaProjectileTarget(OtherActor, SourceTeamID, TargetASC))
	{
		return;
	}

	HitActors.Add(WeakActor);
	const bool bDetonatedMark = DetonateMark(TargetASC, OtherActor);
	ApplyDamage(TargetASC, BaseDamage);
	if (bAppliesMark)
	{
		ApplyMark(TargetASC);
	}
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = GetInstigator();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveBurstActor* Burst = GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
		ABBPrimitiveBurstActor::StaticClass(), OtherActor->GetActorLocation(), FRotator::ZeroRotator, Params))
	{
		Burst->InitBurst(OtherActor->GetActorLocation(), bDetonatedMark ? 78.0f : 42.0f, 0.2f,
			bDetonatedMark ? FLinearColor(0.94f, 0.27f, 0.82f, 1.0f) : FLinearColor(0.13f, 0.83f, 0.93f, 1.0f));
	}

	Destroy();
}

void ABBCrystaLMBProjectile::ApplyDamage(UAbilitySystemComponent* TargetASC, float Damage) const
{
	if (!SourceASC.IsValid() || !TargetASC || !DamageEffectClass || Damage <= 0.0f)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, Damage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void ABBCrystaLMBProjectile::ApplyMark(UAbilitySystemComponent* TargetASC) const
{
	if (!SourceASC.IsValid() || !TargetASC || !MarkEffectClass)
	{
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(MarkEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (SpecHandle.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

bool ABBCrystaLMBProjectile::DetonateMark(UAbilitySystemComponent* TargetASC, AActor* TargetActor) const
{
	if (!bCanDetonate || !SourceASC.IsValid() || !TargetASC || !TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Crysta_Reverberation))
	{
		return false;
	}

	ApplyDamage(TargetASC, DetonationDamage);
	FGameplayTagContainer MarkTags;
	MarkTags.AddTag(BBGameplayTags::State_Crysta_Reverberation);
	TargetASC->RemoveActiveEffectsWithGrantedTags(MarkTags);

	FGameplayEventData EventData;
	EventData.Instigator = GetOwner();
	EventData.Target = TargetActor;
	SourceASC->HandleGameplayEvent(BBGameplayTags::Event_Crysta_ReverberationDetonated, &EventData);
	return true;
}
