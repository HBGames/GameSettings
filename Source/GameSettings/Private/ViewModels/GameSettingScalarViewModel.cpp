// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "ViewModels/GameSettingScalarViewModel.h"

#include "GameSettingValueScalar.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingScalarViewModel)

void UGameSettingScalarViewModel::SetNormalizedValue(double NewValue)
{
	// Tolerance has to absorb float-to-double precision drift from the two-way
	// slider binding. USlider's Value is float; MVVM converts to double on the
	// way in. 0.7 round-trips as ~0.699999988, which differs from a previously
	// stored 0.7000000001 by more than the default 1e-8 tolerance. Use
	// KINDA_SMALL_NUMBER (1e-4) - well below user-perceptible precision on a
	// 0..1 slider but generous enough to swallow conversion noise.
	constexpr double ScalarTolerance = KINDA_SMALL_NUMBER;

	if (FMath::IsNearlyEqual(NormalizedValue, NewValue, ScalarTolerance))
	{
		return;
	}

	UGameSettingValueScalar* Scalar = Cast<UGameSettingValueScalar>(Setting);
	if (!Scalar)
	{
		return;
	}

	// Update the cached value BEFORE calling into the setting. If the setting
	// fires OnSettingChangedEvent synchronously, RefreshFromSetting runs during
	// this call and compares the setting's new value against our cached value.
	// Setting them equal up-front means RefreshFromSetting's "have I changed?"
	// gate stays closed, no broadcast fires, and the slider's two-way binding
	// doesn't loop back into here. The setter's potential clamp/snap is
	// reconciled below.
	NormalizedValue = NewValue;

	Scalar->SetValueNormalized(NewValue);

	// Reconcile clamp/snap. If the setting accepted NewValue as-is, the slider
	// already has the right value - broadcasting would re-write the same float,
	// the slider would re-broadcast its own FieldNotify, and MVVM's recursion
	// detector would trip. Only broadcast when the setting actually changed
	// the value (clamp / step quantization).
	const double Applied = Scalar->GetValueNormalized();
	if (!FMath::IsNearlyEqual(Applied, NewValue, ScalarTolerance))
	{
		NormalizedValue = Applied;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(NormalizedValue);
	}

	// FormattedText is one-way (no destination side feedback), always safe to broadcast.
	const FText NewFormatted = Scalar->GetFormattedText();
	if (!FormattedText.EqualTo(NewFormatted))
	{
		FormattedText = NewFormatted;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(FormattedText);
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
	const double NewSourceMin = Range.GetLowerBoundValue();
	if (SourceMin != NewSourceMin)
	{
		SourceMin = NewSourceMin;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSourceMin);
	}

	const double NewSourceMax = Range.GetUpperBoundValue();
	if (SourceMax != NewSourceMax)
	{
		SourceMax = NewSourceMax;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetSourceMax);
	}

	const double NewStep = Scalar->GetSourceStep();
	if (Step != NewStep)
	{
		Step = NewStep;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetStep);
	}

	// Match the SetNormalizedValue tolerance so float-conversion noise doesn't
	// trip false broadcasts when RefreshFromSetting fires synchronously from
	// inside the setter (which already updated NormalizedValue up-front).
	const double NewNormalized = Scalar->GetValueNormalized();
	if (!FMath::IsNearlyEqual(NormalizedValue, NewNormalized, KINDA_SMALL_NUMBER))
	{
		NormalizedValue = NewNormalized;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(NormalizedValue);
	}

	const FText NewFormatted = Scalar->GetFormattedText();
	if (!FormattedText.EqualTo(NewFormatted))
	{
		FormattedText = NewFormatted;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(FormattedText);
	}
}
