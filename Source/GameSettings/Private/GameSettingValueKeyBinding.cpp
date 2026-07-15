// Copyright Hitbox Games, LLC. All Rights Reserved.
// Originally derived from Lyra's ULyraSettingKeyboardInput (Copyright Epic Games, Inc.).

#include "GameSettingValueKeyBinding.h"

#include "DataSource/GameSettingDataSourceEnhancedInput.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingValueKeyBinding)

#define LOCTEXT_NAMESPACE "GameSettings"

namespace UE::GameSettings::KeyBinding
{
	static const FText UnknownMappingName = LOCTEXT("KeyBinding_UnknownMappingName", "Unknown Mapping");
}

UGameSettingValueKeyBinding::UGameSettingValueKeyBinding()
{
	bReportAnalytics = false;
}

FText UGameSettingValueKeyBinding::GetSettingDisplayName() const
{
	if (const FKeyMappingRow* Row = FindKeyMappingRow())
	{
		if (Row->HasAnyMappings())
		{
			return Row->Mappings.begin()->GetDisplayName();
		}
	}

	return UE::GameSettings::KeyBinding::UnknownMappingName;
}

FText UGameSettingValueKeyBinding::GetSettingDisplayCategory() const
{
	if (const FKeyMappingRow* Row = FindKeyMappingRow())
	{
		if (Row->HasAnyMappings())
		{
			return Row->Mappings.begin()->GetDisplayCategory();
		}
	}

	return UE::GameSettings::KeyBinding::UnknownMappingName;
}

const FKeyMappingRow* UGameSettingValueKeyBinding::FindKeyMappingRow() const
{
	if (const UEnhancedPlayerMappableKeyProfile* Profile = FindMappableKeyProfile())
	{
		return Profile->FindKeyMappingRow(ActionMappingName);
	}

	// Quietly absent before a profile exists. A key binding row can be queried
	// during teardown or before input mapping contexts register their keys.
	return nullptr;
}

UEnhancedPlayerMappableKeyProfile* UGameSettingValueKeyBinding::FindMappableKeyProfile() const
{
	if (UEnhancedInputUserSettings* Settings = GetUserSettings())
	{
		return Settings->GetKeyProfileWithId(ProfileIdentifier);
	}

	return nullptr;
}

UEnhancedInputUserSettings* UGameSettingValueKeyBinding::GetUserSettings() const
{
	// Any local player with an Enhanced Input subsystem works. No project type.
	if (LocalPlayer)
	{
		if (UEnhancedInputLocalPlayerSubsystem* System = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			return System->GetUserSettings();
		}
	}

	return nullptr;
}

void UGameSettingValueKeyBinding::OnInitialized()
{
	DynamicDetails = FGetGameSettingsDetails::CreateWeakLambda(this, [this](ULocalPlayer&)
	{
		if (const FKeyMappingRow* Row = FindKeyMappingRow())
		{
			if (Row->HasAnyMappings())
			{
				return FText::Format(LOCTEXT("DynamicDetails_KeyBindingAction", "Bindings for {0}"), Row->Mappings.begin()->GetDisplayName());
			}
		}
		return FText::GetEmpty();
	});

	// Shared persist hook. All key binding rows return one of these with the
	// same persist key, so a screen of rebinds saves the user settings once.
	PersistDataSource = MakeShared<FGameSettingDataSourceEnhancedInput>();

	Super::OnInitialized();
}

void UGameSettingValueKeyBinding::InitializeInputData(const UEnhancedPlayerMappableKeyProfile* KeyProfile, const FKeyMappingRow& MappingData, const FPlayerMappableKeyQueryOptions& InQueryOptions)
{
	check(KeyProfile);

	ProfileIdentifier = KeyProfile->GetProfileIdString();
	QueryOptions = InQueryOptions;

	for (const FPlayerKeyMapping& Mapping : MappingData.Mappings)
	{
		// Only record mappings that pass the query filter this row was built for.
		if (!KeyProfile->DoesMappingPassQueryOptions(Mapping, QueryOptions))
		{
			continue;
		}

		ActionMappingName = Mapping.GetMappingName();
		InitialKeyMappings.Add(Mapping.GetSlot(), Mapping.GetCurrentKey());

		const FText& MappingDisplayName = Mapping.GetDisplayName();
		if (!MappingDisplayName.IsEmpty())
		{
			SetDisplayName(MappingDisplayName);
		}
	}
}

