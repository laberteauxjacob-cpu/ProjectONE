using UnrealBuildTool;
public class ProjectONE : ModuleRules
{
    public ProjectONE(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {"Core", "CoreUObject", "Engine", "InputCore", "AIModule", "NavigationSystem", "AnimGraphRuntime", "ProceduralMeshComponent", "RenderCore", "RHI", "Json", "JsonUtilities"});
    }
}
