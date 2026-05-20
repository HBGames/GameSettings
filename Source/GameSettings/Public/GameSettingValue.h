// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameSetting.h"

#include "GameSettingValue.generated.h"

#define UE_API GAMESETTINGS_API

class UObject;
class FGameSettingDataSource;

//--------------------------------------
// UGameSettingValue
//--------------------------------------

/**
 * The base class for all settings that are conceptually a value, that can be 
 * changed, and thus reset or restored to their initial value.
 */
UCLASS(MinimalAPI, Abstract)
class UGameSettingValue : public UGameSetting
{
	GENERATED_BODY()

public:
	UE_API UGameSettingValue();

	/** Stores an initial value for the setting.  This will be called on initialize, but should also be called if you 'apply' the setting. */
	virtual void StoreInitial() PURE_VIRTUAL(, );

	/** Resets the property to the default. */
	virtual void ResetToDefault() PURE_VIRTUAL(, );

	/** Restores the setting to the initial value, this is the value when you open the settings before making any tweaks. */
	virtual void RestoreToInitial() PURE_VIRTUAL(, );

	/**
	 * Returns the data source whose backing store must be flushed to disk for
	 * this setting's value to persist. UGameSettingRegistry::SaveChanges
	 * de-duplicates these by GetPersistKey and calls Persist once per store.
	 * Default null = nothing to persist (e.g. an action setting, or a value
	 * type that persists through some other channel).
	 */
	virtual TSharedPtr<FGameSettingDataSource> GetPersistableDataSource() const { return nullptr; }

	/**
	 * Whether this setting's current value differs from its configured default,
	 * i.e. resetting it would actually change something. Drives the screen's
	 * Reset-To-Defaults affordance, which is intentionally NOT the same as dirty
	 * state: a fresh-but-non-default value should still offer a reset, and a
	 * just-reset value should stop offering one even though the change tracker
	 * still considers it a pending change. Default false = no concept of a
	 * default (e.g. an action), so never resettable. Concrete value types with
	 * a configured default override this.
	 */
	virtual bool IsResettableToDefault() const { return false; }

protected:
	UE_API virtual void OnInitialized() override;
};

#undef UE_API
