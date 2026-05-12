// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingValueDiscreteDynamic_ScalabilityQuality.h"

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingValueDiscreteDynamic_ScalabilityQuality)

#define LOCTEXT_NAMESPACE "GameSettings"

UGameSettingValueDiscreteDynamic_ScalabilityQuality::UGameSettingValueDiscreteDynamic_ScalabilityQuality()
{
}

int32 UGameSettingValueDiscreteDynamic_ScalabilityQuality::GetMaxSupportedQualityLevel() const
{
	// No cap by default; project subclasses can clamp for low-end platforms.
	return -1;
}

void UGameSettingValueDiscreteDynamic_ScalabilityQuality::OnInitialized()
{
	Super::OnInitialized();

	const int32 MaxQualityLevel = GetMaxSupportedQualityLevel();
	auto AddOptionIfPossible = [&](int32 Index, FText&& Value)
		{
			if (MaxQualityLevel < 0 || Index <= MaxQualityLevel)
			{
				Options.Add(MoveTemp(Value));
			}
		};

	AddOptionIfPossible(0, LOCTEXT("VideoQualityOverall_Low",    "Low"));
	AddOptionIfPossible(1, LOCTEXT("VideoQualityOverall_Medium", "Medium"));
	AddOptionIfPossible(2, LOCTEXT("VideoQualityOverall_High",   "High"));
	AddOptionIfPossible(3, LOCTEXT("VideoQualityOverall_Epic",   "Epic"));

	OptionsWithCustom = Options;
	OptionsWithCustom.Add(LOCTEXT("VideoQualityOverall_Custom", "Custom"));
}

void UGameSettingValueDiscreteDynamic_ScalabilityQuality::StoreInitial()
{
}

void UGameSettingValueDiscreteDynamic_ScalabilityQuality::ResetToDefault()
{
}

void UGameSettingValueDiscreteDynamic_ScalabilityQuality::RestoreToInitial()
{
}

void UGameSettingValueDiscreteDynamic_ScalabilityQuality::SetDiscreteOptionByIndex(int32 Index)
{
	UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!UserSettings)
	{
		return;
	}

	if (Index == GetCustomOptionIndex())
	{
		// Custom selected: keep the per-feature scalability values exactly as they are.
	}
	else
	{
		UserSettings->SetOverallScalabilityLevel(Index);
	}

	NotifySettingChanged(EGameSettingChangeReason::Change);
}

int32 UGameSettingValueDiscreteDynamic_ScalabilityQuality::GetDiscreteOptionIndex() const
{
	const int32 Level = GetCurrentScalabilityLevel();
	if (Level == INDEX_NONE)
	{
		return GetCustomOptionIndex();
	}
	return Level;
}

TArray<FText> UGameSettingValueDiscreteDynamic_ScalabilityQuality::GetDiscreteOptions() const
{
	const int32 Level = GetCurrentScalabilityLevel();
	return (Level == INDEX_NONE) ? OptionsWithCustom : Options;
}

int32 UGameSettingValueDiscreteDynamic_ScalabilityQuality::GetCustomOptionIndex() const
{
	return OptionsWithCustom.Num() - 1;
}

int32 UGameSettingValueDiscreteDynamic_ScalabilityQuality::GetCurrentScalabilityLevel() const
{
	const UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	return UserSettings ? UserSettings->GetOverallScalabilityLevel() : 3;
}

#undef LOCTEXT_NAMESPACE
