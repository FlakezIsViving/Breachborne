#include "BBPowerNukeStrikeActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BBNukeBurnZone.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/World/BBPowerDestructibleInterface.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABBPowerNukeStrikeActor::ABBPowerNukeStrikeActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WarningRadius = CreateDefaultSubobject<USphereComponent>(TEXT("WarningRadius"));
	WarningRadius->SetupAttachment(SceneRoot);
	WarningRadius->InitSphereRadius(Radius);
	WarningRadius->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WarningMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WarningMesh"));
	WarningMesh->SetupAttachment(SceneRoot);
	WarningMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WarningMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f));

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(SceneRoot);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 900.0f));
	ProjectileMesh->SetRelativeScale3D(FVector(0.18f, 0.18f, 8.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		WarningMesh->SetStaticMesh(CylinderMesh.Object);
		ProjectileMesh->SetStaticMesh(CylinderMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (BasicMaterial.Succeeded())
	{
		if (UMaterialInstanceDynamic* WarningMID = UMaterialInstanceDynamic::Create(BasicMaterial.Object, this))
		{
			WarningMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.0f, 0.0f, 0.5f));
			WarningMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.0f, 0.0f, 0.0f, 0.5f));
			WarningMID->SetScalarParameterValue(TEXT("Opacity"), 0.5f);
			WarningMesh->SetMaterial(0, WarningMID);
		}
		if (UMaterialInstanceDynamic* ProjectileMID = UMaterialInstanceDynamic::Create(BasicMaterial.Object, this))
		{
			ProjectileMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.16f, 0.0f, 1.0f));
			ProjectileMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.0f, 0.16f, 0.0f, 1.0f));
			ProjectileMesh->SetMaterial(0, ProjectileMID);
		}
	}

	RefreshRadiusVisuals();
}

void ABBPowerNukeStrikeActor::InitNukeStrike(UAbilitySystemComponent* InSourceASC, AActor* InInstigatorActor,
	TSubclassOf<UGameplayEffect> InDamageGE, float InRadius, float InImpactDelay, float InImpactDamage,
	float InBurnDuration, float InBurnDamagePerTick, float InBurnTickInterval)
{
	SourceASC = InSourceASC;
	InstigatorActor = InInstigatorActor;
	DamageEffectClass = InDamageGE;
	LaunchLocation = InInstigatorActor ? InInstigatorActor->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f) : GetActorLocation() + FVector(0.0f, 0.0f, 1200.0f);
	Radius = FMath::Max(1.0f, InRadius);
	ImpactDelay = FMath::Max(0.01f, InImpactDelay);
	ImpactDamage = FMath::Max(0.0f, InImpactDamage);
	BurnDuration = FMath::Max(0.0f, InBurnDuration);
	BurnDamagePerTick = FMath::Max(0.0f, InBurnDamagePerTick);
	BurnTickInterval = FMath::Max(0.05f, InBurnTickInterval);
	RefreshRadiusVisuals();
	RefreshProjectileVisual();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(ImpactTimerHandle, this, &ABBPowerNukeStrikeActor::Impact, ImpactDelay, false);
		ForceNetUpdate();
	}
}

void ABBPowerNukeStrikeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABBPowerNukeStrikeActor, Radius);
	DOREPLIFETIME(ABBPowerNukeStrikeActor, LaunchLocation);
}

void ABBPowerNukeStrikeActor::OnRep_Radius()
{
	RefreshRadiusVisuals();
}

void ABBPowerNukeStrikeActor::OnRep_LaunchLocation()
{
	RefreshProjectileVisual();
}

void ABBPowerNukeStrikeActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	LocalElapsedTime += DeltaSeconds;
	RefreshProjectileVisual();
}

void ABBPowerNukeStrikeActor::Impact()
{
	if (!HasAuthority())
	{
		return;
	}

	ApplyImpactDamage();
	ApplyPowerDestruction();
	SpawnBurnZone();
	Destroy();
}

void ABBPowerNukeStrikeActor::ApplyImpactDamage()
{
	if (!SourceASC.IsValid() || !DamageEffectClass || ImpactDamage <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	if (!World->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectParams, Shape))
	{
		return;
	}

	TSet<AActor*> HitActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Actor->IsActorBeingDestroyed() || HitActors.Contains(Actor))
		{
			continue;
		}
		HitActors.Add(Actor);

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC || TargetASC->HasMatchingGameplayTag(BBGameplayTags::State_Dead))
		{
			continue;
		}

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		if (InstigatorActor.IsValid())
		{
			Context.AddInstigator(InstigatorActor.Get(), InstigatorActor.Get());
		}

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, ImpactDamage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
}

