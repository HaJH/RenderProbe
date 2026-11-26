// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

using UnrealBuildTool;

public class RenderProbeToolset : ModuleRules
{
	public RenderProbeToolset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"RenderProbe",
			"ToolsetRegistry"
		});
	}
}
