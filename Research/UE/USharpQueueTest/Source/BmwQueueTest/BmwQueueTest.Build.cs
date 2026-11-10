using UnrealBuildTool;
using System.IO;
public class BmwQueueTest : ModuleRules
{
    public BmwQueueTest(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
        PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../../Source/USharp/Private")));
    }
}
