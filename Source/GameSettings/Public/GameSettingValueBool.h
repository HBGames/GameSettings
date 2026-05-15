// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingValue.h"

#include "GameSettingValueBool.generated.h"

#define UE_API GAMESETTINGS_API

class FGameSettingDataSource;
enum class EGameSettingChangeReason : uint8;

/**
 * First-class boolean setting. Peer of UGameSettingValueDiscrete, not a child
 * of it - toggles are semantically a single bool, not "a 2-option discrete
 * that happens to be bool labels," so the type system reflects that.
 *
 * Storage routes through FGameSettingDataSource (string-based at the wire
 * level for compatibility with the existing data source layer) but the public
 * surface is typed bool. Consumers - the Toggle contribution, the Toggle VM -
 * read and write bool natively without going through option-list strings.
 *
 * Full UGameSettingValue lifecycle is implemented so the change tracker
 * (Apply / Cancel) and edit conditions work the same as Scalar and Discrete.
 */
UCLASS(MinimalAPI)
class UGameSettingValueBool : public UGameSettingValue
{
	GENERATED_BODY()
public:
	UE_API UGameSettingValueBool();

	//~ UGameSettingValue
	UE_API virtual void Startup() override;
	UE_API virtual void StoreInitial() override;
	UE_API virtual void ResetToDefault() override;
	UE_API virtual void RestoreToInitial() override;
	//~ End UGameSettingValue

	/** Bind a getter that returns bool (typically a UFUNCTION on the project's settings class). */
	UE_API void SetDynamicGetter(const TSharedRef<FGameSettingDataSource>& InGetter);

	/** Bind a setter that takes bool (typically a UFUNCTION on the project's settings class). */
	UE_API void SetDynamicSetter(const TSharedRef<FGameSettingDataSource>& InSetter);

	/** Default value applied on ResetToDefault. Unset means no default - reset is a no-op. */
	UE_API void SetDefaultValue(bool InDefault);

	/** Reads the current bool by calling the bound getter. */
	UE_API bool GetBoolValue() const;

	/** Writes a new bool by calling the bound setter and broadcasts the change. */
	UE_API void SetBoolValue(bool InValue);

protected:
	//~ UGameSettingValue
	UE_API virtual void OnInitialized() override;
	//~ End UGameSettingValue

	UE_API void OnDataSourcesReady();

	/** Internal setter that broadcasts with the specified change reason - used by ResetToDefault / RestoreToInitial. */
	UE_API void SetBoolValueWithReason(bool InValue, EGameSettingChangeReason Reason);

protected:
	TSharedPtr<FGameSettingDataSource> Getter;
	TSharedPtr<FGameSettingDataSource> Setter;

	TOptional<bool> DefaultValue;
	bool InitialValue = false;
};

#undef UE_API
