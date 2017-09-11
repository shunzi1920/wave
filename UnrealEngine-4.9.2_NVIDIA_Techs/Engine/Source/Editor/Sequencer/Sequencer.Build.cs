// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Sequencer : ModuleRules
{
	public Sequencer(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Editor/Sequencer/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
                "InputCore",
				"Engine", 
				"Slate", 
				"SlateCore",
                "EditorStyle",
				"UnrealEd", 
				"MovieScene", 
				"MovieSceneTracks", 
				"MovieSceneTools", 
				"EditorWidgets", 
				"SequencerWidgets",
				"BlueprintGraph"
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"PropertyEditor",
				"Kismet"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"PropertyEditor",
				"WorkspaceMenuStructure"
			}
		);

		CircularlyReferencedDependentModules.Add("MovieSceneTools");
	}
}
