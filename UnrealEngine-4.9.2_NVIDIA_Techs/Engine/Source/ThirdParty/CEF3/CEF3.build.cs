// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class CEF3 : ModuleRules
{
	public CEF3(TargetInfo Target)
	{
		/** Mark the current version of the library */
		string CEFVersion = "3.2272.2077";
		string CEFPlatform = "";

		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			CEFPlatform = "windows64";
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32)
		{
			CEFPlatform = "windows32";
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			CEFVersion = "3.2272.24.ga8ab9ad";
			CEFPlatform = "macosx64";
		}

		if (CEFPlatform.Length > 0 && UEBuildConfiguration.bCompileCEF3)
		{
			Definitions.Add("WITH_CEF3=1");

			string PlatformPath = UEBuildConfiguration.UEThirdPartySourceDirectory + "CEF3/cef_binary_" + CEFVersion + "_" + CEFPlatform;

			PublicSystemIncludePaths.Add(PlatformPath);

			string LibraryPath = PlatformPath + "/Release";

			if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
			{
                string VSVersionFolderName = "/VS" + WindowsPlatform.GetVisualStudioCompilerVersionName();
                LibraryPath += VSVersionFolderName;

                PublicLibraryPaths.Add(LibraryPath);

				//PublicAdditionalLibraries.Add("cef_sandbox.lib");
				PublicAdditionalLibraries.Add("libcef.lib");
				if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
				{
					PublicAdditionalLibraries.Add("libcef_dll_wrapperD.lib");
				}
				else
				{
					PublicAdditionalLibraries.Add("libcef_dll_wrapper.lib");
				}

				PublicDelayLoadDLLs.Add("d3dcompiler_43.dll");
				PublicDelayLoadDLLs.Add("d3dcompiler_47.dll");
				PublicDelayLoadDLLs.Add("ffmpegsumo.dll");
				PublicDelayLoadDLLs.Add("libcef.dll");
				PublicDelayLoadDLLs.Add("libEGL.dll");
                PublicDelayLoadDLLs.Add("libGLESv2.dll");
                PublicDelayLoadDLLs.Add("pdf.dll");

				RuntimeDependencies.Add(new RuntimeDependency("$(EngineDir)/Binaries/" + Target.Platform.ToString() + "/UnrealCEFSubProcess.exe"));
			}
			// TODO: Ensure these are filled out correctly when adding other platforms
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				string WrapperPath = LibraryPath + "/libcef_dll_wrapper.a";
                string FrameworkPath = UEBuildConfiguration.UEThirdPartyBinariesDirectory + "CEF3/Mac/Chromium Embedded Framework.framework";

				PublicAdditionalLibraries.Add(WrapperPath);
                PublicFrameworks.Add(FrameworkPath);

                var LocaleFolders = Directory.GetFileSystemEntries(LibraryPath + "/locale", "*.lproj");
				foreach (var FolderName in LocaleFolders)
				{
					AdditionalBundleResources.Add(new UEBuildBundleResource(FolderName, bInShouldLog:false));
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				if (Target.IsMonolithic)
				{
					PublicAdditionalLibraries.Add(LibraryPath + "/libcef.a");
				}
				else
				{
					PublicLibraryPaths.Add(LibraryPath);
					PublicAdditionalLibraries.Add("libcef");
				}
			}
		}
	}
}
