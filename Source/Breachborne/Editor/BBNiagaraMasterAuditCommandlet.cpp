#include "BBNiagaraMasterAuditCommandlet.h"

#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraLightRendererProperties.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"

namespace
{
	enum class EBBNiagaraParameterType : uint8
	{
		Color,
		Float,
		Vector,
		Bool
	};

	struct FBBRequiredNiagaraParameter
	{
		const TCHAR* Name;
		EBBNiagaraParameterType Type;
	};

	struct FBBNiagaraMasterSpec
	{
		const TCHAR* Name;
		TArray<FBBRequiredNiagaraParameter> RequiredParameters;
	};

	const TArray<FBBNiagaraMasterSpec>& GetMasterSpecs()
	{
		static const TArray<FBBNiagaraMasterSpec> Specs = {
			{ TEXT("NS_BB_ProjectileBody"), {
				{ TEXT("User.PrimaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.SecondaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.TeamRelationColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.Width"), EBBNiagaraParameterType::Float },
				{ TEXT("User.Direction"), EBBNiagaraParameterType::Vector },
				{ TEXT("User.Intensity"), EBBNiagaraParameterType::Float },
				{ TEXT("User.bEmpowered"), EBBNiagaraParameterType::Bool }
			} },
			{ TEXT("NS_BB_ProjectileTrail"), {
				{ TEXT("User.PrimaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.SecondaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.Length"), EBBNiagaraParameterType::Float },
				{ TEXT("User.Width"), EBBNiagaraParameterType::Float },
				{ TEXT("User.Direction"), EBBNiagaraParameterType::Vector },
				{ TEXT("User.Lifetime"), EBBNiagaraParameterType::Float },
				{ TEXT("User.bEmpowered"), EBBNiagaraParameterType::Bool },
				{ TEXT("User.bEnemy"), EBBNiagaraParameterType::Bool }
			} },
			{ TEXT("NS_BB_BeamTether"), {
				{ TEXT("User.PrimaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.SecondaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.TeamRelationColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.Length"), EBBNiagaraParameterType::Float },
				{ TEXT("User.Width"), EBBNiagaraParameterType::Float },
				{ TEXT("User.Direction"), EBBNiagaraParameterType::Vector },
				{ TEXT("User.Progress"), EBBNiagaraParameterType::Float },
				{ TEXT("User.bEmpowered"), EBBNiagaraParameterType::Bool },
				{ TEXT("User.bEnemy"), EBBNiagaraParameterType::Bool }
			} },
			{ TEXT("NS_BB_GroundTelegraph"), {
				{ TEXT("User.PrimaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.TeamRelationColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.Radius"), EBBNiagaraParameterType::Float },
				{ TEXT("User.Progress"), EBBNiagaraParameterType::Float },
				{ TEXT("User.Lifetime"), EBBNiagaraParameterType::Float },
				{ TEXT("User.bEmpowered"), EBBNiagaraParameterType::Bool },
				{ TEXT("User.bEnemy"), EBBNiagaraParameterType::Bool }
			} },
			{ TEXT("NS_BB_PersistentGroundZone"), {
				{ TEXT("User.PrimaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.SecondaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.TeamRelationColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.Radius"), EBBNiagaraParameterType::Float },
				{ TEXT("User.Progress"), EBBNiagaraParameterType::Float },
				{ TEXT("User.bEmpowered"), EBBNiagaraParameterType::Bool },
				{ TEXT("User.bEnemy"), EBBNiagaraParameterType::Bool }
			} },
			{ TEXT("NS_BB_ConeWedge"), {
				{ TEXT("User.PrimaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.TeamRelationColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.Length"), EBBNiagaraParameterType::Float },
				{ TEXT("User.Width"), EBBNiagaraParameterType::Float },
				{ TEXT("User.Direction"), EBBNiagaraParameterType::Vector },
				{ TEXT("User.Lifetime"), EBBNiagaraParameterType::Float },
				{ TEXT("User.bEmpowered"), EBBNiagaraParameterType::Bool },
				{ TEXT("User.bEnemy"), EBBNiagaraParameterType::Bool }
			} },
			{ TEXT("NS_BB_ImpactBurst"), {
				{ TEXT("User.PrimaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.SecondaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.TeamRelationColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.Radius"), EBBNiagaraParameterType::Float },
				{ TEXT("User.Direction"), EBBNiagaraParameterType::Vector },
				{ TEXT("User.Lifetime"), EBBNiagaraParameterType::Float },
				{ TEXT("User.bEmpowered"), EBBNiagaraParameterType::Bool }
			} },
			{ TEXT("NS_BB_CharacterAura"), {
				{ TEXT("User.PrimaryColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.TeamRelationColor"), EBBNiagaraParameterType::Color },
				{ TEXT("User.Radius"), EBBNiagaraParameterType::Float },
				{ TEXT("User.Intensity"), EBBNiagaraParameterType::Float },
				{ TEXT("User.bEmpowered"), EBBNiagaraParameterType::Bool },
				{ TEXT("User.bEnemy"), EBBNiagaraParameterType::Bool }
			} }
		};
		return Specs;
	}

