using System;
using System.IO;
using UnrealBuildTool;

public class LudusCore : ModuleRules
{
	public LudusCore(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"HTTP",
			"HTTPServer",
			"Json",
			"JsonUtilities",
			"EditorScriptingUtilities",
			"Projects",
			"RenderCore",
			"RHICore",
			"RHI",
			"AssetRegistry",
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Slate",
			"SlateCore",
			"DeveloperSettings",
			"UnrealEd",
			"EditorSubsystem",
			"Projects"
		});

		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicDefinitions.Add("PLATFORM_MAC");
			PublicFrameworks.AddRange(new string[] {
				"Foundation",
				"Security",
				"SystemConfiguration",
			});
		}

	}

}
