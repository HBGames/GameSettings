// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsBinding.h"

#include "DataSource/GameSettingDataSourceFromGameUserSettings.h"
#include "DataSource/GameSettingDataSourceFromLocalPlayerSaveGame.h"
#include "DataSource/GameSettingDataSourceFromSubsystem.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/SaveGame.h"
#include "PropertyPathHelpers.h"
#include "Subsystems/Subsystem.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "GameSettingsLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsBinding)

#define LOCTEXT_NAMESPACE "GameSettings"

namespace
{
	UClass* TryLoadTargetClass(const FSoftClassPath& Path)
	{
		return Path.IsValid() ? Path.TryLoadClass<UObject>() : nullptr;
	}

	bool IsSupportedTarget(const UClass* Class)
	{
		return Class
			&& (Class->IsChildOf(USubsystem::StaticClass())
				|| Class->IsChildOf(UGameUserSettings::StaticClass())
				|| Class->IsChildOf(ULocalPlayerSaveGame::StaticClass()));
	}

	FString ResolveSaveGameSlotName(const FName SlotOverride, const UClass* SaveGameClass)
	{
		if (!SlotOverride.IsNone())
		{
			return SlotOverride.ToString();
		}
		return SaveGameClass ? SaveGameClass->GetName() : FString();
	}

	TSharedPtr<FGameSettingDataSource> CreateDataSource(
		UClass* Loaded,
		FName FunctionName,
		const FName SaveGameSlotOverride)
	{
		if (!Loaded || FunctionName.IsNone())
		{
			return nullptr;
		}

		const TArray<FString> Path{ FunctionName.ToString() };

		if (Loaded->IsChildOf(USubsystem::StaticClass()))
		{
			return MakeShared<FGameSettingDataSourceFromSubsystem>(
				TSubclassOf<USubsystem>(Loaded), Path);
		}
		if (Loaded->IsChildOf(UGameUserSettings::StaticClass()))
		{
			return MakeShared<FGameSettingDataSourceFromGameUserSettings>(
				TSubclassOf<UGameUserSettings>(Loaded), Path);
		}
		if (Loaded->IsChildOf(ULocalPlayerSaveGame::StaticClass()))
		{
			return MakeShared<FGameSettingDataSourceFromLocalPlayerSaveGame>(
				TSubclassOf<ULocalPlayerSaveGame>(Loaded),
				ResolveSaveGameSlotName(SaveGameSlotOverride, Loaded),
				Path);
		}
		return nullptr;
	}
}

bool FGameSettingsBinding::IsValid() const
{
	if (!TargetClass.IsValid() || GetterFunctionName.IsNone() || SetterFunctionName.IsNone())
	{
		return false;
	}
	UClass* Loaded = TryLoadTargetClass(TargetClass);
	if (!IsSupportedTarget(Loaded))
	{
		return false;
	}
	return Loaded->FindFunctionByName(GetterFunctionName) != nullptr
		&& Loaded->FindFunctionByName(SetterFunctionName) != nullptr;
}

TSharedPtr<FGameSettingDataSource> FGameSettingsBinding::CreateGetter() const
{
	UClass* Loaded = TryLoadTargetClass(TargetClass);
	if (!IsSupportedTarget(Loaded))
	{
		return nullptr;
	}
	return CreateDataSource(Loaded, GetterFunctionName, SaveGameSlotName);
}

TSharedPtr<FGameSettingDataSource> FGameSettingsBinding::CreateSetter() const
{
	UClass* Loaded = TryLoadTargetClass(TargetClass);
	if (!IsSupportedTarget(Loaded))
	{
		return nullptr;
	}
	return CreateDataSource(Loaded, SetterFunctionName, SaveGameSlotName);
}

