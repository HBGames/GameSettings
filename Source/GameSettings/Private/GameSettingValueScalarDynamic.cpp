// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameSettingValueScalarDynamic.h"

#include "DataSource/GameSettingDataSource.h"
#include "UObject/WeakObjectPtr.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingValueScalarDynamic)

#define LOCTEXT_NAMESPACE "GameSetting"

//////////////////////////////////////////////////////////////////////////
// SettingScalarFormats
//////////////////////////////////////////////////////////////////////////

static FText PercentFormat = LOCTEXT("PercentFormat", "{0}%");

FSettingScalarFormatFunction UGameSettingValueScalarDynamic::Raw([](double SourceValue, double NormalizedValue) {
	return FText::AsNumber(SourceValue);
});

FSettingScalarFormatFunction UGameSettingValueScalarDynamic::RawOneDecimal([](double SourceValue, double NormalizedValue) {
	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.MinimumIntegralDigits = 1;
	FormattingOptions.MinimumFractionalDigits = 1;
	FormattingOptions.MaximumFractionalDigits = 1;
	return FText::AsNumber(SourceValue, &FormattingOptions);
});

FSettingScalarFormatFunction UGameSettingValueScalarDynamic::RawTwoDecimals([](double SourceValue, double NormalizedValue) {
	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.MinimumIntegralDigits = 1;
	FormattingOptions.MinimumFractionalDigits = 2;
	FormattingOptions.MaximumFractionalDigits = 2;
	return FText::AsNumber(SourceValue, &FormattingOptions);
});

FSettingScalarFormatFunction UGameSettingValueScalarDynamic::SourceAsInteger([](double SourceValue, double NormalizedValue) {
	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.MinimumIntegralDigits = 1;
	FormattingOptions.MinimumFractionalDigits = 0;
	FormattingOptions.MaximumFractionalDigits = 0;
	return FText::AsNumber(SourceValue, &FormattingOptions);
});

FSettingScalarFormatFunction UGameSettingValueScalarDynamic::ZeroToOnePercent([](double SourceValue, double NormalizedValue) {
	return FText::Format(PercentFormat, (int32)FMath::RoundHalfFromZero(100.0 * NormalizedValue));
});

FSettingScalarFormatFunction UGameSettingValueScalarDynamic::ZeroToOnePercent_OneDecimal([](double SourceValue, double NormalizedValue) {
	const FNumberFormattingOptions& FormattingOptions = GetOneDecimalFormattingOptions();
	const double NormalizedValueTo100_0 = FMath::RoundHalfFromZero(1000.0 * NormalizedValue);
	return FText::Format(PercentFormat, FText::AsNumber(NormalizedValueTo100_0 / 10.0, &FormattingOptions));
});

const FNumberFormattingOptions& UGameSettingValueScalarDynamic::GetOneDecimalFormattingOptions()
{
	static FNumberFormattingOptions FormattingOptions;
	FormattingOptions.MinimumFractionalDigits = 1;
	FormattingOptions.MaximumFractionalDigits = 1;

	return FormattingOptions;
}

FSettingScalarFormatFunction UGameSettingValueScalarDynamic::SourceAsPercent1([](double SourceValue, double NormalizedValue) {
	return FText::Format(PercentFormat, (int32)FMath::RoundHalfFromZero(100.0 * SourceValue));
});

FSettingScalarFormatFunction UGameSettingValueScalarDynamic::SourceAsPercent100([](double SourceValue, double NormalizedValue) {
	return FText::Format(PercentFormat, (int32)FMath::RoundHalfFromZero(SourceValue));
});

//////////////////////////////////////////////////////////////////////////
// UGameSettingValueScalarDynamic
//////////////////////////////////////////////////////////////////////////

UGameSettingValueScalarDynamic::UGameSettingValueScalarDynamic()
{
}

void UGameSettingValueScalarDynamic::Startup()
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

void UGameSettingValueScalarDynamic::OnDataSourcesReady()
{
	StartupComplete();
}

