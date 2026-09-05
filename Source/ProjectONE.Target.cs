using UnrealBuildTool;
public class ProjectONETarget : TargetRules
{
    public ProjectONETarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("ProjectONE");
    }
}
