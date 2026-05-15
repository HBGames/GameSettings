// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "ViewModels/GameSettingViewModel.h"

#include "GameSettingToggleViewModel.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * View model for a bool setting (a toggle). Wraps UGameSettingValueBool.
 *
 * Exposes a single IsChecked field, anchored on a UPROPERTY with
 * BlueprintGetter / BlueprintSetter / FieldNotify so MVVM resolves both the
 * read path and the write path off the same bindable field - that's what
 * Two Way bindings to a UCheckBox.IsChecked or styled toggle button need.
 *
 * Sibling of UGameSettingDiscreteViewModel, not a subclass: toggle and option
 * list are semantically different concepts in the UI even though they shared
 * a value-type ancestor in the original Lyra design.
 */
UCLASS(MinimalAPI, BlueprintType, DisplayName = "Game Setting Toggle")
class UGameSettingToggleViewModel : public UGameSettingViewModel
{
	GENERATED_BODY()
public:
	/** Write path; sends the new value to the underlying setting and re-reads in case it clamped. */
	UFUNCTION(BlueprintCallable, Category = "Value")
	UE_API void SetIsChecked(bool NewValue);

	/** Read path. */
	UFUNCTION(BlueprintCallable, Category = "Value")
	bool GetIsChecked() const { return bIsChecked; }

protected:
	//~ UGameSettingViewModel
	UE_API virtual void RefreshFromSetting() override;
	//~ End UGameSettingViewModel

private:
	/**
	 * UPROPERTY anchor for MVVM Two Way bindings. BlueprintReadWrite is required
	 * so the property carries CPF_BlueprintVisible - MVVM's IsValidForSourceBinding
	 * gates on that flag, not on the presence of BlueprintGetter/Setter. The
	 * Getter/Setter route reads and writes through the public UFUNCTIONs above;
	 * FieldNotify makes the MVVM compiler observe broadcasts. Broadcast call sites
	 * must reference the property name (bIsChecked), not GetIsChecked.
	 */
	UPROPERTY(BlueprintReadWrite, Getter="GetIsChecked", Setter="SetIsChecked", FieldNotify, BlueprintGetter=GetIsChecked, BlueprintSetter=SetIsChecked, Category="Value", meta=(AllowPrivateAccess=true))
	bool bIsChecked = false;
};

#undef UE_API
