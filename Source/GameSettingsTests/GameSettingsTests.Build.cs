// Copyright Hitbox Games, LLC. All Rights Reserved.

using UnrealBuildTool;

public class GameSettingsTests : ModuleRules
{
	public GameSettingsTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				// FGameplayTag / FGameplayTagContainer appear on UGameSetting's
				// public surface; depend on it directly rather than leaning on
				// GameSettings' transitive export.
				"GameplayTags",
				// The module under test.
				"GameSettings",
			}
		);
	}
}
