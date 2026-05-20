// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "DataSource/GameSettingDataSourceFromGameUserSettings.h"

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "GameSettingsLog.h"

FGameSettingDataSourceFromGameUserSettings::FGameSettingDataSourceFromGameUserSettings(
	TSubclassOf<UGameUserSettings> InGameUserSettingsClass,
	const TArray<FString>& InPathFromGameUserSettings)
	: GameUserSettingsClass(InGameUserSettingsClass)
{
	FString JoinedPath;
	for (int32 Index = 0; Index < InPathFromGameUserSettings.Num(); ++Index)
	{
		if (Index > 0)
		{
			JoinedPath += TEXT(".");
		}
		JoinedPath += InPathFromGameUserSettings[Index];
	}
	PropertyPath = FCachedPropertyPath(JoinedPath);
}

bool FGameSettingDataSourceFromGameUserSettings::Resolve(ULocalPlayer* InLocalPlayer)
{
	UGameUserSettings* Settings = ResolveGameUserSettings();
	if (!Settings)
	{
		return false;
	}
	return PropertyPath.Resolve(Settings);
}

FString FGameSettingDataSourceFromGameUserSettings::GetValueAsString(ULocalPlayer* InLocalPlayer) const
{
	UGameUserSettings* Settings = ResolveGameUserSettings();
	if (!Settings)
	{
		return FString();
	}

	FString Out;
	PropertyPathHelpers::GetPropertyValueAsString(Settings, PropertyPath.ToString(), Out);
	return Out;
}

void FGameSettingDataSourceFromGameUserSettings::SetValue(ULocalPlayer* InLocalPlayer, const FString& Value)
{
	UGameUserSettings* Settings = ResolveGameUserSettings();
	if (!Settings)
	{
		return;
	}
	PropertyPathHelpers::SetPropertyValueFromString(Settings, PropertyPath.ToString(), Value);
}

FString FGameSettingDataSourceFromGameUserSettings::ToString() const
{
	return FString::Printf(TEXT("%s.%s"),
		GameUserSettingsClass ? *GameUserSettingsClass->GetName() : TEXT("(null GameUserSettings class)"),
		*PropertyPath.ToString());
}

void FGameSettingDataSourceFromGameUserSettings::Persist(ULocalPlayer* InLocalPlayer)
{
	UGameUserSettings* Settings = ResolveGameUserSettings();
	if (!Settings)
	{
		return;
	}

	// ApplySettings applies hardware-affecting settings (resolution, vsync,
	// scalability) AND writes GameUserSettings.ini via SaveSettings. This is
	// exactly what Lyra's ULyraGameSettingRegistry::SaveChanges does for its
	// local settings. bCheckForCommandLineOverrides = false matches Lyra.
	Settings->ApplySettings(false);
}

FString FGameSettingDataSourceFromGameUserSettings::GetPersistKey() const
{
	// One engine-global GameUserSettings object regardless of which subclass
	// the binding declared, so a single fixed key collapses every
	// GameUserSettings-bound setting into one ApplySettings call.
	return TEXT("GameUserSettings");
}

UGameUserSettings* FGameSettingDataSourceFromGameUserSettings::ResolveGameUserSettings() const
{
	if (!GEngine)
	{
		return nullptr;
	}

	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	if (!Settings)
	{
		return nullptr;
	}

	UClass* ConfiguredClass = GameUserSettingsClass.Get();
	if (ConfiguredClass && !Settings->IsA(ConfiguredClass))
	{
		UE_LOG(LogGameSettings, Warning,
			TEXT("FGameSettingDataSourceFromGameUserSettings: engine GameUserSettings is '%s' but binding expected '%s'. Check GameUserSettingsClassName in DefaultEngine.ini."),
			*Settings->GetClass()->GetName(),
			*ConfiguredClass->GetName());
		return nullptr;
	}

	return Settings;
}
