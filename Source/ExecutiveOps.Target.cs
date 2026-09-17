using UnrealBuildTool;

public class ExecutiveOpsTarget : TargetRules
{
	public ExecutiveOpsTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ExecutiveOps");
	}
}
