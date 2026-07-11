#include "BBMoonlightZone.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBAbilitySystemComponent.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBHealEffect.h"
#include "Breachborne/Combat/BBTestAlly.h"
#include "Breachborne/Breachborne.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/OverlapResult.h"

ABBMoonlightZone::ABBMoonlightZone()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	HealSphere = CreateDefaultSubobject<USphereComponent>(TEXT("HealSphere"));
	RootComponent = HealSphere;
	HealSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	HealSphere->SetGenerateOverlapEvents(true);

	ZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneMesh"));
	ZoneMesh->SetupAttachment(HealSphere);
	ZoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GroundRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundRing"));
	GroundRing->SetupAttachment(HealSphere);
	GroundRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GroundRing->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.02f));
	GroundRing->SetRelativeLocation(FVector(0.0f, 0.0f, -50.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		GroundRing->SetStaticMesh(CylinderMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterial> BasicMat(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (BasicMat.Succeeded())
	{
		UMaterialInstanceDynamic* WhiteMat = UMaterialInstanceDynamic::Create(BasicMat.Object, this);
		if (WhiteMat)
		{
			WhiteMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(2.0f, 2.0f, 2.5f));
			GroundRing->SetMaterial(0, WhiteMat);
		}
	}

	HealEffectClass = UBBHealEffect::StaticClass();
}

AHunterCharacter* ABBMoonlightZone::GetAttachedAlly() const
{
	return Cast<AHunterCharacter>(AttachedAlly.Get());
}

void ABBMoonlightZone::InitZone(AHunterCharacter* InSourceHunter, float InHealPerTick, float InTickInterval, float InHealRadius, float InDuration, float InSelfHealFraction, float InWispHealMultiplier, float InBurstHeal)
{
	SourceHunter = InSourceHunter;
	HealPerTick = InHealPerTick;
	TickInterval = InTickInterval;
	HealRadius = InHealRadius;
	ZoneDuration = InDuration;
	SelfHealFraction = InSelfHealFraction;
	WispHealMultiplier = InWispHealMultiplier;
	BurstHeal = InBurstHeal;
	bInitialized = true;

	HealSphere->SetSphereRadius(HealRadius);
}

void ABBMoonlightZone::BeginPlay()
{
	Super::BeginPlay();
}

void ABBMoonlightZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bInitialized)
	{
		return;
	}

	// Server-only: travel and follow attached ally
	if (HasAuthority())
	{
		if (AttachedAlly.IsValid())
		{
			SetActorLocation(AttachedAlly->GetActorLocation());
		}
		else if (bIsTraveling)
		{
			UpdateTravel(DeltaTime);
		}
	}

	ElapsedTime += DeltaTime;
	TickAccumulator += DeltaTime;

	if (TickAccumulator >= TickInterval)
	{
		TickHeal();
		TickAccumulator = 0.0f;
	}

	if (ElapsedTime >= ZoneDuration)
	{
		ApplyBurstHeal();
		DestroyZone();
	}
}

