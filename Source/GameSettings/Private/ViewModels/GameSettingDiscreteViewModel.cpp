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

	const bool bWasAtDefault = (SelectedIndex == DefaultIndex);

	// Update the cached value BEFORE calling into the setting: the setting
	// fires OnSettingChangedEvent synchronously, so RefreshFromSetting runs
	// inside this call. With the cache pre-set its "have I changed?" gate
	// stays closed and the two-way binding doesn't loop back into here
	// (same discipline as the scalar VM's SetNormalizedValue).
	SelectedIndex = NewIndex;
	Discrete->SetDiscreteOptionByIndex(NewIndex);

	// Reconcile in case the setting refused or remapped the write. Broadcast
	// only when the applied value differs from what the widget set -
	// re-broadcasting the same value would loop the two-way binding.
	const int32 Applied = Discrete->GetDiscreteOptionIndex();
	if (SelectedIndex != Applied)
	{
		SelectedIndex = Applied;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedIndex);
	}

	const bool bIsAtDefaultNow = (SelectedIndex == DefaultIndex);
	if (bWasAtDefault != bIsAtDefaultNow)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsAtDefault);
	}

	// SelectedOptionText is one-way (no destination-side feedback), so always
	// reconcile it. When the setting accepts the write as-is, SelectedIndex's
	// broadcast is suppressed above, so this is what refreshes a value label
	// bound to the current option (mirrors the scalar VM broadcasting FormattedText).
	RefreshSelectedOptionText();
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

	RefreshSelectedOptionText();

	// Broadcast IsAtDefault whenever its computed value actually flipped,
	// whichever input moved it.
	const bool bIsAtDefaultNow = (SelectedIndex == DefaultIndex);
	if (bWasAtDefault != bIsAtDefaultNow)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsAtDefault);
	}
}

void UGameSettingDiscreteViewModel::RefreshSelectedOptionText()
{
	const FText NewOptionText = Options.IsValidIndex(SelectedIndex) ? Options[SelectedIndex] : FText::GetEmpty();
	if (!SelectedOptionText.EqualTo(NewOptionText))
	{
		SelectedOptionText = NewOptionText;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSelectedOptionText);
	}
}
