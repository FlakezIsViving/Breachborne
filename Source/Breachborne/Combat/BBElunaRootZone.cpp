#include "BBElunaRootZone.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Breachborne.h"
#include "Breachborne/Combat/BBPrimitiveBurstActor.h"
#include "Breachborne/Combat/BBPrimitiveVisuals.h"
#include "Breachborne/Combat/BBStunTagEffect.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABBElunaRootZone::ABBElunaRootZone()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootSphere = CreateDefaultSubobject<USphereComponent>(TEXT("RootSphere"));
	RootSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootSphere->SetGenerateOverlapEvents(false);
	SetRootComponent(RootSphere);

	CircleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CircleMesh"));
	CircleMesh->SetupAttachment(RootSphere);
	CircleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CircleMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.02f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		CircleMesh->SetStaticMesh(CylinderMesh.Object);
	}

	BBPrimitiveVisuals::ApplyColor(CircleMesh, FLinearColor(0.43f, 0.78f, 1.0f, 1.0f));
	CircleMesh->SetVisibility(false, true);
}

void ABBElunaRootZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABBElunaRootZone, RootRadius);
	DOREPLIFETIME(ABBElunaRootZone, RootDuration);
	DOREPLIFETIME(ABBElunaRootZone, bVisualInitialized);
}

void ABBElunaRootZone::InitZone(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageGE, float InDamage, int32 InTeamID, float InRadius, float InDuration)
{
	SourceASC = InSourceASC;
	DamageEffectClass = InDamageGE;
	BaseDamage = InDamage;
	SourceTeamID = InTeamID;
	RootRadius = InRadius;
	RootDuration = InDuration;
	bVisualInitialized = true;
	bInitialized = true;

	// Enable collision for overlap queries (was NoCollision in constructor)
	RootSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RootSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootSphere->SetGenerateOverlapEvents(true);
	RootSphere->SetSphereRadius(RootRadius);

	RefreshVisualState();
	ForceNetUpdate();

	UE_LOG(LogBreachborne, Log, TEXT("Eluna RootZone: Init at %s | Radius=%.0f | Duration=%.1f"),
		*GetActorLocation().ToString(), RootRadius, RootDuration);
}

void ABBElunaRootZone::OnRep_VisualState()
{
	RefreshVisualState();
}

void ABBElunaRootZone::RefreshVisualState()
{
	bInitialized = bVisualInitialized;
	RootSphere->SetSphereRadius(RootRadius);
	CircleMesh->SetVisibility(bVisualInitialized, true);
	CircleMesh->SetRelativeScale3D(FVector(0.01f, 0.01f, 0.02f));
}

void ABBElunaRootZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bInitialized || bHasPopped)
	{
		return;
	}

	ElapsedTime += DeltaTime;

	float GrowthAlpha = FMath::Clamp(ElapsedTime / RootDuration, 0.0f, 1.0f);
	float CurrentScale = FMath::Lerp(0.01f, RootRadius / 50.0f, GrowthAlpha);
	CircleMesh->SetRelativeScale3D(FVector(CurrentScale, CurrentScale, 0.02f));

	if (ElapsedTime >= RootDuration)
	{
		CircleMesh->SetRelativeScale3D(FVector(RootRadius / 50.0f, RootRadius / 50.0f, 0.02f));
		if (HasAuthority())
		{
			ApplyRootAndDamage();
			bHasPopped = true;
			Destroy();
		}
	}
}

void ABBElunaRootZone::ApplyRootAndDamage()
{
	if (!SourceASC.IsValid())
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	RootSphere->GetOverlappingActors(OverlappingActors, AHunterCharacter::StaticClass());


	int32 HitCount = 0;
	for (AActor* Actor : OverlappingActors)
	{
		AHunterCharacter* Enemy = Cast<AHunterCharacter>(Actor);
		if (!Enemy) continue;
		if (Actor == GetOwner() || Actor == GetInstigator()) continue;

		const ABreachbornePlayerState* EnemyPS = Enemy->GetPlayerState<ABreachbornePlayerState>();
		if (!EnemyPS || EnemyPS->GetTeamID() == SourceTeamID) continue;

		UAbilitySystemComponent* TargetASC = Enemy->GetAbilitySystemComponent();
		if (TargetASC && DamageEffectClass)
		{
			FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1, SourceASC->MakeEffectContext());
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, BaseDamage);
				SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
				++HitCount;
			}
		}
		if (TargetASC)
		{
			FGameplayEffectSpecHandle RootSpec = SourceASC->MakeOutgoingSpec(
				UBBStunTagEffect::StaticClass(), 1.0f, SourceASC->MakeEffectContext());
			if (RootSpec.IsValid())
			{
				RootSpec.Data->SetDuration(RootDuration, false);
				SourceASC->ApplyGameplayEffectSpecToTarget(*RootSpec.Data.Get(), TargetASC);
			}
		}
	}

	FActorSpawnParameters BurstParams;
	BurstParams.Owner = GetOwner();
	BurstParams.Instigator = GetInstigator();
	BurstParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ABBPrimitiveBurstActor* Burst = GetWorld()->SpawnActor<ABBPrimitiveBurstActor>(
		ABBPrimitiveBurstActor::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, BurstParams))
	{
		Burst->InitBurst(GetActorLocation(), RootRadius, 0.28f, FLinearColor(0.86f, 0.92f, 1.0f, 1.0f));
	}

}
