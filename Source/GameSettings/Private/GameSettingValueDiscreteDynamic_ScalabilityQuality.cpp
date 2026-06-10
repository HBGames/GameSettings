// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingValueDiscreteDynamic_ScalabilityQuality.h"

#include "DataSource/GameSettingDataSourceFromGameUserSettings.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingValueDiscreteDynamic_ScalabilityQuality)

#define LOCTEXT_NAMESPACE "GameSettings"

UGameSettingValueDiscreteDynamic_ScalabilityQuality::UGameSettingValueDiscreteDynamic_ScalabilityQuality()
{
	// Persist-only source: SaveChanges uses GetPersistKey/Persist (never the
	// value path), and the "GameUserSettings" key collapses this into the same
	// single ApplySettings flush as every other GameUserSettings-bound setting.
	PersistDataSource = MakeShared<FGameSettingDataSourceFromGameUserSettings>(
		TSubclassOf<UGameUserSettings>(UGameUserSettings::StaticClass()),
		TArray<FString>({ TEXT("GetOverallScalabilityLevel") }));
}

void UGameSettingValueDiscreteDynamic_ScalabilityQuality::Startup()
{
	// No Getter/Setter data sources to wait on (we talk to GameUserSettings
	// directly), so skip the base-class Startup that requires a Getter.
	StartupComplete();
}

int32 UGameSettingValueDiscreteDynamic_ScalabilityQuality::GetMaxSupportedQualityLevel() const
{
	// No cap by default; project subclasses can clamp for low-end platforms.
	return -1;
}

void UGameSettingValueDiscreteDynamic_ScalabilityQuality::OnInitialized()
{
	// Deliberately skip UGameSettingValueDiscreteDynamic::OnInitialized - it
	// requires Getter/Setter data sources, which this class never has.
	UGameSettingValue::OnInitialized();

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
	// The engine can report a quality level above our (possibly capped) preset
	// list; clamp so the index stays inside the options we actually offer.
	return FMath::Min(Level, Options.Num() - 1);
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
