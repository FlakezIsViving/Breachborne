#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "Breachborne/World/BBPowerDestructibleInterface.h"
#include "BBBrushVolume.generated.h"

/**
 * Brush volume — a placeable zone that hides hunters inside it from enemy vision cones.
 *
 * Mechanism:
 * - On overlap begin (server): grants State_InBrush tag to the overlapping hunter's ASC
 * - On overlap end (server): removes the tag
 * - Enemy hunters check State_InBrush on a target before rendering their vision cone HUD marker
 * - UBBVisionConeComponent reduces outward range when the owner has State_InBrush
 *
 * Note: Server-side replication of the State_InBrush tag (via loose gameplay tag on ASC)
 * ensures all clients know who is hidden. Visual brush rendering is Blueprint-side.
 */
UCLASS()
class BREACHBORNE_API ABBBrushVolume : public AVolume, public IBBPowerDestructibleInterface
{
	GENERATED_BODY()

public:
	ABBBrushVolume();

	virtual void ApplyPowerDestruction_Implementation(const FBBPowerDestructionContext& Context) override;

protected:
	virtual void BeginPlay() override;

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

private:
	void SetBrushTag(AActor* Actor, bool bInBrush);

	bool bDestroyed = false;
};
