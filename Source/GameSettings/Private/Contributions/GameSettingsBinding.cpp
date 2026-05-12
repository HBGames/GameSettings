// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsBinding.h"

#include "DataSource/GameSettingDataSourceFromGameUserSettings.h"
#include "DataSource/GameSettingDataSourceFromLocalPlayerSaveGame.h"
#include "DataSource/GameSettingDataSourceFromSubsystem.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/Subsystem.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

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
