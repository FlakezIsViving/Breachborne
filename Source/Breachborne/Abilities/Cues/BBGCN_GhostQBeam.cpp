#include "BBGCN_GhostQBeam.h"

#include "Breachborne/Breachborne.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

namespace
{
	const FName BeamStartParameter(TEXT("User.BeamStart"));
	const FName BeamEndParameter(TEXT("User.BeamEnd"));
	const FName DirectionParameter(TEXT("User.Direction"));
	const FName LengthParameter(TEXT("User.Length"));
	const FName LifetimeParameter(TEXT("User.Lifetime"));
}

bool UBBGCN_GhostQBeam::OnExecute_Implementation(AActor* MyTarget,
	const FGameplayCueParameters& Parameters) const
{
	UWorld* World = MyTarget ? MyTarget->GetWorld() : nullptr;
	UNiagaraSystem* System = BeamSystem.LoadSynchronous();
	if (!World || !System || Parameters.RawMagnitude <= UE_KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogBreachborne, Verbose,
			TEXT("BB_GHOST_Q_VFX|CueSkipped target=%s system=%s distance=%.1f"),
			*GetNameSafe(MyTarget), *BeamSystem.ToSoftObjectPath().ToString(), Parameters.RawMagnitude);
		return false;
	}

	const FVector StartLocation = FVector(Parameters.Location);
	const FVector AimDirection = FVector(Parameters.Normal).GetSafeNormal();
	const FVector EndLocation = StartLocation + AimDirection * Parameters.RawMagnitude;
	UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World, System, StartLocation, AimDirection.Rotation(), FVector::OneVector,
		true, false, ENCPoolMethod::None, false);
	if (!Component)
	{
		return false;
	}

	Component->SetVariableVec3(BeamStartParameter, StartLocation);
	Component->SetVariableVec3(BeamEndParameter, EndLocation);
	Component->SetVariableVec3(DirectionParameter, AimDirection);
	Component->SetVariableFloat(LengthParameter, Parameters.RawMagnitude);
	Component->SetVariableFloat(LifetimeParameter, Lifetime);
	Component->Activate(true);

	FTimerHandle DeactivateHandle;
	World->GetTimerManager().SetTimer(DeactivateHandle,
		[WeakComponent = TWeakObjectPtr<UNiagaraComponent>(Component)]()
		{
			if (UNiagaraComponent* ActiveComponent = WeakComponent.Get())
			{
				ActiveComponent->DeactivateImmediate();
			}
		},
		Lifetime, false);
	return true;
}

UBBGCN_GhostQTelegraph::UBBGCN_GhostQTelegraph()
{
	BeamSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(
		TEXT("/Game/GameplayCues/Hunters/Ghost/FX/NS_GhostRailGunTelegraph.NS_GhostRailGunTelegraph")));
	Lifetime = 0.3f;
}

UBBGCN_GhostQFire::UBBGCN_GhostQFire()
{
	BeamSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(
		TEXT("/Game/GameplayCues/Hunters/Ghost/FX/NS_GhostRailGunLaser.NS_GhostRailGunLaser")));
	Lifetime = 0.15f;
}
