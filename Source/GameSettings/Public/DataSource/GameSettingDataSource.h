// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Delegates/Delegate.h"

class ULocalPlayer;

//--------------------------------------
// FGameSettingDataSource
//--------------------------------------

class FGameSettingDataSource : public TSharedFromThis<FGameSettingDataSource>
{
public:
	virtual ~FGameSettingDataSource() { }

	/**
	 * Some settings may take an async amount of time to finish initializing.  The settings system will wait
	 * for all settings to be ready before showing the setting.
	 */
	virtual void Startup(ULocalPlayer* InLocalPlayer, FSimpleDelegate StartupCompleteCallback) { StartupCompleteCallback.ExecuteIfBound(); }

	virtual bool Resolve(ULocalPlayer* InContext) = 0;

	virtual FString GetValueAsString(ULocalPlayer* InContext) const = 0;

	virtual void SetValue(ULocalPlayer* InContext, const FString& Value) = 0;

	virtual FString ToString() const = 0;

	/**
	 * Flush this data source's backing store to disk. Called once per distinct
	 * store by UGameSettingRegistry::SaveChanges (de-duplicated via
	 * GetPersistKey). Default is a no-op for stores that don't persist or
	 * persist themselves elsewhere.
	 */
	virtual void Persist(ULocalPlayer* InContext) { }

	/**
	 * Stable identity for the backing store this data source writes to, used
	 * to de-duplicate Persist calls (40 video settings on one
	 * GameUserSettings object should ApplySettings once, not 40 times). Empty
	 * string means "not persistable" - excluded from the save pass.
	 */
	virtual FString GetPersistKey() const { return FString(); }
};
