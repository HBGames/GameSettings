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

void UGameSettingDiscreteViewModel::SelectNextOption()
{
	const int32 Count = Options.Num();
	if (Count < 2)
	{
		return;
	}
	// SelectedIndex can be INDEX_NONE before the first RefreshFromSetting;
	// treat that as "before 0" so the first Next lands on 0.
	const int32 Current = (SelectedIndex >= 0 && SelectedIndex < Count) ? SelectedIndex : Count - 1;
	SetSelectedIndex((Current + 1) % Count);
}

void UGameSettingDiscreteViewModel::SelectPreviousOption()
{
	const int32 Count = Options.Num();
	if (Count < 2)
	{
		return;
	}
	const int32 Current = (SelectedIndex >= 0 && SelectedIndex < Count) ? SelectedIndex : 0;
	SetSelectedIndex((Current - 1 + Count) % Count);
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