	const FNiagaraTypeDefinition& GetExpectedType(EBBNiagaraParameterType Type)
	{
		switch (Type)
		{
		case EBBNiagaraParameterType::Color:
			return FNiagaraTypeDefinition::GetColorDef();
		case EBBNiagaraParameterType::Float:
			return FNiagaraTypeDefinition::GetFloatDef();
		case EBBNiagaraParameterType::Vector:
			return FNiagaraTypeDefinition::GetVec3Def();
		case EBBNiagaraParameterType::Bool:
			return FNiagaraTypeDefinition::GetBoolDef();
		default:
			checkNoEntry();
			return FNiagaraTypeDefinition::GetFloatDef();
		}
	}

	FString GetTypeLabel(EBBNiagaraParameterType Type)
	{
		switch (Type)
		{
		case EBBNiagaraParameterType::Color: return TEXT("LinearColor");
		case EBBNiagaraParameterType::Float: return TEXT("Float");
		case EBBNiagaraParameterType::Vector: return TEXT("Vector3");
		case EBBNiagaraParameterType::Bool: return TEXT("Bool");
		default: return TEXT("Unknown");
		}
	}

	FString SanitizeReportField(FString Value)
	{
		Value.ReplaceInline(TEXT("\t"), TEXT(" "));
		Value.ReplaceInline(TEXT("\r"), TEXT(" "));
		Value.ReplaceInline(TEXT("\n"), TEXT(" "));
		return Value;
	}
}

