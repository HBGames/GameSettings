// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "DataSource/GameSettingDataSourceEnhancedInput.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "UserSettings/EnhancedInputUserSettings.h"

const FString FGameSettingDataSourceEnhancedInput::PersistKey = TEXT("EnhancedInputUserSettings");

namespace UE::GameSettings::Private
{
	static UEnhancedInputUserSettings* GetUserSettingsForPlayer(ULocalPlayer* InLocalPlayer)
	{
		if (!InLocalPlayer)
		{
			return nullptr;
		}
		if (UEnhancedInputLocalPlayerSubsystem* System = InLocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			return System->GetUserSettings();
		}
		return nullptr;
	}
}

bool FGameSettingDataSourceEnhancedInput::Resolve(ULocalPlayer* InLocalPlayer)
{
	return UE::GameSettings::Private::GetUserSettingsForPlayer(InLocalPlayer) != nullptr;
}

FString FGameSettingDataSourceEnhancedInput::GetValueAsString(ULocalPlayer* /*InLocalPlayer*/) const
{
	// Key bindings are read from the mapping profile, not from a string value.
	return FString();
}

void FGameSettingDataSourceEnhancedInput::SetValue(ULocalPlayer* /*InLocalPlayer*/, const FString& /*Value*/)
{
	// No string value to write. Rebinds go through UGameSettingValueKeyBinding.
}

FString FGameSettingDataSourceEnhancedInput::ToString() const
{
	return PersistKey;
}

void FGameSettingDataSourceEnhancedInput::Persist(ULocalPlayer* InLocalPlayer)
{
	if (UEnhancedInputUserSettings* Settings = UE::GameSettings::Private::GetUserSettingsForPlayer(InLocalPlayer))
	{
		Settings->AsyncSaveSettings();
	}
}

FString FGameSettingDataSourceEnhancedInput::GetPersistKey() const
{
	return PersistKey;
}
