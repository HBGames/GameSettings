// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsBinding.h"

#include "DataSource/GameSettingDataSourceFromSubsystem.h"
#include "Subsystems/Subsystem.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsBinding)

#define LOCTEXT_NAMESPACE "GameSettings"

namespace
{
	UClass* TryLoadSubsystemClass(const FSoftClassPath& Path)
	{
		return Path.IsValid() ? Path.TryLoadClass<USubsystem>() : nullptr;
	}
}

bool FGameSettingsBinding::IsValid() const
{
	if (!SubsystemClass.IsValid() || GetterFunctionName.IsNone() || SetterFunctionName.IsNone())
	{
		return false;
	}
	UClass* Loaded = TryLoadSubsystemClass(SubsystemClass);
	if (!Loaded)
	{
		return false;
	}
	return Loaded->FindFunctionByName(GetterFunctionName) != nullptr
		&& Loaded->FindFunctionByName(SetterFunctionName) != nullptr;
}

TSharedPtr<FGameSettingDataSource> FGameSettingsBinding::CreateGetter() const
{
	UClass* Loaded = TryLoadSubsystemClass(SubsystemClass);
	if (!Loaded || GetterFunctionName.IsNone())
	{
		return nullptr;
	}
	return MakeShared<FGameSettingDataSourceFromSubsystem>(
		TSubclassOf<USubsystem>(Loaded),
		TArray<FString>({ GetterFunctionName.ToString() }));
}

TSharedPtr<FGameSettingDataSource> FGameSettingsBinding::CreateSetter() const
{
	UClass* Loaded = TryLoadSubsystemClass(SubsystemClass);
	if (!Loaded || SetterFunctionName.IsNone())
	{
		return nullptr;
	}
	return MakeShared<FGameSettingDataSourceFromSubsystem>(
		TSubclassOf<USubsystem>(Loaded),
		TArray<FString>({ SetterFunctionName.ToString() }));
}

#if WITH_EDITOR
EDataValidationResult FGameSettingsBinding::Validate(FDataValidationContext& Context, const FString& OwnerLabel) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	UClass* Loaded = TryLoadSubsystemClass(SubsystemClass);
	if (!Loaded)
	{
		Context.AddError(FText::Format(
			LOCTEXT("BindingClassMissing", "{0}: Binding SubsystemClass '{1}' does not load."),
			FText::FromString(OwnerLabel),
			FText::FromString(SubsystemClass.ToString())));
		return EDataValidationResult::Invalid;
	}

	if (GetterFunctionName.IsNone() || !Loaded->FindFunctionByName(GetterFunctionName))
	{
		Context.AddError(FText::Format(
			LOCTEXT("BindingGetterMissing", "{0}: GetterFunctionName '{1}' is not a UFUNCTION on {2}."),
			FText::FromString(OwnerLabel),
			FText::FromName(GetterFunctionName),
			FText::FromString(Loaded->GetName())));
		Result = EDataValidationResult::Invalid;
	}

	if (SetterFunctionName.IsNone() || !Loaded->FindFunctionByName(SetterFunctionName))
	{
		Context.AddError(FText::Format(
			LOCTEXT("BindingSetterMissing", "{0}: SetterFunctionName '{1}' is not a UFUNCTION on {2}."),
			FText::FromString(OwnerLabel),
			FText::FromName(SetterFunctionName),
			FText::FromString(Loaded->GetName())));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
