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

	BoolSetting->SetBoolValue(NewValue);

	// Re-read in case the setting refused or clamped the write.
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
