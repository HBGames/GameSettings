// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsContribution_Discrete.h"

#include "GameSettingRegistry.h"
#include "GameSettingValueDiscreteDynamic.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsContribution_Discrete)

#define LOCTEXT_NAMESPACE "GameSettings"

void UGameSettingsContribution_Discrete::Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles)
{
	if (!SettingId.IsValid() || DisplayName.IsEmpty() || !Binding.IsValid() || Options.IsEmpty())
	{
		return;
	}

	UGameSettingValueDiscreteDynamic* Setting = NewObject<UGameSettingValueDiscreteDynamic>(&Registry);
	ConfigureBaseSetting(*Setting);

	if (TSharedPtr<FGameSettingDataSource> Getter = Binding.CreateGetter())
	{
		Setting->SetDynamicGetter(Getter.ToSharedRef());
	}
	if (TSharedPtr<FGameSettingDataSource> Setter = Binding.CreateSetter())
	{
		Setting->SetDynamicSetter(Setter.ToSharedRef());
	}
	if (!DefaultValue.IsEmpty())
	{
		Setting->SetDefaultValueFromString(DefaultValue);
	}
	for (const FGameSettingsDiscreteOption& Option : Options)
	{
		Setting->AddDynamicOption(Option.Value, Option.DisplayText);
	}

	const FGameSettingHandle Handle = Registry.AddSetting(Setting, ParentTab);
	if (Handle.IsValid())
	{
		OutHandles.Add(Handle);
	}
}

#if WITH_EDITOR
EDataValidationResult UGameSettingsContribution_Discrete::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!SettingId.IsValid())
	{
		Context.AddError(LOCTEXT("Discrete_NoTag", "Discrete contribution: SettingId tag is required."));
		Result = EDataValidationResult::Invalid;
	}
	if (DisplayName.IsEmpty())
	{
		Context.AddError(LOCTEXT("Discrete_NoName", "Discrete contribution: DisplayName is required."));
		Result = EDataValidationResult::Invalid;
	}
	if (Options.IsEmpty())
	{
		Context.AddError(LOCTEXT("Discrete_NoOptions", "Discrete contribution: at least one option is required."));
		Result = EDataValidationResult::Invalid;
	}
	if (!DefaultValue.IsEmpty())
	{
		const bool bMatch = Options.ContainsByPredicate(
			[&](const FGameSettingsDiscreteOption& Opt) { return Opt.Value == DefaultValue; });
		if (!bMatch)
		{
			Context.AddError(FText::Format(
				LOCTEXT("Discrete_DefaultMissing", "Discrete contribution: DefaultValue '{0}' is not present in Options."),
				FText::FromString(DefaultValue)));
			Result = EDataValidationResult::Invalid;
		}
	}
	Result = CombineDataValidationResults(Result,
		Binding.Validate(Context, FString::Printf(TEXT("Discrete '%s'"), *SettingId.ToString())));

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
