using UnrealBuildTool;

public class SurvivalGame : ModuleRules
{
    public SurvivalGame(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivatePCHHeaderFile = "SurvivalGamePCH.h";
        PrivateIncludePaths.Add(ModuleDirectory);
        CppStandard = CppStandardVersion.Cpp20;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
            "ProceduralMeshComponent", "AudioMixer", "AnimGraphRuntime", "Json",
            "Slate", "SlateCore", "HairStrandsCore"
        });
        PrivateDependencyModuleNames.AddRange(new[] {"PhysicsCore", "RHI", "RenderCore"});
    }
}
