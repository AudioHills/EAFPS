using UnrealBuildTool;

public class EAFPSEditorTarget : TargetRules
{
	public EAFPSEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("EAFPS");
	}
}
