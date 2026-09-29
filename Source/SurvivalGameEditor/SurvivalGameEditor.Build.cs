using UnrealBuildTool;

public class SurvivalGameEditor : ModuleRules
{
    public SurvivalGameEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "..", "SurvivalGame"));
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "Json", "RenderCore", "InputCore", "Landscape",
            "ImageWrapper", "AssetRegistry", "AssetTools", "SurvivalGame"
        });
    }
}