void UGameSettingValueScalarDynamic::OnInitialized()
{
#if !UE_BUILD_SHIPPING
	ensureAlwaysMsgf(DisplayFormat, TEXT("%s: Has no DisplayFormat set.  Please call SetDisplayFormat."), *GetSettingId().ToString());
#endif

	if (!ensureAlways(Getter) || !ensureAlways(Setter))
	{
		return;
	}

#if !UE_BUILD_SHIPPING
	ensureAlwaysMsgf(Getter->Resolve(LocalPlayer), TEXT("%s: %s did not resolve, are all functions and properties valid, and are they UFunctions/UProperties?"), *GetSettingId().ToString(), *Getter->ToString());
	ensureAlwaysMsgf(Setter->Resolve(LocalPlayer), TEXT("%s: %s did not resolve, are all functions and properties valid, and are they UFunctions/UProperties?"), *GetSettingId().ToString(), *Setter->ToString());
#endif

	Super::OnInitialized();
}

void UGameSettingValueScalarDynamic::StoreInitial()
{
	InitialValue = GetValue();
}

void UGameSettingValueScalarDynamic::ResetToDefault()
{
	if (DefaultValue.IsSet())
	{
		SetValue(DefaultValue.GetValue(), EGameSettingChangeReason::ResetToDefault);
	}
}

void UGameSettingValueScalarDynamic::RestoreToInitial()
{
	SetValue(InitialValue, EGameSettingChangeReason::RestoreToInitial);
}

void UGameSettingValueScalarDynamic::SetDynamicGetter(const TSharedRef<FGameSettingDataSource>& InGetter)
{
	Getter = InGetter;
}

void UGameSettingValueScalarDynamic::SetDynamicSetter(const TSharedRef<FGameSettingDataSource>& InSetter)
{
	Setter = InSetter;
}

void UGameSettingValueScalarDynamic::SetDefaultValue(double InValue)
{
	DefaultValue = InValue;
}

void UGameSettingValueScalarDynamic::SetDisplayFormat(FSettingScalarFormatFunction InDisplayFormat)
{
	DisplayFormat = InDisplayFormat;
}

void UGameSettingValueScalarDynamic::SetSourceRangeAndStep(const TRange<double>& InRange, double InStep)
{
	SourceRange = InRange;
	SourceStep = InStep;
}

void UGameSettingValueScalarDynamic::SetMinimumLimit(const TOptional<double>& InMinimum)
{
	Minimum = InMinimum;
}

void UGameSettingValueScalarDynamic::SetMaximumLimit(const TOptional<double>& InMaximum)
{
	Maximum = InMaximum;
}

double UGameSettingValueScalarDynamic::GetValue() const
{
	const FString OutValue = Getter->GetValueAsString(LocalPlayer);

	double Value;
	LexFromString(Value, *OutValue);

	return Value;
}

TRange<double> UGameSettingValueScalarDynamic::GetSourceRange() const
{
	return SourceRange;
}

double UGameSettingValueScalarDynamic::GetSourceStep() const
{
	return SourceStep;
}

TOptional<double> UGameSettingValueScalarDynamic::GetDefaultValue() const
{
	return DefaultValue;
}

void UGameSettingValueScalarDynamic::SetValue(double InValue, EGameSettingChangeReason Reason)
{
	InValue = FMath::RoundHalfFromZero(InValue / SourceStep);
	InValue = InValue * SourceStep;

	if (Minimum.IsSet())
	{
		InValue = FMath::Max(Minimum.GetValue(), InValue);
	}

	if (Maximum.IsSet())
	{
		InValue = FMath::Min(Maximum.GetValue(), InValue);
	}

	const FString StringValue = LexToString(InValue);
	Setter->SetValue(LocalPlayer, StringValue);

	NotifySettingChanged(Reason);
}

bool UGameSettingValueScalarDynamic::IsResettableToDefault() const
{
	if (!DefaultValue.IsSet())
	{
		return false;
	}
	// Compare in source units, snapped to the same step the setter would apply,
	// so a default that isn't on a step boundary doesn't read as perpetually
	// non-default. KINDA_SMALL_NUMBER matches the tolerance the scalar VM uses
	// to absorb float<->double round-trip noise from the slider binding.
	const double Current = GetValue();
	const double Default = DefaultValue.GetValue();
	return !FMath::IsNearlyEqual(Current, Default, KINDA_SMALL_NUMBER);
}

FText UGameSettingValueScalarDynamic::GetFormattedText() const
{
	const double SourceValue = GetValue();
	const double NormalizedValue = GetValueNormalized();

	return DisplayFormat(SourceValue, NormalizedValue);
}

#undef LOCTEXT_NAMESPACE
