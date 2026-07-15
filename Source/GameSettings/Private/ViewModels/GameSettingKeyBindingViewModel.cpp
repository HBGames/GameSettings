// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "ViewModels/GameSettingKeyBindingViewModel.h"

#include "GameSettingValueKeyBinding.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingKeyBindingViewModel)

UGameSettingValueKeyBinding* UGameSettingKeyBindingViewModel::GetKeyBinding() const
{
	return Cast<UGameSettingValueKeyBinding>(Setting);
}

FText UGameSettingKeyBindingViewModel::GetKeyTextForSlot(int32 Slot) const
{
	if (UGameSettingValueKeyBinding* KeyBinding = GetKeyBinding())
	{
		return KeyBinding->GetKeyTextForSlot((EPlayerMappableKeySlot)(static_cast<uint8>(Slot)));
	}
	return FText::GetEmpty();
}

bool UGameSettingKeyBindingViewModel::ChangeBinding(int32 Slot, FKey NewKey)
{
	UGameSettingValueKeyBinding* KeyBinding = GetKeyBinding();
	if (!KeyBinding)
	{
		return false;
	}

	// The setting fires OnSettingChangedEvent synchronously, which runs
	// RefreshFromSetting through the base VM and rebroadcasts the slot text.
	return KeyBinding->ChangeBinding(Slot, NewKey);
}

void UGameSettingKeyBindingViewModel::ResetBindingToDefault()
{
	if (UGameSettingValueKeyBinding* KeyBinding = GetKeyBinding())
	{
		KeyBinding->ResetToDefault();
	}
}

TArray<FName> UGameSettingKeyBindingViewModel::GetActionsBoundToKey(int32 Slot, FKey Key) const
{
	TArray<FName> OutActionNames;
	if (UGameSettingValueKeyBinding* KeyBinding = GetKeyBinding())
	{
		KeyBinding->GetMappedActionNamesForKey(Slot, Key, OutActionNames);
	}
	return OutActionNames;
}

void UGameSettingKeyBindingViewModel::RefreshFromSetting()
{
	Super::RefreshFromSetting();

	UGameSettingValueKeyBinding* KeyBinding = GetKeyBinding();
	if (!KeyBinding)
	{
		return;
	}

	const FText NewPrimary = KeyBinding->GetKeyTextForSlot((EPlayerMappableKeySlot)(static_cast<uint8>(PrimarySlot)));
	if (!PrimaryKeyText.IdenticalTo(NewPrimary))
	{
		PrimaryKeyText = NewPrimary;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetPrimaryKeyText);
	}

	const FText NewSecondary = KeyBinding->GetKeyTextForSlot((EPlayerMappableKeySlot)(static_cast<uint8>(SecondarySlot)));
	if (!SecondaryKeyText.IdenticalTo(NewSecondary))
	{
		SecondaryKeyText = NewSecondary;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSecondaryKeyText);
	}

	const bool bNewCustomized = KeyBinding->IsMappingCustomized();
	if (bIsCustomized != bNewCustomized)
	{
		bIsCustomized = bNewCustomized;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsCustomized);
	}
}
