// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RacingGame : ModuleRules
{
	public RacingGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"RacingGame",
			"RacingGame/Vehicle",
			"RacingGame/Test",
			"RacingGame/Track"
		});
	}
}