FText UGameSettingValueKeyBinding::GetKeyTextForSlot(EPlayerMappableKeySlot InSlot) const
{
	if (const UEnhancedPlayerMappableKeyProfile* Profile = FindMappableKeyProfile())
	{
		FPlayerMappableKeyQueryOptions QueryOptionsForSlot = QueryOptions;
		QueryOptionsForSlot.SlotToMatch = InSlot;

		if (const FKeyMappingRow* Row = FindKeyMappingRow())
		{
			for (const FPlayerKeyMapping& Mapping : Row->Mappings)
			{
				if (Profile->DoesMappingPassQueryOptions(Mapping, QueryOptionsForSlot))
				{
					return Mapping.GetCurrentKey().GetDisplayName();
				}
			}
		}
	}

	return EKeys::Invalid.GetDisplayName();
}

void UGameSettingValueKeyBinding::ResetToDefault()
{
	if (UEnhancedInputUserSettings* Settings = GetUserSettings())
	{
		FMapPlayerKeyArgs Args = {};
		Args.MappingName = ActionMappingName;

		FGameplayTagContainer FailureReason;
		Settings->ResetAllPlayerKeysInRow(Args, FailureReason);

		NotifySettingChanged(EGameSettingChangeReason::ResetToDefault);
	}
}

void UGameSettingValueKeyBinding::StoreInitial()
{
	if (const UEnhancedPlayerMappableKeyProfile* Profile = FindMappableKeyProfile())
	{
		if (const FKeyMappingRow* Row = FindKeyMappingRow())
		{
			for (const FPlayerKeyMapping& Mapping : Row->Mappings)
			{
				if (Profile->DoesMappingPassQueryOptions(Mapping, QueryOptions))
				{
					ActionMappingName = Mapping.GetMappingName();
					InitialKeyMappings.Add(Mapping.GetSlot(), Mapping.GetCurrentKey());
				}
			}
		}
	}
}

void UGameSettingValueKeyBinding::RestoreToInitial()
{
	for (const TPair<EPlayerMappableKeySlot, FKey>& Pair : InitialKeyMappings)
	{
		ChangeBinding((int32)Pair.Key, Pair.Value);
	}
}

bool UGameSettingValueKeyBinding::ChangeBinding(int32 InKeyBindSlot, FKey NewKey)
{
	// Accept any key. Gamepad and VR controller buttons are valid mappable keys.
	// Device policy, if a screen wants one, belongs in the capture UI.
	UEnhancedInputUserSettings* Settings = GetUserSettings();
	if (!Settings)
	{
		return false;
	}

	FMapPlayerKeyArgs Args = {};
	Args.MappingName = ActionMappingName;
	Args.Slot = (EPlayerMappableKeySlot)(static_cast<uint8>(InKeyBindSlot));
	Args.NewKey = NewKey;

	FGameplayTagContainer FailureReason;
	Settings->MapPlayerKey(Args, FailureReason);
	NotifySettingChanged(EGameSettingChangeReason::Change);

	return true;
}

void UGameSettingValueKeyBinding::GetMappedActionNamesForKey(int32 /*InKeyBindSlot*/, FKey Key, TArray<FName>& OutActionNames) const
{
	if (const UEnhancedPlayerMappableKeyProfile* Profile = FindMappableKeyProfile())
	{
		Profile->GetMappingNamesForKey(Key, OutActionNames);
	}
}

bool UGameSettingValueKeyBinding::IsMappingCustomized() const
{
	bool bResult = false;

	if (const UEnhancedPlayerMappableKeyProfile* Profile = FindMappableKeyProfile())
	{
		if (const FKeyMappingRow* Row = FindKeyMappingRow())
		{
			for (const FPlayerKeyMapping& Mapping : Row->Mappings)
			{
				if (Profile->DoesMappingPassQueryOptions(Mapping, QueryOptions))
				{
					bResult |= Mapping.IsCustomized();
				}
			}
		}
	}

	return bResult;
}

bool UGameSettingValueKeyBinding::IsResettableToDefault() const
{
	return IsMappingCustomized();
}

TSharedPtr<FGameSettingDataSource> UGameSettingValueKeyBinding::GetPersistableDataSource() const
{
	return PersistDataSource;
}

#undef LOCTEXT_NAMESPACE
