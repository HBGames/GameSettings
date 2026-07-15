// Copyright Hitbox Games, LLC. All Rights Reserved.
// Originally derived from Lyra's ULyraSettingKeyboardInput (Copyright Epic Games, Inc.).

#pragma once

#include "EnhancedActionKeyMapping.h"
#include "GameSettingValue.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include "GameSettingValueKeyBinding.generated.h"

#define UE_API GAMESETTINGS_API

class FGameSettingDataSource;

//--------------------------------------
// UGameSettingValueKeyBinding
//--------------------------------------

/**
 * A settings value that rebinds one Enhanced Input player mappable key.
 *
 * Device-agnostic on purpose. A mappable key is just an FKey, so the same
 * setting rebinds keyboard, mouse, gamepad, and VR motion-controller buttons.
 * The only per-device policy is which keys the capture UI offers and which
 * slots this setting reads, both carried by QueryOptions. Nothing here rejects
 * a key by device. Reworked from Lyra's ULyraSettingKeyboardInput, which was
 * keyboard-and-mouse only and refused gamepad keys in ChangeBinding.
 *
 * Persistence rides the Enhanced Input user settings channel. GetPersistableDataSource
 * returns a data source whose Persist saves those user settings, and every key
 * binding row shares one persist key so the registry saves them once on Apply.
 */
UCLASS(MinimalAPI, BlueprintType)
class UGameSettingValueKeyBinding : public UGameSettingValue
{
	GENERATED_BODY()

public:
	UE_API UGameSettingValueKeyBinding();

	/**
	 * Bind this setting to one mapping row from a key profile. Records the
	 * mapping name, the profile id, and the current key in every slot that
	 * passes InQueryOptions. Call before adding the setting to the registry so
	 * the display name is set by the time Initialize runs.
	 */
	UE_API void InitializeInputData(const UEnhancedPlayerMappableKeyProfile* KeyProfile, const FKeyMappingRow& MappingData, const FPlayerMappableKeyQueryOptions& InQueryOptions);

	/** Display name pulled from the first mapping, used by the auto-contributor. */
	UE_API FText GetSettingDisplayName() const;

	/** Display category pulled from the first mapping, used to group rows into sections. */
	UE_API FText GetSettingDisplayCategory() const;

	/** Current key bound in a given slot, or the invalid-key display name if none. */
	UE_API FText GetKeyTextForSlot(EPlayerMappableKeySlot InSlot) const;

	/** The action this setting rebinds. */
	FName GetActionMappingName() const { return ActionMappingName; }

	//~UGameSettingValue interface
	UE_API virtual void StoreInitial() override;
	UE_API virtual void ResetToDefault() override;
	UE_API virtual void RestoreToInitial() override;
	UE_API virtual bool IsResettableToDefault() const override;
	UE_API virtual TSharedPtr<FGameSettingDataSource> GetPersistableDataSource() const override;
	//~End of UGameSettingValue interface

	/**
	 * Rebind one slot to NewKey. Accepts any FKey, including gamepad and VR
	 * controller buttons. Returns false only when the user settings are not
	 * reachable yet. Caller side decides whether a key is valid for this
	 * binding (device policy lives in the UI, not here).
	 */
	UE_API bool ChangeBinding(int32 InKeyBindSlot, FKey NewKey);

	/** Names of every action already mapped to Key, for duplicate-binding warnings. */
	UE_API void GetMappedActionNamesForKey(int32 InKeyBindSlot, FKey Key, TArray<FName>& OutActionNames) const;

	/** True if any slot on this row differs from its default mapping. */
	UE_API bool IsMappingCustomized() const;

	UE_API const FKeyMappingRow* FindKeyMappingRow() const;
	UE_API UEnhancedPlayerMappableKeyProfile* FindMappableKeyProfile() const;
	UE_API UEnhancedInputUserSettings* GetUserSettings() const;

protected:
	//~UGameSetting interface
	UE_API virtual void OnInitialized() override;
	//~End of UGameSetting interface

	/** The name of this action's mappings. */
	FName ActionMappingName;

	/** The query options that filter which keys and slots this setting cares about. */
	FPlayerMappableKeyQueryOptions QueryOptions;

	/** The profile identifier that this key setting is from. */
	FString ProfileIdentifier;

	/** The key that was mapped in each slot when the screen opened, for RestoreToInitial. */
	TMap<EPlayerMappableKeySlot, FKey> InitialKeyMappings;

	/** Shared persist hook that flushes the Enhanced Input user settings on Apply. */
	TSharedPtr<FGameSettingDataSource> PersistDataSource;
};

#undef UE_API
