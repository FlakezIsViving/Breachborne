#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbyssKillVolume.generated.h"

class UBoxComponent;

/**
 * Kill volume placed below the floating island map.
 * Any character overlapping this volume is instantly killed (no wisp state).
 * Abyss death = instant deathbox, per Supervive design.
 * Deathbox spawning is stubbed until Phase 6.
 */
UCLASS()
class BREACHBORNE_API AAbyssKillVolume : public AActor
{
	GENERATED_BODY()

public:
	AAbyssKillVolume();

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Breachborne|World")
	TObjectPtr<UBoxComponent> KillVolume;
};
