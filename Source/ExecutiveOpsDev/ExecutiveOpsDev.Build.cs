using UnrealBuildTool;

/**
 * Development-only code: the self-test harness and the console cheats.
 *
 * A separate module rather than #if guards scattered through the game module,
 * because "does this ship" is then a property of where a file lives rather than
 * of every future author remembering a macro. The uproject marks this
 * DeveloperTool, so a shipping build never links it.
 *
 * The dependency runs one way: this module knows about the game, the game knows
 * nothing about this. Both entry points are engine hooks - see ExecutiveOpsDev.cpp.
 */
public class ExecutiveOpsDev : ModuleRules
{
	public ExecutiveOpsDev(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"ExecutiveOps",
			"FunctionalTesting",
			"NavigationSystem"
		});

		PublicIncludePaths.Add(ModuleDirectory);
	}
}
