// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Sandbox : ModuleRules
{
	public Sandbox(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", 
			"CoreUObject", "Engine", "RHI", "InputCore", "GeometryCore", "ProceduralMeshComponent", "GeometryFramework", "OpenCV", "OpenCVHelper" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Kinect", "GeometryScriptingCore" });

		// Uncomment if you are using Slate UI
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UMG" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
