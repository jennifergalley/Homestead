using UnrealBuildTool;

public class SurvivalGameEditor : ModuleRules
{
    public SurvivalGameEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        PrivateIncludePathModuleNames.Add("PythonScriptPlugin");
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "UnrealEd", "Json", "Projects", "DerivedDataCache",
            "MaterialEditor", "MeshDescription", "StaticMeshDescription", "PhysicsCore", "RHI", "RenderCore"
        });
    }
}
