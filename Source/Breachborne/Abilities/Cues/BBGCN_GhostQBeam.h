#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "BBGCN_GhostQBeam.generated.h"

class UNiagaraSystem;

UCLASS(Abstract, Blueprintable)
class BREACHBORNE_API UBBGCN_GhostQBeam : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget,
		const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|Q")
	TSoftObjectPtr<UNiagaraSystem> BeamSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Breachborne|Ghost|Q", meta = (ClampMin = "0.01"))
	float Lifetime = 0.15f;
};

UCLASS(Blueprintable)
class BREACHBORNE_API UBBGCN_GhostQTelegraph : public UBBGCN_GhostQBeam
{
	GENERATED_BODY()

public:
	UBBGCN_GhostQTelegraph();
};

UCLASS(Blueprintable)
class BREACHBORNE_API UBBGCN_GhostQFire : public UBBGCN_GhostQBeam
{
	GENERATED_BODY()

public:
	UBBGCN_GhostQFire();
};
