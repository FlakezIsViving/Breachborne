using UnrealBuildTool;

public class LudusEditor : ModuleRules
{
    public LudusEditor(ReadOnlyTargetRules Target) : base(Target)
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
                "LudusCore",
                "EditorScriptingUtilities"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",
                "LevelEditor",
                "EditorStyle",
                "AssetTools",
                "AssetRegistry",
                "Slate",
                "SlateCore",
                "RHI",
                "RenderCore",
                "UMG",
            }
        );
    }
}
