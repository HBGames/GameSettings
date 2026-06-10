// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "ViewModels/GameSettingToggleViewModel.h"

#include "GameSettingValueBool.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingToggleViewModel)

void UGameSettingToggleViewModel::SetIsChecked(bool NewValue)
{
	if (bIsChecked == NewValue)
	{
		return;
	}

	UGameSettingValueBool* BoolSetting = Cast<UGameSettingValueBool>(Setting);
	if (!BoolSetting)
	{
		return;
	}

	// Update the cached value BEFORE calling into the setting: the setting
	// fires OnSettingChangedEvent synchronously, so RefreshFromSetting runs
	// inside this call. With the cache pre-set its "have I changed?" gate
	// stays closed and no broadcast fires from inside the setter (same
	// discipline as the scalar VM's SetNormalizedValue).
	bIsChecked = NewValue;
	BoolSetting->SetBoolValue(NewValue);

	// Reconcile in case the setting refused or clamped the write; broadcast
	// only when the applied value differs from what the widget set.
	const bool Applied = BoolSetting->GetBoolValue();
	if (bIsChecked != Applied)
	{
		bIsChecked = Applied;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bIsChecked);
	}
}

void UGameSettingToggleViewModel::RefreshFromSetting()
{
	Super::RefreshFromSetting();

	UGameSettingValueBool* BoolSetting = Cast<UGameSettingValueBool>(Setting);
	if (!BoolSetting)
	{
		return;
	}

	const bool New = BoolSetting->GetBoolValue();
	if (bIsChecked != New)
	{
		bIsChecked = New;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bIsChecked);
	}
}
