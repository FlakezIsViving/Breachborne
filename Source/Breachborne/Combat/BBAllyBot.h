#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BBAllyBot.generated.h"

/**
 * Placeable allied bot for testing wisp proximity revive.
 * Stands still, counts as an alive ally for wisp rez detection.
 * Green mesh so it's easy to spot. Set TeamID to match the player you want to test with.
 *
 * Drop into the level near where combat happens. When a same-team player becomes a wisp
 * and the wisp is near this bot, the rez bar will fill.
 */
UCLASS()
class BREACHBORNE_API ABBAllyBot : public ACharacter
{
	GENERATED_BODY()

public:
	ABBAllyBot();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Breachborne|AllyBot")
	int32 GetTeamID() const { return TeamID; }

	UFUNCTION(BlueprintPure, Category = "Breachborne|AllyBot")
	bool GetIsAlive() const { return true; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|AllyBot")
	TObjectPtr<UStaticMeshComponent> BotMesh;

	/** Flat disc showing the rez proximity range */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breachborne|AllyBot")
	TObjectPtr<UStaticMeshComponent> RangeCircleMesh;

	/** Team ID — set this to match the player's team (first player is Team 0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|AllyBot")
	int32 TeamID = 0;

	/** Rez proximity radius to visualize (should match BBWispPawn::ProximityRadius) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breachborne|AllyBot")
	float RezRange = 400.0f;
};
