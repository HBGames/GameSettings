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
	// Two-way binding pair. SelectedIndex is a UPROPERTY anchor (below) so the
	// MVVM compiler resolves a single bindable field with both Get and Set sides.

	UFUNCTION(BlueprintCallable, Category = "Value")
	UE_API void SetSelectedIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "Value")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	/**
	 * Cycle to the next / previous option, wrapping at the ends. Widget-agnostic
	 * so a rotator, combo, gamepad shoulders, or a keybind all share one wrap
	 * policy instead of each reimplementing modulo. No-ops when there are fewer
	 * than two options. Delegates to SetSelectedIndex (which de-dups and
	 * broadcasts), so the model write and FieldNotify happen exactly as for a
	 * direct selection.
	 */
	UFUNCTION(BlueprintCallable, Category = "Value")
	UE_API void SelectNextOption();

	UFUNCTION(BlueprintCallable, Category = "Value")
	UE_API void SelectPreviousOption();

	// Read-only.

	UFUNCTION(BlueprintCallable, Category = "Value")
	const TArray<FText>& GetOptions() const { return Options; }

	UFUNCTION(BlueprintCallable, Category = "Value")
	int32 GetDefaultIndex() const { return DefaultIndex; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "State")
	bool IsAtDefault() const { return SelectedIndex == DefaultIndex; }

protected:
	UE_API virtual void RefreshFromSetting() override;

private:
	/**
	 * Two-way bindable. Getter / Setter (C++ accessors) auto-detect GetSelectedIndex / SetSelectedIndex
	 * by naming convention - that's what MVVM's binding compiler looks at when building the FieldNotify
	 * field handles.
	 */
	UPROPERTY(BlueprintReadWrite, Getter, Setter, FieldNotify, BlueprintGetter=GetSelectedIndex, BlueprintSetter=SetSelectedIndex, Category="Value", meta=(AllowPrivateAccess=true))
	int32 SelectedIndex = INDEX_NONE;

	/** Available options. One-way readable; updated when the underlying setting's options change. */
	UPROPERTY(BlueprintReadOnly, Getter, FieldNotify, BlueprintGetter=GetOptions, Category="Value", meta=(AllowPrivateAccess=true))
	TArray<FText> Options;

	/** Index of the default option. One-way readable. */
	UPROPERTY(BlueprintReadOnly, Getter, FieldNotify, BlueprintGetter=GetDefaultIndex, Category="Value", meta=(AllowPrivateAccess=true))
	int32 DefaultIndex = INDEX_NONE;
};

#undef UE_API
