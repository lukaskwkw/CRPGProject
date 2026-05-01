// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CRPGProject : ModuleRules
{
	public CRPGProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"CRPGProject",
			"CRPGProject/Variant_Platforming",
			"CRPGProject/Variant_Platforming/Animation",
			"CRPGProject/Variant_Combat",
			"CRPGProject/Variant_Combat/AI",
			"CRPGProject/Variant_Combat/Animation",
			"CRPGProject/Variant_Combat/Gameplay",
			"CRPGProject/Variant_Combat/Interfaces",
			"CRPGProject/Variant_Combat/UI",
			"CRPGProject/Variant_SideScrolling",
			"CRPGProject/Variant_SideScrolling/AI",
			"CRPGProject/Variant_SideScrolling/Gameplay",
			"CRPGProject/Variant_SideScrolling/Interfaces",
			"CRPGProject/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