void ABBPowerNukeStrikeActor::ApplyPowerDestruction()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	if (!World->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectParams, Shape))
	{
		return;
	}

	FBBPowerDestructionContext Context;
	Context.InstigatorActor = InstigatorActor.Get();
	Context.Origin = GetActorLocation();
	Context.Radius = Radius;
	Context.PowerID = FName(TEXT("TacticalNuke"));

	TSet<AActor*> ProcessedActors;
	TSet<UActorComponent*> ProcessedComponents;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Actor == this || Actor->IsActorBeingDestroyed() || ProcessedActors.Contains(Actor))
		{
			continue;
		}
		ProcessedActors.Add(Actor);

		if (Actor->GetClass()->ImplementsInterface(UBBPowerDestructibleInterface::StaticClass()))
		{
			IBBPowerDestructibleInterface::Execute_ApplyPowerDestruction(Actor, Context);
		}

		TArray<UActorComponent*> Components = Actor->GetComponentsByInterface(UBBPowerDestructibleInterface::StaticClass());
		for (UActorComponent* Component : Components)
		{
			if (!Component || ProcessedComponents.Contains(Component))
			{
				continue;
			}
			ProcessedComponents.Add(Component);
			IBBPowerDestructibleInterface::Execute_ApplyPowerDestruction(Component, Context);
		}
	}
}

void ABBPowerNukeStrikeActor::SpawnBurnZone()
{
	if (BurnDuration <= 0.0f || BurnDamagePerTick <= 0.0f || !DamageEffectClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = InstigatorActor.Get();
	Params.Instigator = Cast<APawn>(InstigatorActor.Get());
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (ABBNukeBurnZone* BurnZone = GetWorld()->SpawnActor<ABBNukeBurnZone>(ABBNukeBurnZone::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params))
	{
		BurnZone->InitBurnZone(SourceASC.Get(), DamageEffectClass, Radius, BurnDuration, BurnDamagePerTick, BurnTickInterval);
	}
}

void ABBPowerNukeStrikeActor::RefreshRadiusVisuals()
{
	if (WarningRadius)
	{
		WarningRadius->SetSphereRadius(Radius, true);
	}

	if (WarningMesh)
	{
		const float RadiusScale = Radius / 100.0f;
		WarningMesh->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, 0.02f));
	}
}

void ABBPowerNukeStrikeActor::RefreshProjectileVisual()
{
	if (!ProjectileMesh)
	{
		return;
	}

	const float Alpha = FMath::Clamp(LocalElapsedTime / FMath::Max(0.01f, ImpactDelay), 0.0f, 1.0f);
	const FVector CurrentPoint = GetProjectilePoint(Alpha);
	const FVector PreviousPoint = GetProjectilePoint(FMath::Max(0.0f, Alpha - 0.06f));
	const FVector Segment = CurrentPoint - PreviousPoint;
	const float SegmentLength = FMath::Max(80.0f, Segment.Size());
	const FVector SegmentDirection = Segment.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);

	ProjectileMesh->SetWorldLocation(PreviousPoint + SegmentDirection * (SegmentLength * 0.5f));
	ProjectileMesh->SetWorldRotation(FRotationMatrix::MakeFromZ(SegmentDirection).Rotator());
	ProjectileMesh->SetWorldScale3D(FVector(0.18f, 0.18f, SegmentLength / 100.0f));
}

FVector ABBPowerNukeStrikeActor::GetProjectilePoint(float Alpha) const
{
	const FVector Target = GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);
	const FVector Start = LaunchLocation.IsNearlyZero() ? Target + FVector(0.0f, 0.0f, TravelArcHeight) : FVector(LaunchLocation);
	const FVector ControlA = Start + FVector(0.0f, 0.0f, TravelArcHeight);
	const FVector ControlB = Target + FVector(0.0f, 0.0f, TravelArcHeight);
	const float InvAlpha = 1.0f - Alpha;

	return (InvAlpha * InvAlpha * InvAlpha * Start)
		+ (3.0f * InvAlpha * InvAlpha * Alpha * ControlA)
		+ (3.0f * InvAlpha * Alpha * Alpha * ControlB)
		+ (Alpha * Alpha * Alpha * Target);
}
