// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Renderer : ModuleRules
{
	public Renderer(TargetInfo Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"Runtime/Renderer/Private",
				"Runtime/Renderer/Private/CompositionLighting",
				"Runtime/Renderer/Private/PostProcess",
			}
		);

		PublicDependencyModuleNames.Add("Core");
        PublicDependencyModuleNames.Add("Engine");

        // Renderer module builds faster without unity
        // Non-unity also provides faster iteration
		// Not enabled by default as it might harm full rebuild times without XGE
        //bFasterWithoutUnity = true;

        MinFilesUsingPrecompiledHeaderOverride = 4;

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"CoreUObject", 
				"RenderCore", 
				"RHI", 
				"ShaderCore",
				"UtilityShaders",
                "WaveWorks"
			}
            );

        PrivateIncludePathModuleNames.AddRange(new string[] { "HeadMountedDisplay", "LightPropagationVolumeRuntime" });
        DynamicallyLoadedModuleNames.AddRange(new string[] { "HeadMountedDisplay" });

        // NVCHANGE_BEGIN: Add VXGI
        if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
        {
            PublicDependencyModuleNames.Add("VXGI");
        }
        // NVCHANGE_END: Add VXGI
	}
}