void ABBMoonlightZone::TickHeal()
{
	if (!SourceHunter.IsValid())
	{
		return;
	}

	ABreachbornePlayerState* SourcePS = SourceHunter->GetPlayerState<ABreachbornePlayerState>();
	if (!SourcePS)
	{
		return;
	}

	UBBAbilitySystemComponent* SourceASC = Cast<UBBAbilitySystemComponent>(SourceHunter->GetAbilitySystemComponent());
	if (!SourceASC)
	{
		return;
	}

	const FVector ZoneLoc = GetActorLocation();
	const int32 SourceTeam = SourcePS->GetTeamID();

	TArray<AActor*> OverlappingActors;
	HealSphere->GetOverlappingActors(OverlappingActors, AHunterCharacter::StaticClass());

	int32 HealCount = 0;
	for (AActor* Actor : OverlappingActors)
	{
		AHunterCharacter* Ally = Cast<AHunterCharacter>(Actor);
		if (!Ally)
		{
			continue;
		}

		ABreachbornePlayerState* AllyPS = Ally->GetPlayerState<ABreachbornePlayerState>();
		if (!AllyPS || AllyPS->GetTeamID() != SourceTeam)
		{
			continue;
		}

		float FinalHeal = HealPerTick;

		// Self heal is reduced
		if (Ally == SourceHunter.Get())
		{
			FinalHeal *= SelfHealFraction;
		}

		// Wisp heal multiplier (500% in Supervive)
		UBBAbilitySystemComponent* AllyASC = Cast<UBBAbilitySystemComponent>(Ally->GetAbilitySystemComponent());
		if (AllyASC && AllyASC->HasMatchingGameplayTag(BBGameplayTags::State_Wisp))
		{
			FinalHeal *= WispHealMultiplier;
		}

		// Apply heal effect
		if (HealEffectClass && AllyASC)
		{
			FGameplayEffectSpecHandle SpecHandle = AllyASC->MakeOutgoingSpec(HealEffectClass, 1.0f, AllyASC->MakeEffectContext());
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_HealAmount.GetTag(), FinalHeal);
				AllyASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				++HealCount;
			}
		}
	}

	// Also heal BBTestAlly actors (placed test allies)
	TArray<AActor*> OverlappingTestAllies;
	HealSphere->GetOverlappingActors(OverlappingTestAllies, ABBTestAlly::StaticClass());
	for (AActor* Actor : OverlappingTestAllies)
	{
		ABBTestAlly* TestAlly = Cast<ABBTestAlly>(Actor);
		if (!TestAlly || TestAlly->GetTeamID() != SourceTeam)
		{
			continue;
		}

		UBBAbilitySystemComponent* AllyASC = Cast<UBBAbilitySystemComponent>(TestAlly->GetAbilitySystemComponent());
		if (HealEffectClass && AllyASC)
		{
			FGameplayEffectSpecHandle SpecHandle = AllyASC->MakeOutgoingSpec(HealEffectClass, 1.0f, AllyASC->MakeEffectContext());
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_HealAmount.GetTag(), HealPerTick);
				AllyASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				++HealCount;
			}
		}
	}

	(void)HealCount; // suppress unused warning if no heals applied
}

void ABBMoonlightZone::ApplyBurstHeal()
{
	if (!SourceHunter.IsValid())
	{
		return;
	}

	ABreachbornePlayerState* SourcePS = SourceHunter->GetPlayerState<ABreachbornePlayerState>();
	if (!SourcePS)
	{
		return;
	}

	const FVector ZoneLoc = GetActorLocation();
	const int32 SourceTeam = SourcePS->GetTeamID();

	TArray<AActor*> OverlappingActors;
	HealSphere->GetOverlappingActors(OverlappingActors, AHunterCharacter::StaticClass());

	int32 HealCount = 0;
	for (AActor* Actor : OverlappingActors)
	{
		AHunterCharacter* Ally = Cast<AHunterCharacter>(Actor);
		if (!Ally)
		{
			continue;
		}

		ABreachbornePlayerState* AllyPS = Ally->GetPlayerState<ABreachbornePlayerState>();
		if (!AllyPS || AllyPS->GetTeamID() != SourceTeam)
		{
			continue;
		}

		float FinalBurst = BurstHeal;

		if (Ally == SourceHunter.Get())
		{
			FinalBurst *= SelfHealFraction;
		}

		UBBAbilitySystemComponent* AllyASC = Cast<UBBAbilitySystemComponent>(Ally->GetAbilitySystemComponent());
		if (AllyASC && AllyASC->HasMatchingGameplayTag(BBGameplayTags::State_Wisp))
		{
			FinalBurst *= WispHealMultiplier;
		}

		if (HealEffectClass && AllyASC)
		{
			FGameplayEffectSpecHandle SpecHandle = AllyASC->MakeOutgoingSpec(HealEffectClass, 1.0f, AllyASC->MakeEffectContext());
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_HealAmount.GetTag(), FinalBurst);
				AllyASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				++HealCount;
			}
		}
	}

	// Also burst-heal BBTestAlly actors
	TArray<AActor*> OverlappingTestAllies;
	HealSphere->GetOverlappingActors(OverlappingTestAllies, ABBTestAlly::StaticClass());
	for (AActor* Actor : OverlappingTestAllies)
	{
		ABBTestAlly* TestAlly = Cast<ABBTestAlly>(Actor);
		if (!TestAlly || TestAlly->GetTeamID() != SourceTeam)
		{
			continue;
		}

		UBBAbilitySystemComponent* AllyASC = Cast<UBBAbilitySystemComponent>(TestAlly->GetAbilitySystemComponent());
		if (HealEffectClass && AllyASC)
		{
			FGameplayEffectSpecHandle SpecHandle = AllyASC->MakeOutgoingSpec(HealEffectClass, 1.0f, AllyASC->MakeEffectContext());
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_HealAmount.GetTag(), BurstHeal);
				AllyASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				++HealCount;
			}
		}
	}

}

