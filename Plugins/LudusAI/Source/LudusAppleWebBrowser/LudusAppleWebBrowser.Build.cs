using System.IO;
using UnrealBuildTool;

public class LudusAppleWebBrowser : ModuleRules
{
	public LudusAppleWebBrowser(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PrivateIncludePaths.AddRange(new string[]
		{
			Path.Combine(ModuleDirectory, "Private"),
			Path.Combine(ModuleDirectory, "Private", "Apple"),
			Path.Combine(ModuleDirectory, "Private", "MobileJS"),
			Path.Combine(EngineDirectory, "Source", "Runtime", "WebBrowser", "Private"),
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"ApplicationCore",
			"RHI",
			"InputCore",
			"Serialization",
			"HTTP",
			"Slate",
			"SlateCore",
			"WebBrowser",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Engine",
			"Json",
			"JsonUtilities",
			"RenderCore",
			"WebBrowserTexture",
		});

		PublicFrameworks.AddRange(new string[]
		{
			"WebKit",
			"Foundation",
		});

		PublicDefinitions.Add("PLATFORM_SPECIFIC_WEB_BROWSER=0");
	}
}
