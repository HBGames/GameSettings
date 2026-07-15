// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingDataSource.h"

#define UE_API GAMESETTINGS_API

class ULocalPlayer;

/**
 * Persist-only data source for settings that store their value in the local
 * player's Enhanced Input user settings (key bindings). It holds no value of
 * its own. GetValueAsString returns empty and SetValue is a no-op because key
 * bindings read and write the mapping profile directly, not through a string.
 *
 * Every key binding setting returns one of these, and they all share the same
 * persist key, so UGameSettingRegistry::SaveChanges collapses a screen full of
 * rebinds into a single AsyncSaveSettings call on Apply.
 */
class FGameSettingDataSourceEnhancedInput : public FGameSettingDataSource
{
public:
	/** The shared persist key. Public so callers and tests can assert the dedup contract. */
	static UE_API const FString PersistKey;

	UE_API virtual bool Resolve(ULocalPlayer* InLocalPlayer) override;
	UE_API virtual FString GetValueAsString(ULocalPlayer* InLocalPlayer) const override;
	UE_API virtual void SetValue(ULocalPlayer* InLocalPlayer, const FString& Value) override;
	UE_API virtual FString ToString() const override;
	UE_API virtual void Persist(ULocalPlayer* InLocalPlayer) override;
	UE_API virtual FString GetPersistKey() const override;
};

#undef UE_API
