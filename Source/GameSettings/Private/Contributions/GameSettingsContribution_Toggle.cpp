// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsContribution_Toggle.h"

#include "GameSettingRegistry.h"
#include "GameSettingValueDiscreteDynamic.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsContribution_Toggle)

#define LOCTEXT_NAMESPACE "GameSettings"

void UGameSettingsContribution_Toggle::Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles)
{
	if (!SettingId.IsValid() || DisplayName.IsEmpty() || !Binding.IsValid())
	{
		return;
	}

	UGameSettingValueDiscreteDynamic_Bool* Setting = NewObject<UGameSettingValueDiscreteDynamic_Bool>(&Registry);
	ConfigureBaseSetting(*Setting);

	if (TSharedPtr<FGameSettingDataSource> Getter = Binding.CreateGetter())
	{
		Setting->SetDynamicGetter(Getter.ToSharedRef());
	}
	if (TSharedPtr<FGameSettingDataSource> Setter = Binding.CreateSetter())
	{
		Setting->SetDynamicSetter(Setter.ToSharedRef());
	}
	Setting->SetDefaultValue(bDefaultValue);

	const FGameSettingHandle Handle = Registry.AddSetting(Setting, ParentTab);
	if (Handle.IsValid())
	{
		OutHandles.Add(Handle);
	}
}

#if WITH_EDITOR
EDataValidationResult UGameSettingsContribution_Toggle::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!SettingId.IsValid())
	{
		Context.AddError(LOCTEXT("Toggle_NoTag", "Toggle contribution: SettingId tag is required."));
		Result = EDataValidationResult::Invalid;
	}
	if (DisplayName.IsEmpty())
	{
		Context.AddError(LOCTEXT("Toggle_NoName", "Toggle contribution: DisplayName is required."));
		Result = EDataValidationResult::Invalid;
	}
	Result = CombineDataValidationResults(Result,
		Binding.Validate(Context, FString::Printf(TEXT("Toggle '%s'"), *SettingId.ToString())));

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