void ABBMoonlightZone::DestroyZone()
{
	Destroy();
}

void ABBMoonlightZone::TossToLocation(const FVector& TargetLocation)
{
	if (bTossed)
	{
		return;
	}

	ThrowStartLocation = GetActorLocation();
	ThrowTargetLocation = TargetLocation;
	bTossed = true;
	bIsTraveling = true;

	// Boost healing by 50% when tossed
	HealPerTick *= 1.5f;
	BurstHeal *= 1.5f;

	UE_LOG(LogBreachborne, Warning, TEXT("[Q_TOSS] Zone::TossToLocation | Start=%s | Target=%s | Speed=%.0f | HasAuthority=%s"),
		*ThrowStartLocation.ToCompactString(), *ThrowTargetLocation.ToCompactString(), TossSpeed,
		HasAuthority() ? TEXT("YES") : TEXT("NO"));
}

void ABBMoonlightZone::UpdateTravel(float DeltaTime)
{
	const FVector CurrentLoc = GetActorLocation();
	const FVector ToTarget = ThrowTargetLocation - CurrentLoc;
	const float DistToTarget = ToTarget.Size2D();
	const float MoveDist = TossSpeed * DeltaTime;

	if (DistToTarget <= MoveDist)
	{
		// Reached max distance — land here
		SetActorLocation(ThrowTargetLocation);
		bIsTraveling = false;
		// One last attach attempt at landing point
		TryAttachToAlly();
		UE_LOG(LogBreachborne, Warning, TEXT("[Q_TOSS] Travel LANDED at max distance %s"), *ThrowTargetLocation.ToCompactString());
		return;
	}

	// Travel horizontally — zone is a ground effect, keep at target Z
	FVector ToTargetHorizontal = ToTarget;
	ToTargetHorizontal.Z = 0.0f;
	FVector NewLoc = CurrentLoc + ToTargetHorizontal.GetSafeNormal() * MoveDist;
	NewLoc.Z = ThrowTargetLocation.Z;

	// Sweep along the travel path to catch allies we might pass through between ticks
	if (TryAttachAlongPath(CurrentLoc, NewLoc))
	{
		return; // Attached — travel stopped inside TryAttachAlongPath
	}

	SetActorLocation(NewLoc);
}

