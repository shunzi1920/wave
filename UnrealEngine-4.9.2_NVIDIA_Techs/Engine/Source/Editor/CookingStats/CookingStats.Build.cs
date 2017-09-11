// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CookingStats : ModuleRules
{
	public CookingStats(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject"
			}
			);

		if (UEBuildConfiguration.bBuildEditor == true)
		{
			PrivateIncludePathModuleNames.AddRange(new string[] { "DirectoryWatcher" });
			DynamicallyLoadedModuleNames.AddRange(new string[] { "DirectoryWatcher" });
		}
	}
}
