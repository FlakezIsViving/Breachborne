#pragma once

#include "Commandlets/Commandlet.h"
#include "BBNiagaraMasterAuditCommandlet.generated.h"

UCLASS()
class BREACHBORNE_API UBBNiagaraMasterAuditCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UBBNiagaraMasterAuditCommandlet();
	virtual int32 Main(const FString& Params) override;
};
