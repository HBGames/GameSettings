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

	// Validate against our own options list before touching anything.
	// SetDiscreteOptionByIndex ensures and refuses out-of-range writes, so
	// pushing an unvalidated index through (a combo box clearing its
	// selection hands us INDEX_NONE) would leave the VM permanently
	// desynced from the model. Invalid input is simply ignored.
	if (!Options.IsValidIndex(NewIndex))
	{
		return;
	}

	// Push the write and let RefreshFromSetting broadcast the result.
	// SetDiscreteOptionByIndex fires OnSettingChangedEvent synchronously, so
	// RefreshFromSetting runs during this call, sees SelectedIndex move from its
	// old value to the applied one, and broadcasts it (plus IsAtDefault). That
	// broadcast is what a one-way consumer such as a CommonRotator bound to
	// SelectedIndex needs to update its shown item.
	//
	// We deliberately do NOT pre-set SelectedIndex first. Pre-setting closed
	// RefreshFromSetting's changed-gate and swallowed the broadcast on accepted
	// writes, leaving the rotator stuck on the old option. The early-out at the
	// top of this function stops a two-way binding from looping, and integer
	// indices round-trip exactly, so unlike the scalar slider there is no float
	// ping-pong to defend against.
	Discrete->SetDiscreteOptionByIndex(NewIndex);

	// Safety net: if the setting did not update synchronously, sync now.
	if (SelectedIndex != Discrete->GetDiscreteOptionIndex())
	{
		RefreshFromSetting();
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

	// Capture IsAtDefault's inputs up front: it depends on BOTH SelectedIndex
	// and DefaultIndex, so its broadcast can't nest inside either field's
	// individual change check (a default-only change must still notify).
	const bool bWasAtDefault = (SelectedIndex == DefaultIndex);

	TArray<FText> NewOptions = Discrete->GetDiscreteOptions();
	if (!UE::GameSettings::Private::AreFTextArraysEqual(Options, NewOptions))
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
		SelectedIndex = NewSelected;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedIndex);
	}

	// Convenience mirror of the selected option's text, for labels that bind
	// straight to it rather than deriving from Options[SelectedIndex].
	const FText NewOptionText = Options.IsValidIndex(SelectedIndex) ? Options[SelectedIndex] : FText::GetEmpty();
	if (!SelectedOptionText.EqualTo(NewOptionText))
	{
		SelectedOptionText = NewOptionText;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSelectedOptionText);
	}

	// Broadcast IsAtDefault whenever its computed value actually flipped,
	// whichever input moved it.
	const bool bIsAtDefaultNow = (SelectedIndex == DefaultIndex);
	if (bWasAtDefault != bIsAtDefaultNow)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsAtDefault);
	}
}
