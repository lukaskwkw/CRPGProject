// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
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
		  "NavigationSystem",
			"StateTreeModule",
			"GameplayTasks",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		AddExistingPublicIncludePaths(new string[] {
			"CRPGProject",
		  "CRPGProject/Core",
			"CRPGProject/Core/Logging",
			"CRPGProject/Framework",
			"CRPGProject/Framework/Controllers",
			"CRPGProject/Framework/Characters",
			"CRPGProject/Framework/Pawns",
			"CRPGProject/Framework/Components",
			"CRPGProject/Framework/Interfaces",
			"CRPGProject/Camera",
			"CRPGProject/Camera/Components",
			"CRPGProject/Camera/Subsystems",
			"CRPGProject/Camera/Actors",
			"CRPGProject/Camera/Data",
			"CRPGProject/Events",
			"CRPGProject/Events/Subsystems",
			"CRPGProject/Tactical",
			"CRPGProject/Tactical/Subsystems",
			"CRPGProject/Tactical/Components",
			"CRPGProject/Tactical/Movement",
			"CRPGProject/Tactical/Visualization",
			"CRPGProject/Tactical/Rules",
			"CRPGProject/Tactical/Data",
			"CRPGProject/Combat",
			"CRPGProject/Combat/GAS",
			"CRPGProject/Combat/Abilities",
			"CRPGProject/Combat/Effects",
			"CRPGProject/Combat/Attributes",
			"CRPGProject/Combat/Execution",
			"CRPGProject/Combat/Rules",
			"CRPGProject/UI",
			"CRPGProject/UI/HUD",
			"CRPGProject/UI/Widgets",
			"CRPGProject/UI/WebUI",
			"CRPGProject/UI/ViewModels",
			"CRPGProject/World",
			"CRPGProject/World/Simulation",
			"CRPGProject/World/Quest",
			"CRPGProject/World/Factions",
			"CRPGProject/World/Time",
			"CRPGProject/World/Navigation",
			"CRPGProject/Data",
			"CRPGProject/Data/DSL",
			"CRPGProject/Data/Definitions",
			"CRPGProject/Data/Tables",
			"CRPGProject/Data/Assets",
			"CRPGProject/Utilities",
			"CRPGProject/Utilities/Helpers",
			"CRPGProject/Utilities/Extensions",
			"CRPGProject/Utilities/Debug",
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

	private void AddExistingPublicIncludePaths(string[] RelativePaths)
	{
		foreach (string RelativePath in RelativePaths)
		{
			string FullPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", RelativePath));
			if (Directory.Exists(FullPath))
			{
				PublicIncludePaths.Add(RelativePath);
			}
		}
	}
}
