// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class Kinect : ModuleRules
{
	public Kinect(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		string sdkPath = System.Environment.GetEnvironmentVariable("KINECTSDK20_DIR");
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
				Path.Combine(sdkPath, "inc")
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
		);
		
		PublicAdditionalLibraries.AddRange(
			new string[]
			{
				Path.Combine(sdkPath, "Lib", "x64", "Kinect20.lib"),
				Path.Combine(sdkPath, "Lib", "x64", "Kinect20.Fusion.lib"),
				Path.Combine(sdkPath, "Lib", "x64", "Kinect20.Face.lib"),
				Path.Combine(sdkPath, "Lib", "x64", "Kinect20.VisualGestureBuilder.lib"),
			}
		);

		string kinectDllPath = "C:/Windows/System32/Kinect20.dll";
		string fusion2DllPath = Path.Combine(sdkPath, "Redist", "Fusion", "x64", "Kinect20.Fusion.dll");
		
		PublicDelayLoadDLLs.AddRange(new string[]
		{
			"Kinect20.dll",
			"Kinect20.Fusion.dll"
		});
		

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"RHI",
				"RenderCore"
				// ... add other public dependencies that you statically link with here ...
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}