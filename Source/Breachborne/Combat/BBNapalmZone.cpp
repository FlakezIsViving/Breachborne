#include "BBNapalmZone.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBTargetDummy.h"
#include "Breachborne/Combat/BBGlideBot.h"
#include "Breachborne/Breachborne.h"
#include "UObject/ConstructorHelpers.h"

ABBNapalmZone::ABBNapalmZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Collision sphere for overlap detection (server-side damage ticks)
	ZoneCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneCollision"));
	ZoneCollision->InitSphereRadius(ZoneRadius);
	ZoneCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneCollision->SetCollisionObjectType(ECC_WorldDynamic);
	ZoneCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ZoneCollision->SetGenerateOverlapEvents(true);
	SetRootComponent(ZoneCollision);

	// Placeholder visual — flat cylinder scaled to match radius
	ZoneVisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneVisualMesh"));
	ZoneVisualMesh->SetupAttachment(ZoneCollision);
	ZoneVisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		ZoneVisualMesh->SetStaticMesh(CylinderMesh.Object);
		// Scale cylinder: diameter matches ZoneRadius, height is thin
		// Default cylinder is 100 units radius, 100 units half-height
		const float RadiusScale = ZoneRadius / 100.0f;
		ZoneVisualMesh->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, 0.05f));
		ZoneVisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 5.0f)); // Slightly above ground
	}
}

void ABBNapalmZone::InitZone(UAbilitySystemComponent* InASC, TSubclassOf<UGameplayEffect> InDamageGE, float InDamagePerTick, int32 InTeamID)
{
	SourceASC = InASC;
	DamageEffectClass = InDamageGE;
	DamagePerTick = InDamagePerTick;
	SourceTeamID = InTeamID;

	if (HasAuthority())
	{
		// Start damage tick timer
		GetWorldTimerManager().SetTimer(
			DamageTickHandle,
			this,
			&ABBNapalmZone::OnDamageTick,
			DamageTickInterval,
			true, // looping
			0.0f  // first tick immediately
		);

		// Self-destruct after duration
		GetWorldTimerManager().SetTimer(
			DurationHandle,
			this,
			&ABBNapalmZone::OnZoneExpired,
			ZoneDuration,
			false
		);

		UE_LOG(LogBreachborne, Log, TEXT("BBNapalmZone: Initialized — radius %.0f, duration %.1fs, dmg/tick %.1f"),
			ZoneRadius, ZoneDuration, DamagePerTick);
	}
}

void ABBNapalmZone::OnDamageTick()
{
	if (!HasAuthority() || !SourceASC.IsValid() || !DamageEffectClass)
	{
		return;
	}

	// No class filter — pick up both HunterCharacter and TargetDummy
	TArray<AActor*> OverlappingActors;
	ZoneCollision->GetOverlappingActors(OverlappingActors);

	int32 DamageCount = 0;
	for (AActor* Actor : OverlappingActors)
	{
		// Resolve target ASC and team — supports HunterCharacter, TargetDummy, and GlideBot
		UAbilitySystemComponent* TargetASC = nullptr;
		int32 TargetTeamID = SourceTeamID; // default = same team → skip

		if (AHunterCharacter* HitHunter = Cast<AHunterCharacter>(Actor))
		{
			if (const ABreachbornePlayerState* TargetPS = HitHunter->GetPlayerState<ABreachbornePlayerState>())
			{
				TargetTeamID = TargetPS->GetTeamID();
			}
			TargetASC = HitHunter->GetAbilitySystemComponent();
		}
		else if (ABBTargetDummy* Dummy = Cast<ABBTargetDummy>(Actor))
		{
			TargetTeamID = Dummy->GetTeamID();
			TargetASC = Dummy->GetAbilitySystemComponent();
		}
		else if (ABBGlideBot* GlideBot = Cast<ABBGlideBot>(Actor))
		{
			TargetTeamID = GlideBot->GetTeamID();
			TargetASC = GlideBot->GetAbilitySystemComponent();
		}

		if (TargetTeamID == SourceTeamID || !TargetASC)
		{
			continue;
		}

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1, SourceASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, DamagePerTick);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			DamageCount++;

			UE_LOG(LogBreachborne, Log, TEXT("BBNapalmZone: Tick %.1f damage to %s (Team %d)"),
				DamagePerTick, *Actor->GetName(), TargetTeamID);
		}
	}

	if (DamageCount > 0)
	{
		UE_LOG(LogBreachborne, Log, TEXT("BBNapalmZone: Tick complete — %d targets damaged"), DamageCount);
	}
}

void ABBNapalmZone::OnZoneExpired()
{
	UE_LOG(LogBreachborne, Log, TEXT("BBNapalmZone: Duration expired, destroying"));
	GetWorldTimerManager().ClearTimer(DamageTickHandle);
	Destroy();
}
