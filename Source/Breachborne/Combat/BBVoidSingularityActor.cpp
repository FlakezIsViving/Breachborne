#include "BBVoidSingularityActor.h"

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
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	bool ResolveSingularityTarget(AActor* Actor, int32 SourceTeamID, UAbilitySystemComponent*& OutASC)
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

ABBVoidSingularityActor::ABBVoidSingularityActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	PullSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PullSphere"));
	PullSphere->InitSphereRadius(Radius);
	PullSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PullSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PullSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(PullSphere);

	VortexMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VortexMesh"));
	VortexMesh->SetupAttachment(PullSphere);
	VortexMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VortexMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 12.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		VortexMesh->SetStaticMesh(CylinderMesh.Object);
	}
	BBPrimitiveVisuals::ApplyColor(VortexMesh, FLinearColor(0.35f, 0.0f, 0.75f, 1.0f));
	RefreshVisuals();
}

void ABBVoidSingularityActor::InitSingularity(UAbilitySystemComponent* InSourceASC, AActor* InInstigatorActor, TSubclassOf<UGameplayEffect> InStunGE,
	int32 InTeamID, float InRadius, float InWarningDelay, float InActiveDuration, bool bEmpowered)
{
	SourceASC = InSourceASC;
	InstigatorActor = InInstigatorActor;
	StunEffectClass = InStunGE;
	SourceTeamID = InTeamID;
	Radius = bEmpowered ? InRadius * 1.28f : InRadius;
	WarningDelay = InWarningDelay;
	ActiveDuration = InActiveDuration;
	RefreshVisuals();
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(ActivateHandle, this, &ABBVoidSingularityActor::ActivateVortex, WarningDelay, false);
		GetWorldTimerManager().SetTimer(FinishHandle, this, &ABBVoidSingularityActor::FinishVortex, WarningDelay + ActiveDuration, false);
		ForceNetUpdate();
	}
}

void ABBVoidSingularityActor::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(HasAuthority());
}

#if WITH_AUTOMATION_TESTS
bool ABBVoidSingularityActor::AreLifecycleTimersScheduledForAutomation() const
{
	return GetWorldTimerManager().IsTimerActive(ActivateHandle)
		&& GetWorldTimerManager().IsTimerActive(FinishHandle);
}

void ABBVoidSingularityActor::ActivateForAutomation()
{
	ActivateVortex();
}

void ABBVoidSingularityActor::FinishForAutomation()
{
	FinishVortex();
}
#endif

void ABBVoidSingularityActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && bActive)
	{
		PullTick(DeltaSeconds);
	}
}

void ABBVoidSingularityActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBVoidSingularityActor, Radius);
	DOREPLIFETIME(ABBVoidSingularityActor, bActive);
}

void ABBVoidSingularityActor::OnRep_Visuals()
{
	RefreshVisuals();
}

void ABBVoidSingularityActor::ActivateVortex()
{
	bActive = true;
	ForceNetUpdate();
	RefreshVisuals();
}

void ABBVoidSingularityActor::FinishVortex()
{
	if (HasAuthority())
	{
		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = GetInstigator();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ABBPrimitiveBurstActor* Implosion = GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
			ABBPrimitiveBurstActor::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params))
		{
			Implosion->InitBurst(GetActorLocation(), Radius * 0.72f, 0.3f,
				FLinearColor(1.0f, 0.31f, 0.85f, 1.0f));
		}
	}
	Destroy();
}

void ABBVoidSingularityActor::PullTick(float DeltaSeconds)
{
	TArray<AActor*> Actors;
	PullSphere->GetOverlappingActors(Actors);
	for (AActor* Actor : Actors)
	{
		if (!Actor || Actor == InstigatorActor.Get())
		{
			continue;
		}
		UAbilitySystemComponent* TargetASC = nullptr;
		if (!ResolveSingularityTarget(Actor, SourceTeamID, TargetASC))
		{
			continue;
		}
		if (!StunnedActors.Contains(Actor))
		{
			StunnedActors.Add(Actor);
			ApplyStun(TargetASC);
		}
		const FVector ToCenter = (GetActorLocation() - Actor->GetActorLocation()).GetSafeNormal2D();
		const float PullStep = PullSpeed * DeltaSeconds;
		const FVector NewLocation = Actor->GetActorLocation() + ToCenter * PullStep;
		if (ACharacter* Character = Cast<ACharacter>(Actor))
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->Velocity = ToCenter * PullSpeed;
				MoveComp->ForceReplicationUpdate();
			}
		}
		Actor->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
		Actor->ForceNetUpdate();
	}
}

void ABBVoidSingularityActor::RefreshVisuals()
{
	if (PullSphere)
	{
		PullSphere->SetSphereRadius(Radius, true);
	}
	if (VortexMesh)
	{
		const float RadiusScale = Radius / 50.0f;
		VortexMesh->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, bActive ? 0.08f : 0.03f));
		BBPrimitiveVisuals::ApplyColor(VortexMesh, bActive
			? FLinearColor(0.95f, 0.1f, 1.0f, 1.0f)
			: FLinearColor(0.35f, 0.0f, 0.75f, 1.0f));
	}
}

void ABBVoidSingularityActor::ApplyStun(UAbilitySystemComponent* TargetASC) const
{
	if (!SourceASC.IsValid() || !TargetASC || !StunEffectClass)
	{
		return;
	}
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(StunEffectClass, 1.0f, SourceASC->MakeEffectContext());
	if (Spec.IsValid())
	{
		Spec.Data->SetDuration(StunDuration, false);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}
