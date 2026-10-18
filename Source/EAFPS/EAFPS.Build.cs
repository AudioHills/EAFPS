using UnrealBuildTool;

public class EAFPS : ModuleRules
{
	public EAFPS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"AIModule", "NavigationSystem", "DeveloperSettings"
		});
	}
}
