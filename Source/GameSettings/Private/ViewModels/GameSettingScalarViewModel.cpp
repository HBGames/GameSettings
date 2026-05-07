// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "ViewModels/GameSettingScalarViewModel.h"

#include "GameSettingValueScalar.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingScalarViewModel)

void UGameSettingScalarViewModel::SetNormalizedValue(double NewValue)
{
	if (FMath::IsNearlyEqual(NormalizedValue, NewValue))
	{
		return;
	}

	UGameSettingValueScalar* Scalar = Cast<UGameSettingValueScalar>(Setting);
	if (!Scalar)
	{
		return;
	}

	Scalar->SetValueNormalized(NewValue);

	// Re-read from the setting: it may have clamped or snapped the value.
	const double Applied = Scalar->GetValueNormalized();
	if (!FMath::IsNearlyEqual(NormalizedValue, Applied))
	{
		NormalizedValue = Applied;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetNormalizedValue);
	}

	const FText NewFormatted = Scalar->GetFormattedText();
	if (!FormattedText.EqualTo(NewFormatted))
	{
		FormattedText = NewFormatted;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetFormattedText);
	}
}

void UGameSettingScalarViewModel::RefreshFromSetting()
{
	Super::RefreshFromSetting();

	UGameSettingValueScalar* Scalar = Cast<UGameSettingValueScalar>(Setting);
	if (!Scalar)
	{
		return;
	}

	const TRange<double> Range = Scalar->GetSourceRange();
	SourceMin = Range.GetLowerBoundValue();
	SourceMax = Range.GetUpperBoundValue();
	Step = Scalar->GetSourceStep();

	const double NewNormalized = Scalar->GetValueNormalized();
	if (!FMath::IsNearlyEqual(NormalizedValue, NewNormalized))
	{
		NormalizedValue = NewNormalized;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetNormalizedValue);
	}

	const FText NewFormatted = Scalar->GetFormattedText();
	if (!FormattedText.EqualTo(NewFormatted))
	{
		FormattedText = NewFormatted;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetFormattedText);
	}
}
