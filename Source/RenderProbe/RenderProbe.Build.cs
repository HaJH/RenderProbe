// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

using UnrealBuildTool;

public class RenderProbe : ModuleRules
{
	public RenderProbe(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"EditorSubsystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ImageCore",
			"Json",
			"JsonUtilities",
			"MaterialEditor",
			"RHI"
		});

		if (Target.Configuration != UnrealTargetConfiguration.Test && Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateDependencyModuleNames.Add("CQTest");
		}
	}
}
