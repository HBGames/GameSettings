// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsContribution_Discrete.h"

#include "Contributions/GameSettingsDiscreteOptionsProvider.h"
#include "GameSettingRegistry.h"
#include "GameSettingValueDiscreteDynamic.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsContribution_Discrete)

#define LOCTEXT_NAMESPACE "GameSettings"

const FPrimaryAssetType UGameSettingsContribution_Discrete::ContributionPrimaryAssetType = FPrimaryAssetType(TEXT("GameSettingsDiscrete"));

FPrimaryAssetType UGameSettingsContribution_Discrete::GetContributionPrimaryAssetType() const
{
	return ContributionPrimaryAssetType;
}

void UGameSettingsContribution_Discrete::Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles)
{
	if (!GetPrimaryAssetId().IsValid() || DisplayName.IsEmpty())
	{
		return;
	}

	const bool bHasCustomSettingClass = SettingClass != nullptr
		&& SettingClass != UGameSettingValueDiscreteDynamic::StaticClass();

	if (!bHasCustomSettingClass)
	{
		// Generic UGameSettingValueDiscreteDynamic requires bindings and at least one option source.
		if (!Binding.IsValid())
		{
			return;
		}
		if (Options.IsEmpty() && OptionsProvider == nullptr)
		{
			return;
		}
	}

	UClass* ClassToUse = SettingClass ? SettingClass.Get() : UGameSettingValueDiscreteDynamic::StaticClass();
	UGameSettingValueDiscreteDynamic* Setting = NewObject<UGameSettingValueDiscreteDynamic>(&Registry, ClassToUse);
	ConfigureBaseSetting(*Setting);

	if (Binding.IsValid())
	{
		if (TSharedPtr<FGameSettingDataSource> Getter = Binding.CreateGetter())
		{
			Setting->SetDynamicGetter(Getter.ToSharedRef());
		}
		if (TSharedPtr<FGameSettingDataSource> Setter = Binding.CreateSetter())
		{
			Setting->SetDynamicSetter(Setter.ToSharedRef());
		}
	}
	FString EffectiveDefault = DefaultValue;
	if (bUseClassDefaultValue)
	{
		FString ClassDefaultStr;
		if (Binding.TryGetClassDefaultValueAsString(ClassDefaultStr))
		{
			EffectiveDefault = ClassDefaultStr;
		}
	}
	if (!EffectiveDefault.IsEmpty())
	{
		Setting->SetDefaultValueFromString(EffectiveDefault);
	}

	// Options. Custom setting classes are assumed to self-manage their options
	// (overriding OnInitialized / GetDiscreteOptions); the AddDynamicOption
	// calls below are harmless when ignored by such subclasses.
	if (OptionsProvider)
	{
		TArray<FGameSettingsDiscreteOption> Generated;
		OptionsProvider->GenerateOptions(Registry.GetOwningLocalPlayer(), Generated);
		for (const FGameSettingsDiscreteOption& Option : Generated)
		{
			Setting->AddDynamicOption(Option.Value, Option.DisplayText);
		}
	}
	else
	{
		for (const FGameSettingsDiscreteOption& Option : Options)
		{
			Setting->AddDynamicOption(Option.Value, Option.DisplayText);
		}
	}

	const FGameSettingHandle Handle = Registry.AddSetting(Setting, ParentContainer);
	if (Handle.IsValid())
	{
		OutHandles.Add(Handle);
		Registry.ApplyEditConditionSpecs(Setting, EditConditions);
	}
}

#if WITH_EDITOR
EDataValidationResult UGameSettingsContribution_Discrete::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	const bool bHasCustomSettingClass = SettingClass != nullptr
		&& SettingClass != UGameSettingValueDiscreteDynamic::StaticClass();

	if (!bHasCustomSettingClass)
	{
		if (Options.IsEmpty() && OptionsProvider == nullptr)
		{
			Context.AddError(LOCTEXT("Discrete_NoOptions",
				"Discrete contribution: provide an OptionsProvider, a non-empty Options array, or a custom SettingClass."));
			Result = EDataValidationResult::Invalid;
		}
		Result = CombineDataValidationResults(Result,
			Binding.Validate(Context, FString::Printf(TEXT("Discrete '%s'"), *GetPrimaryAssetId().ToString()),
				EGameSettingsBindingValueType::Discrete));
	}

	if (!DefaultValue.IsEmpty() && OptionsProvider == nullptr && !Options.IsEmpty())
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

	return Result;
}

bool UGameSettingsContribution_Discrete::CanEditChange(const FProperty* InProperty) const
{
	if (!Super::CanEditChange(InProperty))
	{
		return false;
	}

	if (!InProperty)
	{
		return true;
	}

	const bool bHasCustomSettingClass = SettingClass != nullptr
		&& SettingClass != UGameSettingValueDiscreteDynamic::StaticClass();
	if (!bHasCustomSettingClass)
	{
		return true;
	}

	// These only feed the generic UGameSettingValueDiscreteDynamic. A custom
	// SettingClass instantiates its own value type and self-manages options /
	// default / persistence, so editing them would be misleading.
	static const TSet<FName> DisabledWhenCustom = {
		GET_MEMBER_NAME_CHECKED(UGameSettingsContribution_Discrete, Binding),
		GET_MEMBER_NAME_CHECKED(UGameSettingsContribution_Discrete, Options),
		GET_MEMBER_NAME_CHECKED(UGameSettingsContribution_Discrete, OptionsProvider),
		GET_MEMBER_NAME_CHECKED(UGameSettingsContribution_Discrete, bUseClassDefaultValue),
		GET_MEMBER_NAME_CHECKED(UGameSettingsContribution_Discrete, DefaultValue),
	};

	return !DisabledWhenCustom.Contains(InProperty->GetFName());
}
#endif

#undef LOCTEXT_NAMESPACE
