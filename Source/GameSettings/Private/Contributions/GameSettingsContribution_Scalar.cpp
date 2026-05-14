// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsContribution_Scalar.h"

#include "GameSettingRegistry.h"
#include "GameSettingValueScalarDynamic.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsContribution_Scalar)

#define LOCTEXT_NAMESPACE "GameSettings"

const FPrimaryAssetType UGameSettingsContribution_Scalar::ContributionPrimaryAssetType = FPrimaryAssetType(TEXT("GameSettingsScalar"));

FPrimaryAssetType UGameSettingsContribution_Scalar::GetContributionPrimaryAssetType() const
{
	return ContributionPrimaryAssetType;
}

namespace
{
	const FSettingScalarFormatFunction& ResolveFormat(EGameSettingsScalarFormat Format)
	{
		switch (Format)
		{
			case EGameSettingsScalarFormat::Raw:                         return UGameSettingValueScalarDynamic::Raw;
			case EGameSettingsScalarFormat::RawOneDecimal:               return UGameSettingValueScalarDynamic::RawOneDecimal;
			case EGameSettingsScalarFormat::RawTwoDecimals:              return UGameSettingValueScalarDynamic::RawTwoDecimals;
			case EGameSettingsScalarFormat::ZeroToOnePercent_OneDecimal: return UGameSettingValueScalarDynamic::ZeroToOnePercent_OneDecimal;
			case EGameSettingsScalarFormat::SourceAsPercent1:            return UGameSettingValueScalarDynamic::SourceAsPercent1;
			case EGameSettingsScalarFormat::SourceAsPercent100:          return UGameSettingValueScalarDynamic::SourceAsPercent100;
			case EGameSettingsScalarFormat::SourceAsInteger:             return UGameSettingValueScalarDynamic::SourceAsInteger;
			case EGameSettingsScalarFormat::ZeroToOnePercent:
			default:                                                     return UGameSettingValueScalarDynamic::ZeroToOnePercent;
		}
	}
}

void UGameSettingsContribution_Scalar::Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles)
{
	if (!GetPrimaryAssetId().IsValid() || DisplayName.IsEmpty() || !Binding.IsValid())
	{
		return;
	}

	UGameSettingValueScalarDynamic* Setting = NewObject<UGameSettingValueScalarDynamic>(&Registry);
	ConfigureBaseSetting(*Setting);

	if (TSharedPtr<FGameSettingDataSource> Getter = Binding.CreateGetter())
	{
		Setting->SetDynamicGetter(Getter.ToSharedRef());
	}
	if (TSharedPtr<FGameSettingDataSource> Setter = Binding.CreateSetter())
	{
		Setting->SetDynamicSetter(Setter.ToSharedRef());
	}
	Setting->SetDefaultValue(DefaultValue);
	Setting->SetSourceRangeAndStep(TRange<double>(SourceRange.X, SourceRange.Y), SourceStep);
	if (MinimumLimit.IsSet()) Setting->SetMinimumLimit(MinimumLimit.GetValue());
	if (MaximumLimit.IsSet()) Setting->SetMaximumLimit(MaximumLimit.GetValue());
	Setting->SetDisplayFormat(ResolveFormat(DisplayFormat));

	const FGameSettingHandle Handle = Registry.AddSetting(Setting, ParentContainer);
	if (Handle.IsValid())
	{
		OutHandles.Add(Handle);
	}
}

#if WITH_EDITOR
EDataValidationResult UGameSettingsContribution_Scalar::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (SourceRange.X >= SourceRange.Y)
	{
		Context.AddError(LOCTEXT("Scalar_BadRange", "Scalar contribution: SourceRange.X must be less than SourceRange.Y."));
		Result = EDataValidationResult::Invalid;
	}
	if (SourceStep <= 0.0)
	{
		Context.AddError(LOCTEXT("Scalar_BadStep", "Scalar contribution: SourceStep must be positive."));
		Result = EDataValidationResult::Invalid;
	}
	Result = CombineDataValidationResults(Result,
		Binding.Validate(Context, FString::Printf(TEXT("Scalar '%s'"), *GetPrimaryAssetId().ToString()),
			EGameSettingsBindingValueType::Numeric));

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
