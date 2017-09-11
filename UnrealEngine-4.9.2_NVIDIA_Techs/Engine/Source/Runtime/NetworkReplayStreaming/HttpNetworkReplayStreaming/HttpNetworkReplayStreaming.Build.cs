// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class HttpNetworkReplayStreaming : ModuleRules
    {
        public HttpNetworkReplayStreaming(TargetInfo Target)
        {
			PrivateIncludePathModuleNames.Add( "OnlineSubsystem" );

			PrivateDependencyModuleNames.AddRange(
                new string[]
				{
					"Core",
					"CoreUObject",
                    "Engine",
                    "HTTP",
					"NetworkReplayStreaming",
					"Json",
				} );
        }
    }
}
