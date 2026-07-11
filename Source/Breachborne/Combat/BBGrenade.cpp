#include "BBGrenade.h"
#include "Components/SphereComponent.h"
#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "Breachborne/Characters/HunterCharacter.h"
#include "Breachborne/Core/BreachbornePlayerState.h"
#include "Breachborne/Abilities/BBGameplayTags.h"
#include "Breachborne/Combat/BBTargetDummy.h"
#include "Breachborne/Combat/BBGlideBot.h"
#include "Breachborne/Breachborne.h"
#include "DrawDebugHelpers.h"

ABBGrenade::ABBGrenade()
{
	ProjectileSpeed = 2500.0f;
	ProjectileLifetime = 3.0f; // Longer than fuse — fuse handles detonation
}

void ABBGrenade::BeginPlay()
{
	Super::BeginPlay();

	// Start fuse timer on server — auto-detonates if no hit occurs
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			FuseTimerHandle,
			this,
			&ABBGrenade::OnFuseExpired,
			FuseTime,
			false
		);
	}
}

void ABBGrenade::OnProjectileOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bHasExploded)
	{
		return;
	}

	// Skip self/instigator
	if (OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	// Check if this is an enemy target (HunterCharacter, TargetDummy, or GlideBot)
	int32 TargetTeamID = SourceTeamID; // default = same team → don't explode

	if (AHunterCharacter* HitHunter = Cast<AHunterCharacter>(OtherActor))
	{
		if (const ABreachbornePlayerState* TargetPS = HitHunter->GetPlayerState<ABreachbornePlayerState>())
		{
			TargetTeamID = TargetPS->GetTeamID();
		}
	}
	else if (ABBTargetDummy* Dummy = Cast<ABBTargetDummy>(OtherActor))
	{
		TargetTeamID = Dummy->GetTeamID();
	}
	else if (ABBGlideBot* GlideBot = Cast<ABBGlideBot>(OtherActor))
	{
		TargetTeamID = GlideBot->GetTeamID();
	}

	if (TargetTeamID == SourceTeamID)
	{
		return;
	}

	UE_LOG(LogBreachborne, Log, TEXT("BBGrenade: Contact with enemy %s (Team %d) — exploding!"),
		*OtherActor->GetName(), TargetTeamID);
	Explode();
}

void ABBGrenade::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || bHasExploded)
	{
		return;
	}

	// Wall hit — explode at wall
	Explode();
}

void ABBGrenade::OnLifetimeExpired()
{
	// Lifetime backup — if fuse didn't trigger, explode now
	Explode();
}

void ABBGrenade::Explode()
{
	if (bHasExploded || !HasAuthority())
	{
		return;
	}
	bHasExploded = true;

	const FVector ExplosionCenter = GetActorLocation();

	UE_LOG(LogBreachborne, Log, TEXT("BBGrenade: Exploding at %s, radius %.0f"), *ExplosionCenter.ToString(), ExplosionRadius);

	DrawDebugSphere(GetWorld(), ExplosionCenter, ExplosionRadius, 16, FColor::Orange, false, 1.5f);

	// Sphere overlap to find all pawns in explosion radius
	TArray<FOverlapResult> Overlaps;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(ExplosionRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		ExplosionCenter,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		SphereShape,
		QueryParams
	);

	if (!SourceASC.IsValid() || !DamageEffectClass)
	{
		UE_LOG(LogBreachborne, Warning, TEXT("BBGrenade: No source ASC or DamageEffectClass for explosion"));
		Destroy();
		return;
	}

	int32 DamageCount = 0;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor)
		{
			continue;
		}

		// Resolve target ASC and team — supports HunterCharacter, TargetDummy, and GlideBot
		UAbilitySystemComponent* TargetASC = nullptr;
		int32 TargetTeamID = SourceTeamID; // default = same team → skip

		if (AHunterCharacter* HitHunter = Cast<AHunterCharacter>(HitActor))
		{
			if (const ABreachbornePlayerState* TargetPS = HitHunter->GetPlayerState<ABreachbornePlayerState>())
			{
				TargetTeamID = TargetPS->GetTeamID();
			}
			TargetASC = HitHunter->GetAbilitySystemComponent();
		}
		else if (ABBTargetDummy* Dummy = Cast<ABBTargetDummy>(HitActor))
		{
			TargetTeamID = Dummy->GetTeamID();
			TargetASC = Dummy->GetAbilitySystemComponent();
		}
		else if (ABBGlideBot* GlideBot = Cast<ABBGlideBot>(HitActor))
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
			SpecHandle.Data->SetSetByCallerMagnitude(BBGameplayTags::SetByCaller_Damage, BaseDamage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			DamageCount++;

			UE_LOG(LogBreachborne, Log, TEXT("BBGrenade: Applied %.1f explosion damage to %s (Team %d)"),
				BaseDamage, *HitActor->GetName(), TargetTeamID);
		}
	}

	UE_LOG(LogBreachborne, Log, TEXT("BBGrenade: Explosion complete — %d overlaps, %d targets damaged"), Overlaps.Num(), DamageCount);

	GetWorldTimerManager().ClearTimer(FuseTimerHandle);
	Destroy();
}

void ABBGrenade::OnFuseExpired()
{
	UE_LOG(LogBreachborne, Log, TEXT("BBGrenade: Fuse expired, detonating"));
	Explode();
}
