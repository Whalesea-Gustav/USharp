using UnrealBuildTool;
public class USharpQueueTestEditorTarget : TargetRules
{
    public USharpQueueTestEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        ExtraModuleNames.Add("BmwQueueTest");
    }
}
