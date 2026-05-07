// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "ViewModels/GameSettingViewModel.h"

#include "GameSetting.h"
#include "GameSettingFilterState.h"
#include "GameSettingViewModelUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingViewModel)

void UGameSettingViewModel::SetSetting(UGameSetting* InSetting)
{
	if (Setting == InSetting)
	{
		return;
	}

	if (Setting)
	{
		Setting->OnSettingChangedEvent.RemoveAll(this);
		Setting->OnSettingEditConditionChangedEvent.RemoveAll(this);
	}

	Setting = InSetting;

	if (Setting)
	{
		Setting->OnSettingChangedEvent.AddUObject(this, &UGameSettingViewModel::HandleSettingChanged);
		Setting->OnSettingEditConditionChangedEvent.AddUObject(this, &UGameSettingViewModel::HandleEditConditionsChanged);
	}

	RefreshFromSetting();
}

void UGameSettingViewModel::BeginDestroy()
{
	if (Setting)
	{
		Setting->OnSettingChangedEvent.RemoveAll(this);
		Setting->OnSettingEditConditionChangedEvent.RemoveAll(this);
	}
	Super::BeginDestroy();
}

void UGameSettingViewModel::RefreshFromSetting()
{
	if (!Setting)
	{
		return;
	}

	const FText NewDisplayName = Setting->GetDisplayName();
	if (!DisplayName.IdenticalTo(NewDisplayName))
	{
		DisplayName = NewDisplayName;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetDisplayName);
	}

	const FText NewDescription = Setting->GetDescriptionRichText();
	if (!DescriptionRichText.IdenticalTo(NewDescription))
	{
		DescriptionRichText = NewDescription;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetDescriptionRichText);
	}

	const FText NewWarning = Setting->GetWarningRichText();
	if (!WarningRichText.IdenticalTo(NewWarning))
	{
		WarningRichText = NewWarning;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetWarningRichText);
	}

	// DynamicDetails is a computed getter; a setting change is enough reason
	// to invalidate it without comparing.
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetDynamicDetails);

	RefreshEditState();
}

void UGameSettingViewModel::RefreshEditState()
{
	if (!Setting)
	{
		return;
	}

	const FGameSettingEditableState& State = Setting->GetEditState();
	const bool bNewEnabled = State.IsEnabled();
	const bool bNewVisible = State.IsVisible();
	TArray<FText> NewDisabledReasons = State.GetDisabledReasons();

	if (bIsEnabled != bNewEnabled)
	{
		bIsEnabled = bNewEnabled;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsEnabled);
	}
	if (bIsVisible != bNewVisible)
	{
		bIsVisible = bNewVisible;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsVisible);
	}
	if (!AreFTextArraysEqual(DisabledReasons, NewDisabledReasons))
	{
		DisabledReasons = MoveTemp(NewDisabledReasons);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetDisabledReasons);
	}
}

FText UGameSettingViewModel::GetDynamicDetails() const
{
	return Setting ? Setting->GetDynamicDetails() : FText::GetEmpty();
}

void UGameSettingViewModel::HandleSettingChanged(UGameSetting* InSetting, EGameSettingChangeReason /*Reason*/)
{
	RefreshFromSetting();
}

void UGameSettingViewModel::HandleEditConditionsChanged(UGameSetting* /*InSetting*/)
{
	RefreshEditState();
}