void ABBMoonlightZone::TryAttachToAlly()
{
	if (!SourceHunter.IsValid())
	{
		return;
	}

	ABreachbornePlayerState* SourcePS = SourceHunter->GetPlayerState<ABreachbornePlayerState>();
	if (!SourcePS)
	{
		return;
	}

	const int32 SourceTeam = SourcePS->GetTeamID();
	AActor* Caster = SourceHunter.Get();

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(AttachSearchRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Sphere, QueryParams))
	{
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Actor = Overlap.GetActor();
			if (!Actor || Actor == Caster)
			{
				continue;
			}

			// Accept HunterCharacter or BBTestAlly as long as they're on the same team
			int32 ActorTeam = -1;
			if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(Actor))
			{
				if (ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>())
				{
					ActorTeam = PS->GetTeamID();
				}
			}
			else if (ABBTestAlly* TestAlly = Cast<ABBTestAlly>(Actor))
			{
				ActorTeam = TestAlly->GetTeamID();
			}

			if (ActorTeam == SourceTeam)
			{
				AttachedAlly = Actor;
				bIsTraveling = false;
				UE_LOG(LogBreachborne, Warning, TEXT("[Q_TOSS] Zone ATTACHED to %s at %s"),
					*Actor->GetName(), *GetActorLocation().ToCompactString());
				break;
			}
		}
	}
}

bool ABBMoonlightZone::TryAttachAlongPath(const FVector& From, const FVector& To)
{
	if (!SourceHunter.IsValid())
	{
		return false;
	}

	ABreachbornePlayerState* SourcePS = SourceHunter->GetPlayerState<ABreachbornePlayerState>();
	if (!SourcePS)
	{
		return false;
	}

	const int32 SourceTeam = SourcePS->GetTeamID();
	AActor* Caster = SourceHunter.Get();
	const FVector PathDir = To - From;
	const float PathLen = PathDir.Size();
	if (PathLen < KINDA_SMALL_NUMBER)
	{
		return false;
	}

	FCollisionShape Sphere = FCollisionShape::MakeSphere(AttachSearchRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> Hits;
	const bool bHit = GetWorld()->SweepMultiByChannel(Hits, From, To, FQuat::Identity, ECC_Pawn, Sphere, QueryParams);
	if (!bHit)
	{
		return false;
	}

	// Collect valid allies and sort by distance along path (closest first)
	struct FAlleyEntry
	{
		float DistAlongPath;
		AActor* Actor;
	};
	TArray<FAlleyEntry> Candidates;

	for (const FHitResult& Hit : Hits)
	{
		AActor* Actor = Hit.GetActor();
		if (!Actor || Actor == Caster)
		{
			continue;
		}

		int32 ActorTeam = -1;
		if (AHunterCharacter* Hunter = Cast<AHunterCharacter>(Actor))
		{
			if (ABreachbornePlayerState* PS = Hunter->GetPlayerState<ABreachbornePlayerState>())
			{
				ActorTeam = PS->GetTeamID();
			}
		}
		else if (ABBTestAlly* TestAlly = Cast<ABBTestAlly>(Actor))
		{
			ActorTeam = TestAlly->GetTeamID();
		}

		if (ActorTeam != SourceTeam)
		{
			continue;
		}

		float DistAlongPath = FVector::DotProduct(Hit.Location - From, PathDir.GetSafeNormal());
		DistAlongPath = FMath::Clamp(DistAlongPath, 0.0f, PathLen);
		Candidates.Add({DistAlongPath, Actor});
	}

	if (Candidates.Num() == 0)
	{
		return false;
	}

	Candidates.Sort([](const FAlleyEntry& A, const FAlleyEntry& B)
	{
		return A.DistAlongPath < B.DistAlongPath;
	});

	AActor* BestAlly = Candidates[0].Actor;
	const FVector AttachLoc = From + PathDir.GetSafeNormal() * Candidates[0].DistAlongPath;

	SetActorLocation(AttachLoc);
	AttachedAlly = BestAlly;
	bIsTraveling = false;

	UE_LOG(LogBreachborne, Warning, TEXT("[Q_TOSS] Zone ATTACHED along path to %s at %s (dist=%.1f)"),
		*BestAlly->GetName(), *AttachLoc.ToCompactString(), Candidates[0].DistAlongPath);

	return true;
}
