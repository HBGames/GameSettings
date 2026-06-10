// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "MVVMViewModelBase.h"

#include "GameSettingViewModel.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSetting;
enum class EGameSettingChangeReason : uint8;

/**
 * View model for any UGameSetting. Mirrors the setting's display fields
 * and edit state as FieldNotify so widgets can bind without per-widget
 * event wiring.
 *
 * Owned by UGameSettingsScreenViewModel. Construct with NewObject, then
 * call SetSetting() once. The VM subscribes to the setting's change /
 * edit-condition events and unsubscribes on BeginDestroy.
 */
UCLASS(MinimalAPI, BlueprintType, DisplayName = "Game Setting")
class UGameSettingViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Bind this view model to a setting. Called once during construction by the screen VM. */
	UE_API void SetSetting(UGameSetting* InSetting);

	/** The underlying setting. May be null if not yet set or destroyed. */
	UGameSetting* GetSetting() const { return Setting; }

	// FieldNotify getters. Storage is private; setters are internal.

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Display")
	const FText& GetDisplayName() const { return DisplayName; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Display")
	const FText& GetDescriptionRichText() const { return DescriptionRichText; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Display")
	const FText& GetWarningRichText() const { return WarningRichText; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Display")
	FText GetDynamicDetails() const;

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "State")
	bool IsEnabled() const { return bIsEnabled; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "State")
	bool IsVisible() const { return bIsVisible; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "State")
	const TArray<FText>& GetDisabledReasons() const { return DisabledReasons; }

	UE_API virtual void BeginDestroy() override;

protected:
	/** Re-read everything from the setting and broadcast on whatever changed. */
	UE_API virtual void RefreshFromSetting();

	/** Re-read just the edit state. Called when conditions change. */
	UE_API void RefreshEditState();

	UPROPERTY(Transient)
	TObjectPtr<UGameSetting> Setting;

private:
	void HandleSettingChanged(UGameSetting* InSetting, EGameSettingChangeReason Reason);
	void HandleEditConditionsChanged(UGameSetting* InSetting);

	FText DisplayName;
	FText DescriptionRichText;
	FText WarningRichText;
	bool bIsEnabled = true;
	bool bIsVisible = true;
	TArray<FText> DisabledReasons;
};

#undef UE_API
