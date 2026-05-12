// Copyright Hitbox Games, LLC. All Rights Reserved.

using UnrealBuildTool;

public class GameSettingsEditor : ModuleRules
{
	public GameSettingsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"EditorStyle",
				"PropertyEditor",
				"PropertyPath",
				"AssetDefinition",
				"GameplayTags",
				"GameSettings",
			}
		);
	}
}
