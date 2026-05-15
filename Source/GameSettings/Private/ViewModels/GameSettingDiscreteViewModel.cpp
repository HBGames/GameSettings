// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "ViewModels/GameSettingDiscreteViewModel.h"

#include "GameSettingValueDiscrete.h"
#include "GameSettingViewModelUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingDiscreteViewModel)

void UGameSettingDiscreteViewModel::SetSelectedIndex(int32 NewIndex)
{
	if (SelectedIndex == NewIndex)
	{
		return;
	}

	UGameSettingValueDiscrete* Discrete = Cast<UGameSettingValueDiscrete>(Setting);
	if (!Discrete)
	{
		return;
	}

	const bool bWasAtDefault = (SelectedIndex == DefaultIndex);

	SelectedIndex = NewIndex;
	Discrete->SetDiscreteOptionByIndex(NewIndex);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedIndex);

	const bool bIsAtDefaultNow = (SelectedIndex == DefaultIndex);
	if (bWasAtDefault != bIsAtDefaultNow)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsAtDefault);
	}
}

void UGameSettingDiscreteViewModel::RefreshFromSetting()
{
	Super::RefreshFromSetting();

	UGameSettingValueDiscrete* Discrete = Cast<UGameSettingValueDiscrete>(Setting);
	if (!Discrete)
	{
		return;
	}

	TArray<FText> NewOptions = Discrete->GetDiscreteOptions();
	if (!AreFTextArraysEqual(Options, NewOptions))
	{
		Options = MoveTemp(NewOptions);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Options);
	}

	const int32 NewDefault = Discrete->GetDiscreteOptionDefaultIndex();
	if (DefaultIndex != NewDefault)
	{
		DefaultIndex = NewDefault;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DefaultIndex);
	}

	const int32 NewSelected = Discrete->GetDiscreteOptionIndex();
	if (SelectedIndex != NewSelected)
	{
		const bool bWasAtDefault = (SelectedIndex == DefaultIndex);
		SelectedIndex = NewSelected;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedIndex);

		const bool bIsAtDefaultNow = (SelectedIndex == DefaultIndex);
		if (bWasAtDefault != bIsAtDefaultNow)
		{
			UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsAtDefault);
		}
	}
}
