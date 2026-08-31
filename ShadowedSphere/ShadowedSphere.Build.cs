// Build rules for the ShadowedSphere game module: precompiled-header mode and the engine
// modules it links against.

using UnrealBuildTool;

public class ShadowedSphere : ModuleRules
{
    public ShadowedSphere(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "ProceduralMeshComponent"
        });
    }
}
