// Copyright Hitbox Games, LLC. All Rights Reserved.

using UnrealBuildTool;

public class GameSettingsGameFeatures : ModuleRules
{
	public GameSettingsGameFeatures(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameFeatures",
				"GameSettings",
				"GameplayTags",
			}
		);
	}
}
