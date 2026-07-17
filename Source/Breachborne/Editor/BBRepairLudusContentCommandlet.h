#pragma once

#include "Commandlets/Commandlet.h"
#include "BBRepairLudusContentCommandlet.generated.h"

UCLASS()
class BREACHBORNE_API UBBRepairLudusContentCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UBBRepairLudusContentCommandlet();
	virtual int32 Main(const FString& Params) override;
};
