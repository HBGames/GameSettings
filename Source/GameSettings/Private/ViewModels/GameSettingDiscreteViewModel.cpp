// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "ViewModels/GameSettingDiscreteViewModel.h"

#include "GameSettingValueDiscrete.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingDiscreteViewModel)

namespace
{
	bool AreFTextArraysEqual(const TArray<FText>& A, const TArray<FText>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (!A[Index].EqualTo(B[Index]))
			{
				return false;
			}
		}
		return true;
	}
}

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
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSelectedIndex);

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
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetOptions);
	}

	const int32 NewDefault = Discrete->GetDiscreteOptionDefaultIndex();
	if (DefaultIndex != NewDefault)
	{
		DefaultIndex = NewDefault;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetDefaultIndex);
	}

	const int32 NewSelected = Discrete->GetDiscreteOptionIndex();
	if (SelectedIndex != NewSelected)
	{
		const bool bWasAtDefault = (SelectedIndex == DefaultIndex);
		SelectedIndex = NewSelected;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSelectedIndex);

		const bool bIsAtDefaultNow = (SelectedIndex == DefaultIndex);
		if (bWasAtDefault != bIsAtDefaultNow)
		{
			UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsAtDefault);
		}
	}
}
