// Copyright Hitbox Games, LLC. All Rights Reserved.
// Originally derived from Lyra's GameSettings plugin (Copyright Epic Games, Inc.).

#include "GameSettingValueDiscreteDynamic.h"
#include "DataSource/GameSettingDataSource.h"
#include "UObject/WeakObjectPtr.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingValueDiscreteDynamic)

#define LOCTEXT_NAMESPACE "GameSettingValueDiscreteDynamic"

//////////////////////////////////////////////////////////////////////////
// UGameSettingValueDiscreteDynamic
//////////////////////////////////////////////////////////////////////////

UGameSettingValueDiscreteDynamic::UGameSettingValueDiscreteDynamic()
{
}

void UGameSettingValueDiscreteDynamic::SetDynamicGetter(const TSharedRef<FGameSettingDataSource>& InGetter)
{
	Getter = InGetter;
}

void UGameSettingValueDiscreteDynamic::SetDynamicSetter(const TSharedRef<FGameSettingDataSource>& InSetter)
{
	Setter = InSetter;
}

void UGameSettingValueDiscreteDynamic::SetDefaultValueFromString(FString InOptionValue)
{
	DefaultValue = InOptionValue;
}

void UGameSettingValueDiscreteDynamic::AddDynamicOption(FString InOptionValue, FText InOptionText)
{
#if !UE_BUILD_SHIPPING
	ensureAlwaysMsgf(!OptionValues.Contains(InOptionValue), TEXT("You already added this option InOptionValue: %s InOptionText %s."), *InOptionValue, *InOptionText.ToString());
#endif

	OptionValues.Add(InOptionValue);
	OptionDisplayTexts.Add(InOptionText);
}

void UGameSettingValueDiscreteDynamic::RemoveDynamicOption(FString InOptionValue)
{
	const int32 Index = OptionValues.IndexOfByKey(InOptionValue);
	if (Index != INDEX_NONE)
	{
		OptionValues.RemoveAt(Index);
		OptionDisplayTexts.RemoveAt(Index);
	}
}

const TArray<FString>& UGameSettingValueDiscreteDynamic::GetDynamicOptions()
{
	return OptionValues;
}

bool UGameSettingValueDiscreteDynamic::HasDynamicOption(const FString& InOptionValue)
{
	return OptionValues.Contains(InOptionValue);
}

FString UGameSettingValueDiscreteDynamic::GetValueAsString() const
{
	return Getter->GetValueAsString(LocalPlayer);
}

void UGameSettingValueDiscreteDynamic::SetValueFromString(FString InStringValue)
{
	SetValueFromString(InStringValue, EGameSettingChangeReason::Change);
}

void UGameSettingValueDiscreteDynamic::SetValueFromString(FString InStringValue, EGameSettingChangeReason Reason)
{
	check(Setter);
	Setter->SetValue(LocalPlayer, InStringValue);

	NotifySettingChanged(Reason);
}

bool UGameSettingValueDiscreteDynamic::AreOptionsEqual(const FString& InOptionA, const FString& InOptionB) const
{
	return InOptionA == InOptionB;
}

bool UGameSettingValueDiscreteDynamic::IsResettableToDefault() const
{
	if (!DefaultValue.IsSet())
	{
		return false;
	}
	return !AreOptionsEqual(GetValueAsString(), DefaultValue.GetValue());
}

void UGameSettingValueDiscreteDynamic::OnInitialized()
{
	if (!ensureAlways(Getter) || !ensureAlways(Setter))
	{
		return;
	}

#if !UE_BUILD_SHIPPING
	ensureAlwaysMsgf(Getter->Resolve(LocalPlayer), TEXT("%s: %s did not resolve, are all functions and properties valid, and are they UFunctions/UProperties? Does the getter function have no parameters?"), *GetSettingId().ToString(), *Getter->ToString());
	ensureAlwaysMsgf(Setter->Resolve(LocalPlayer), TEXT("%s: %s did not resolve, are all functions and properties valid, and are they UFunctions/UProperties? Does the setting function have exactly one parameter?"), *GetSettingId().ToString(), *Setter->ToString());
#endif

	Super::OnInitialized();
}

void UGameSettingValueDiscreteDynamic::Startup()
{
	// Should I also do something with Setter?
	if (!ensureAlways(Getter))
	{
		// Misconfigured (no Getter bound). Complete startup anyway so the
		// registry's IsFinishedInitializing doesn't wait forever on us.
		StartupComplete();
		return;
	}
	Getter->Startup(LocalPlayer, FSimpleDelegate::CreateUObject(this, &ThisClass::OnDataSourcesReady));
}

void UGameSettingValueDiscreteDynamic::OnDataSourcesReady()
{
	StartupComplete();
}

void UGameSettingValueDiscreteDynamic::StoreInitial()
{
	InitialValue = GetValueAsString();
}

void UGameSettingValueDiscreteDynamic::ResetToDefault()
{
	if (DefaultValue.IsSet())
	{
		SetValueFromString(DefaultValue.GetValue(), EGameSettingChangeReason::ResetToDefault);
	}
}

