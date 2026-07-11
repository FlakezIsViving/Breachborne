using System.IO;
using UnrealBuildTool;

public class LudusChatUI : ModuleRules
{
	public LudusChatUI(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
				"SlateCore",
				"UMG",
				"LudusClient",
				"Projects",
				"UnrealEd",
				"Documentation",
				"GraphEditor",
				"BlueprintGraph",
				"MessageLog",
				"ApplicationCore",
				"Json",
				"EditorSubsystem",
				"ImageDownload",
				"WebBrowser"
			}
		);

        // macOS: always use WKWebView (via LudusAppleWebBrowser module for UE < 5.7,
        // engine-native Apple WebBrowser for UE 5.7+). CEF is Windows/Linux only.
        if (Target.Platform != UnrealTargetPlatform.Mac)
        {
            AddEngineThirdPartyPrivateStaticDependencies(Target, "CEF3");
            PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Source/Runtime/WebBrowser/Private/"));
        }
        else
        {
            PublicFrameworks.Add("WebKit");
            PrivateDependencyModuleNames.Add("LudusAppleWebBrowser");
        }

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"LudusCore",
				"HTTP",
				"ImageWrapper",
				"WorkspaceMenuStructure",
				"ToolMenus",
				"AppFramework",
				"EditorFramework",
				"Kismet",
				"PlacementMode",
				"Settings",
				"PropertyEditor",
				"DesktopPlatform",
				"AssetTools",
				"SourceCodeAccess",
				"ContentBrowser",
				"LevelEditor",
				"AssetRegistry",
				"Analytics",
				"GameProjectGeneration",
				"ImageCore"
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"MainFrame",
				"TargetPlatform",
				"TargetDeviceServices",
				"LauncherServices",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"MainFrame",
				"LauncherServices",
			}
		);
	}
}
