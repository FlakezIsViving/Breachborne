#include "BBNukeBurnZone.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ABBNukeBurnZone::ABBNukeBurnZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	ZoneCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneCollision"));
	ZoneCollision->InitSphereRadius(500.0f);
	ZoneCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneCollision->SetCollisionObjectType(ECC_WorldDynamic);
	ZoneCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ZoneCollision->SetGenerateOverlapEvents(true);
	SetRootComponent(ZoneCollision);

	ZoneVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneVisualMesh"));
	ZoneVisualMesh->SetupAttachment(ZoneCollision);
	ZoneVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZoneVisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 6.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		ZoneVisualMesh->SetStaticMesh(CylinderMesh.Object);
		RefreshVisualScale(500.0f);
	}
}

void ABBNukeBurnZone::InitBurnZone(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageGE, float InRadius,
	float InDuration, float InDamagePerTick, float InTickInterval)
{
	SourceASC = InSourceASC;
	DamageEffectClass = InDamageGE;
	DamagePerTick = InDamagePerTick;
	DamageTickInterval = FMath::Max(0.05f, InTickInterval);

	const float Radius = FMath::Max(1.0f, InRadius);
	ZoneCollision->SetSphereRadius(Radius, true);
	RefreshVisualScale(Radius);

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(DamageTickHandle, this, &ABBNukeBurnZone::DamageTick, DamageTickInterval, true, 0.0f);
		GetWorldTimerManager().SetTimer(DurationHandle, this, &ABBNukeBurnZone::ExpireZone, FMath::Max(0.1f, InDuration), false);
	}
}

void ABBNukeBurnZone::DamageTick()
{
	if (!HasAuthority() || !SourceASC.IsValid() || !DamageEffectClass || DamagePerTick <= 0.0f)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	ZoneCollision->GetOverlappingActors(OverlappingActors);

	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor || Actor->IsActorBeingDestroyed())
		{
			continue;
		}

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC || TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
		{
			continue;
		}

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, DamagePerTick);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
}

void ABBNukeBurnZone::ExpireZone()
{
	GetWorldTimerManager().ClearTimer(DamageTickHandle);
	Destroy();
}

void ABBNukeBurnZone::RefreshVisualScale(float Radius)
{
	if (ZoneVisualMesh)
	{
		const float RadiusScale = Radius / 100.0f;
		ZoneVisualMesh->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, 0.04f));
	}
}
