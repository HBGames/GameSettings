// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "ViewModels/GameSettingViewModel.h"

#include "GameSettingDiscreteViewModel.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * View model for an option-list (discrete) setting. Wraps
 * UGameSettingValueDiscrete. Two-way binding on SelectedIndex; widget
 * writes propagate back to the setting via SetDiscreteOptionByIndex.
 */
UCLASS(MinimalAPI, BlueprintType, DisplayName = "Game Setting Discrete")
class UGameSettingDiscreteViewModel : public UGameSettingViewModel
{
	GENERATED_BODY()
public:
	// Two-way binding pair.

	UFUNCTION(BlueprintCallable, Category = "Value")
	UE_API void SetSelectedIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Value")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	// Read-only.

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Value")
	const TArray<FText>& GetOptions() const { return Options; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Value")
	int32 GetDefaultIndex() const { return DefaultIndex; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "State")
	bool IsAtDefault() const { return SelectedIndex == DefaultIndex; }

protected:
	UE_API virtual void RefreshFromSetting() override;

private:
	TArray<FText> Options;
	int32 SelectedIndex = INDEX_NONE;
	int32 DefaultIndex = INDEX_NONE;
};

#undef UE_API
