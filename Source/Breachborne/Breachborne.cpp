// Copyright Epic Games, Inc. All Rights Reserved.

#include "Breachborne.h"
#include "Modules/ModuleManager.h"
#include "Misc/EngineVersion.h"

DEFINE_LOG_CATEGORY(LogBreachborne);

class FBreachborneModule final : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

#if UE_BUILD_SHIPPING
		const TCHAR* BuildConfig = TEXT("Shipping");
#elif UE_BUILD_TEST
		const TCHAR* BuildConfig = TEXT("Test");
#elif UE_BUILD_DEBUG
		const TCHAR* BuildConfig = TEXT("Debug");
#else
		const TCHAR* BuildConfig = TEXT("Development");
#endif

		UE_LOG(LogBreachborne, Display, TEXT("BB_BUILD|Version project=Breachborne config=%s engine=%s"),
			BuildConfig,
			*FEngineVersion::Current().ToString());
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FBreachborneModule, Breachborne, "Breachborne");