bool FGameSettingsBinding::TryGetClassDefaultValueAsString(FString& OutValue) const
{
	UClass* Loaded = TryLoadTargetClass(TargetClass);
	if (!Loaded || GetterFunctionName.IsNone())
	{
		return false;
	}

	// IMPORTANT: a config class's CDO is hydrated from the saved .ini at class
	// load time, so its getter returns the LAST-SAVED value, not the factory
	// default. Reading it here would let a saved value become its own default on
	// the next launch (and "Reset to Default" would snap back to it). Only a
	// non-config CDO carries the true C++ member initializers.
	if (Loaded->HasAnyClassFlags(CLASS_Config | CLASS_PerObjectConfig))
	{
		// UGameUserSettings is config but exposes a factory-default reset. Build a
		// throwaway instance (which loads the ini), reset it to defaults, then read
		// the getter off that. It's the real default with no ini bleed, and without
		// disturbing the engine's GameUserSettings singleton. SetToDefaults only
		// assigns members. It does not save or apply hardware settings.
		if (Loaded->IsChildOf(UGameUserSettings::StaticClass()))
		{
			UGameUserSettings* Temp = NewObject<UGameUserSettings>(GetTransientPackage(), Loaded);
			Temp->SetToDefaults();
			return PropertyPathHelpers::GetPropertyValueAsString(Temp, GetterFunctionName.ToString(), OutValue);
		}

		// Any other config-backed target has no generic factory-default source, so
		// the CDO is unreliable. Refuse and let the caller fall back to the
		// authored DefaultValue instead of silently adopting the saved value.
		UE_LOG(LogGameSettings, Warning,
			TEXT("FGameSettingsBinding: target '%s' is a config class; its CDO reflects saved ini values, not factory defaults. "
				 "Set bUseClassDefaultValue=false and an explicit DefaultValue on the contribution."),
			*Loaded->GetName());
		return false;
	}

	// Non-config CDO carries the C++-declared member initializers.
	// Running the getter against it gives the value a Reset-To-Default should restore.
	UObject* CDO = Loaded->GetDefaultObject();
	if (!CDO)
	{
		return false;
	}

	return PropertyPathHelpers::GetPropertyValueAsString(CDO, GetterFunctionName.ToString(), OutValue);
}

#if WITH_EDITOR
EDataValidationResult FGameSettingsBinding::Validate(
	FDataValidationContext& Context,
	const FString& OwnerLabel,
	EGameSettingsBindingValueType ExpectedValueType) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	UClass* Loaded = TryLoadTargetClass(TargetClass);
	if (!Loaded)
	{
		Context.AddError(FText::Format(
			LOCTEXT("BindingClassMissing", "{0}: Binding TargetClass '{1}' does not load."),
			FText::FromString(OwnerLabel),
			FText::FromString(TargetClass.ToString())));
		return EDataValidationResult::Invalid;
	}

	if (!IsSupportedTarget(Loaded))
	{
		Context.AddError(FText::Format(
			LOCTEXT("BindingClassUnsupported", "{0}: Binding TargetClass '{1}' must derive from USubsystem, UGameUserSettings, or ULocalPlayerSaveGame."),
			FText::FromString(OwnerLabel),
			FText::FromString(Loaded->GetName())));
		return EDataValidationResult::Invalid;
	}

	UFunction* GetterFunc = nullptr;
	UFunction* SetterFunc = nullptr;

	if (GetterFunctionName.IsNone() || (GetterFunc = Loaded->FindFunctionByName(GetterFunctionName)) == nullptr)
	{
		Context.AddError(FText::Format(
			LOCTEXT("BindingGetterMissing", "{0}: GetterFunctionName '{1}' is not a UFUNCTION on {2}."),
			FText::FromString(OwnerLabel),
			FText::FromName(GetterFunctionName),
			FText::FromString(Loaded->GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (SetterFunctionName.IsNone() || (SetterFunc = Loaded->FindFunctionByName(SetterFunctionName)) == nullptr)
	{
		Context.AddError(FText::Format(
			LOCTEXT("BindingSetterMissing", "{0}: SetterFunctionName '{1}' is not a UFUNCTION on {2}."),
			FText::FromString(OwnerLabel),
			FText::FromName(SetterFunctionName),
			FText::FromString(Loaded->GetName())));
		Result = EDataValidationResult::Invalid;
	}

	// Type-shape check: getter must return Expected, setter must take Expected.
	if (ExpectedValueType != EGameSettingsBindingValueType::Unknown)
	{
		if (GetterFunc)
		{
			const FProperty* Ret = GetterFunc->GetReturnProperty();
			if (!UE::GameSettings::IsPropertyOfValueType(Ret, ExpectedValueType))
			{
				Context.AddError(FText::Format(
					LOCTEXT("BindingGetterReturnTypeMismatch",
						"{0}: Getter '{1}' return type '{2}' does not match the expected shape for this contribution."),
					FText::FromString(OwnerLabel),
					FText::FromName(GetterFunctionName),
					FText::FromString(Ret ? Ret->GetCPPType() : TEXT("void"))));
				Result = EDataValidationResult::Invalid;
			}
		}
		if (SetterFunc)
		{
			const FProperty* InputProp = nullptr;
			for (TFieldIterator<FProperty> It(SetterFunc); It; ++It)
			{
				if (It->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
				{
					continue;
				}
				InputProp = *It;
				break;
			}
			if (!UE::GameSettings::IsPropertyOfValueType(InputProp, ExpectedValueType))
			{
				Context.AddError(FText::Format(
					LOCTEXT("BindingSetterParamTypeMismatch",
						"{0}: Setter '{1}' parameter type '{2}' does not match the expected shape for this contribution."),
					FText::FromString(OwnerLabel),
					FText::FromName(SetterFunctionName),
					FText::FromString(InputProp ? InputProp->GetCPPType() : TEXT("(none)"))));
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