void UGameSettingValueDiscreteDynamic::RestoreToInitial()
{
	SetValueFromString(InitialValue, EGameSettingChangeReason::RestoreToInitial);
}

void UGameSettingValueDiscreteDynamic::SetDiscreteOptionByIndex(int32 Index)
{
	// Public discrete indices are in the filtered space GetDiscreteOptions
	// exposes; map back to the unfiltered OptionValues storage.
	const int32 ValueIndex = ValueIndexFromFilteredIndex(Index);
	if (ensure(OptionValues.IsValidIndex(ValueIndex)))
	{
		SetValueFromString(OptionValues[ValueIndex]);
	}
}

int32 UGameSettingValueDiscreteDynamic::GetDiscreteOptionIndex() const
{
	const FString CurrentValue = GetValueAsString();
	const int32 ValueIndex = OptionValues.IndexOfByPredicate([this, CurrentValue](const FString& InOption) {
		return AreOptionsEqual(CurrentValue, InOption);
	});

	const int32 FilteredIndex = FilteredIndexFromValueIndex(ValueIndex);

	// If we can't find the correct index (unknown or disabled value), send the default index.
	if (FilteredIndex == INDEX_NONE)
	{
		return GetDiscreteOptionDefaultIndex();
	}

	return FilteredIndex;
}

int32 UGameSettingValueDiscreteDynamic::GetDiscreteOptionDefaultIndex() const
{
	if (DefaultValue.IsSet())
	{
		const int32 ValueIndex = OptionValues.IndexOfByPredicate([this](const FString& InOption) {
			return AreOptionsEqual(DefaultValue.GetValue(), InOption);
		});
		return FilteredIndexFromValueIndex(ValueIndex);
	}

	return INDEX_NONE;
}

int32 UGameSettingValueDiscreteDynamic::FilteredIndexFromValueIndex(int32 ValueIndex) const
{
	if (!OptionValues.IsValidIndex(ValueIndex))
	{
		return INDEX_NONE;
	}

	const TArray<FString>& DisabledOptions = GetEditState().GetDisabledOptions();
	if (DisabledOptions.Num() == 0)
	{
		return ValueIndex;
	}

	if (DisabledOptions.Contains(OptionValues[ValueIndex]))
	{
		return INDEX_NONE;
	}

	int32 FilteredIndex = 0;
	for (int32 Index = 0; Index < ValueIndex; ++Index)
	{
		if (!DisabledOptions.Contains(OptionValues[Index]))
		{
			++FilteredIndex;
		}
	}
	return FilteredIndex;
}

int32 UGameSettingValueDiscreteDynamic::ValueIndexFromFilteredIndex(int32 FilteredIndex) const
{
	if (FilteredIndex < 0)
	{
		return INDEX_NONE;
	}

	const TArray<FString>& DisabledOptions = GetEditState().GetDisabledOptions();
	if (DisabledOptions.Num() == 0)
	{
		return OptionValues.IsValidIndex(FilteredIndex) ? FilteredIndex : INDEX_NONE;
	}

	int32 Remaining = FilteredIndex;
	for (int32 Index = 0; Index < OptionValues.Num(); ++Index)
	{
		if (!DisabledOptions.Contains(OptionValues[Index]))
		{
			if (Remaining == 0)
			{
				return Index;
			}
			--Remaining;
		}
	}
	return INDEX_NONE;
}

TArray<FText> UGameSettingValueDiscreteDynamic::GetDiscreteOptions() const
{
	const TArray<FString>& DisabledOptions = GetEditState().GetDisabledOptions();

	if (DisabledOptions.Num() > 0)
	{
		TArray<FText> AllowedOptions;

		for (int32 OptionIndex = 0; OptionIndex < OptionValues.Num(); ++OptionIndex)
		{
			if (!DisabledOptions.Contains(OptionValues[OptionIndex]))
			{
				AllowedOptions.Add(OptionDisplayTexts[OptionIndex]);
			}
		}

		return AllowedOptions;
	}

	return OptionDisplayTexts;
}

//////////////////////////////////////////////////////////////////////////
// UGameSettingValueDiscreteDynamic_Number
//////////////////////////////////////////////////////////////////////////

UGameSettingValueDiscreteDynamic_Number::UGameSettingValueDiscreteDynamic_Number()
{

}

void UGameSettingValueDiscreteDynamic_Number::OnInitialized()
{
	Super::OnInitialized();

	ensure(OptionValues.Num() > 0);
}

//////////////////////////////////////////////////////////////////////////
// UGameSettingValueDiscreteDynamic_Enum
//////////////////////////////////////////////////////////////////////////

UGameSettingValueDiscreteDynamic_Enum::UGameSettingValueDiscreteDynamic_Enum()
{

}

void UGameSettingValueDiscreteDynamic_Enum::OnInitialized()
{
	Super::OnInitialized();

	ensure(OptionValues.Num() > 0);
}

//////////////////////////////////////////////////////////////////////////
// UGameSettingValueDiscreteDynamic_Color
//////////////////////////////////////////////////////////////////////////

UGameSettingValueDiscreteDynamic_Color::UGameSettingValueDiscreteDynamic_Color()
{

}


#undef LOCTEXT_NAMESPACE
