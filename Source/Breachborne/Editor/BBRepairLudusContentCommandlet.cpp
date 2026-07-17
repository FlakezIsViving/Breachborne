#include "BBRepairLudusContentCommandlet.h"

#include "Animation/Skeleton.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
	const TCHAR* HudsonSkeletonPath =
		TEXT("/Game/Hunters/Hudson/Hudson_Idle/SkeletalMeshes/Hudson_Idle_Skeleton.Hudson_Idle_Skeleton");
}

UBBRepairLudusContentCommandlet::UBBRepairLudusContentCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UBBRepairLudusContentCommandlet::Main(const FString& Params)
{
	(void)Params;

	USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, HudsonSkeletonPath);
	if (!Skeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("BB_LUDUS_REPAIR|HUDSON_SKELETON|LOAD_FAILED path=%s"), HudsonSkeletonPath);
		return 1;
	}

	Skeleton->Modify();
	const int32 RemovedCount = Skeleton->Sockets.RemoveAll(
		[](const TObjectPtr<USkeletalMeshSocket>& Socket)
		{
			return Socket == nullptr;
		});
	if (RemovedCount == 0)
	{
		UE_LOG(LogTemp, Display, TEXT("BB_LUDUS_REPAIR|HUDSON_NULL_SOCKETS|REMOVED=0"));
		return 0;
	}

	UPackage* Package = Skeleton->GetOutermost();
	Package->MarkPackageDirty();

	FString PackageFilename;
	if (!FPackageName::TryConvertLongPackageNameToFilename(
		Package->GetName(), PackageFilename, FPackageName::GetAssetPackageExtension()))
	{
		UE_LOG(LogTemp, Error, TEXT("BB_LUDUS_REPAIR|HUDSON_SKELETON|INVALID_PACKAGE path=%s"),
			*Package->GetName());
		return 1;
	}

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, Skeleton, *PackageFilename, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("BB_LUDUS_REPAIR|HUDSON_SKELETON|SAVE_FAILED file=%s"),
			*PackageFilename);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("BB_LUDUS_REPAIR|HUDSON_NULL_SOCKETS|REMOVED=%d file=%s"),
		RemovedCount, *PackageFilename);
	return 0;
}
