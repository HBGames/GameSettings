// Copyright Hitbox Games, LLC. All Rights Reserved.

using UnrealBuildTool;

public class GameSettings : ModuleRules
{
	public GameSettings(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"InputCore",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"CommonInput",
				"CommonUI",
				"GameplayTags",
				// DeveloperSettings is part of the public surface:
				// GameSettingsDeveloperSettings.h is a public header.
				"DeveloperSettings",
				// PropertyPath is part of the public surface: GameSettingDataSourceDynamic.h
				// (a public header) includes PropertyPathHelpers.h, so external consumers
				// need this on their include path.
				"PropertyPath",
				// MVVM. ViewModel headers extend UMVVMViewModelBase and use
				// FieldNotify; the resolver subclasses UMVVMViewModelContextResolver.
				"FieldNotification",
				"ModelViewViewModel",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"ApplicationCore",
			}
		);
	}
}
