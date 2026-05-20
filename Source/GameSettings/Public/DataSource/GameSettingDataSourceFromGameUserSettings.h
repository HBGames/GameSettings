// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingDataSource.h"
#include "PropertyPathHelpers.h"
#include "Templates/SubclassOf.h"

#define UE_API GAMESETTINGS_API

class UGameUserSettings;

/**
 * Data source that resolves a property/function chain rooted at the engine's
 * UGameUserSettings singleton (GEngine->GetGameUserSettings()). Use this when
 * a plugin or project subclasses UGameUserSettings (e.g. UCommonSettingsLocal)
 * and wants to bind a setting directly to it without inventing a wrapper
 * subsystem.
 *
 * The configured GameUserSettingsClass is used for editor-time validation and
 * a runtime IsA check; mismatches are logged and resolution fails safely.
 */
class FGameSettingDataSourceFromGameUserSettings : public FGameSettingDataSource
{
public:
	UE_API FGameSettingDataSourceFromGameUserSettings(TSubclassOf<UGameUserSettings> InGameUserSettingsClass, const TArray<FString>& InPathFromGameUserSettings);

	UE_API virtual bool Resolve(ULocalPlayer* InLocalPlayer) override;
	UE_API virtual FString GetValueAsString(ULocalPlayer* InLocalPlayer) const override;
	UE_API virtual void SetValue(ULocalPlayer* InLocalPlayer, const FString& Value) override;
	UE_API virtual FString ToString() const override;
	UE_API virtual void Persist(ULocalPlayer* InLocalPlayer) override;
	UE_API virtual FString GetPersistKey() const override;

private:
	UE_API UGameUserSettings* ResolveGameUserSettings() const;

	TSubclassOf<UGameUserSettings> GameUserSettingsClass;
	FCachedPropertyPath PropertyPath;
};

#undef UE_API