UBBNiagaraMasterAuditCommandlet::UBBNiagaraMasterAuditCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UBBNiagaraMasterAuditCommandlet::Main(const FString& Params)
{
	FString ReportPath;
	if (!FParse::Value(*Params, TEXT("Report="), ReportPath) || ReportPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("BB_NIAGARA_AUDIT|ERROR|missing Report= path"));
		return 1;
	}

	TArray<FString> ReportLines;
	ReportLines.Add(TEXT("AssetPath\tStatus\tEmitterCount\tCpuOnly\tFixedBounds\tNoEnabledLights\tUserParameterCount\tFailures"));
	int32 PassedCount = 0;

	for (const FBBNiagaraMasterSpec& Spec : GetMasterSpecs())
	{
		const FString AssetPath = FString::Printf(
			TEXT("/Game/GameplayCues/Templates/%s.%s"), Spec.Name, Spec.Name);
		UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *AssetPath);
		if (!System)
		{
			ReportLines.Add(FString::Printf(TEXT("%s\tMISSING\t0\t0\t0\t0\t0\tasset_missing"), *AssetPath));
			UE_LOG(LogTemp, Display, TEXT("BB_NIAGARA_AUDIT|MISSING|asset=%s"), *AssetPath);
			continue;
		}

		TArray<FString> Failures;
		TArray<FNiagaraVariable> UserParameters;
		System->GetExposedParameters().GetUserParameters(UserParameters);
		for (const FBBRequiredNiagaraParameter& Required : Spec.RequiredParameters)
		{
			const FNiagaraVariable* Found = UserParameters.FindByPredicate(
				[&Required](const FNiagaraVariable& Variable)
				{
					return Variable.GetName() == FName(Required.Name);
				});
			if (!Found)
			{
				Failures.Add(FString::Printf(TEXT("missing_%s:%s"), Required.Name, *GetTypeLabel(Required.Type)));
			}
			else if (Found->GetType() != GetExpectedType(Required.Type))
			{
				Failures.Add(FString::Printf(TEXT("wrong_type_%s:expected_%s"),
					Required.Name, *GetTypeLabel(Required.Type)));
			}
		}

		int32 EnabledEmitterCount = 0;
		bool bCpuOnly = true;
		bool bNoEnabledLights = true;
		for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
		{
			if (!Handle.GetIsEnabled())
			{
				continue;
			}

			++EnabledEmitterCount;
			const FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();
			if (!EmitterData)
			{
				bCpuOnly = false;
				Failures.Add(FString::Printf(TEXT("emitter_%s_has_no_versioned_data"), *Handle.GetName().ToString()));
				continue;
			}

			if (EmitterData->SimTarget != ENiagaraSimTarget::CPUSim)
			{
				bCpuOnly = false;
			}
			Handle.ForEachEnabledRendererWithIndex(
				[&bNoEnabledLights](const UNiagaraRendererProperties* Renderer, int32 RendererIndex)
				{
					(void)RendererIndex;
					if (Renderer && Renderer->IsA<UNiagaraLightRendererProperties>())
					{
						bNoEnabledLights = false;
					}
				});
		}

		if (EnabledEmitterCount == 0)
		{
			Failures.Add(TEXT("no_enabled_emitters"));
		}
		if (!bCpuOnly)
		{
			Failures.AddUnique(TEXT("non_cpu_emitter"));
		}
		if (!bNoEnabledLights)
		{
			Failures.Add(TEXT("enabled_light_renderer"));
		}

		const FBox Bounds = System->GetFixedBounds();
		const bool bHasValidFixedBounds = System->bFixedBounds
			&& Bounds.IsValid
			&& !Bounds.GetExtent().IsNearlyZero();
		if (!bHasValidFixedBounds)
		{
			Failures.Add(TEXT("missing_or_invalid_system_fixed_bounds"));
		}

		const bool bPassed = Failures.IsEmpty();
		PassedCount += bPassed ? 1 : 0;
		const FString FailureText = Failures.IsEmpty() ? TEXT("none") : FString::Join(Failures, TEXT(";"));
		ReportLines.Add(FString::Printf(TEXT("%s\t%s\t%d\t%d\t%d\t%d\t%d\t%s"),
			*AssetPath,
			bPassed ? TEXT("PASS") : TEXT("FAIL"),
			EnabledEmitterCount,
			bCpuOnly ? 1 : 0,
			bHasValidFixedBounds ? 1 : 0,
			bNoEnabledLights ? 1 : 0,
			UserParameters.Num(),
			*SanitizeReportField(FailureText)));
		UE_LOG(LogTemp, Display, TEXT("BB_NIAGARA_AUDIT|%s|asset=%s emitters=%d params=%d failures=%s"),
			bPassed ? TEXT("PASS") : TEXT("FAIL"), *AssetPath, EnabledEmitterCount,
			UserParameters.Num(), *FailureText);
	}

	if (!FFileHelper::SaveStringArrayToFile(ReportLines, *ReportPath))
	{
		UE_LOG(LogTemp, Error, TEXT("BB_NIAGARA_AUDIT|ERROR|report_write_failed path=%s"), *ReportPath);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("BB_NIAGARA_AUDIT|COMPLETE|passed=%d total=%d report=%s"),
		PassedCount, GetMasterSpecs().Num(), *ReportPath);
	return 0;
}
