using UnrealBuildTool;

public class ExecutiveOps : ModuleRules
{
	public ExecutiveOps(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"DeveloperSettings",
			"ProceduralMeshComponent"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});

		PublicIncludePaths.Add(ModuleDirectory);
	}
}
