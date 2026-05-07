// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"

#include "GameSettingsDeveloperSettings.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSettingRegistry;

/**
 * Project Settings page for the GameSettings plugin.
 *
 * Lets a project pick a UGameSettingRegistry subclass without writing
 * any wiring code. The subsystem instantiates RegistryClass on first
 * need. Leave it unset to get a plain UGameSettingRegistry.
 */
UCLASS(Config = Game, DefaultConfig, MinimalAPI, DisplayName = "Game Settings")
class UGameSettingsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	/**
	 * Subclass of UGameSettingRegistry that UGameSettingsSubsystem
	 * instantiates when no registry has been provided externally. Defaults
	 * to the plain UGameSettingRegistry. Projects that prefer the Lyra
	 * pattern point this at their own subclass.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Registry", meta = (MetaClass = "/Script/GameSettings.GameSettingRegistry", AllowAbstract = "false"))
	FSoftClassPath RegistryClass;

	UE_API virtual FName GetCategoryName() const override;
};

#undef UE_API
