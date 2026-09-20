using UnrealBuildTool;

public class SurvivalGame : ModuleRules
{
    public SurvivalGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
            "ProceduralMeshComponent", "AudioMixer", "AnimGraphRuntime"
        });
    }
}
