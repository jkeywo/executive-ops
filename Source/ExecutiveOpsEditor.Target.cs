using UnrealBuildTool;

public class ExecutiveOpsEditorTarget : TargetRules
{
	public ExecutiveOpsEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ExecutiveOps");
	}
}
