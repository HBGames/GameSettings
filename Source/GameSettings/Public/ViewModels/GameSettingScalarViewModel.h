// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "ViewModels/GameSettingViewModel.h"

#include "GameSettingScalarViewModel.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * View model for a slider (scalar) setting. Wraps UGameSettingValueScalar.
 *
 * Two-way binding on NormalizedValue (0..1). Widget writes propagate back
 * to the setting via SetValueNormalized; the setting's clamp/step handling
 * applies on the way through.
 *
 * SourceMin/Max/Step are read-once from the setting on SetSetting and
 * stay constant. They're plain getters (no FieldNotify) since they don't
 * change at runtime.
 */
UCLASS(MinimalAPI, BlueprintType, DisplayName = "Game Setting Scalar")
class UGameSettingScalarViewModel : public UGameSettingViewModel
{
	GENERATED_BODY()

public:
	// Two-way binding pair. NormalizedValue is a UPROPERTY anchor (below) so the
	// MVVM compiler resolves a single bindable field with both Get and Set sides.
	// FieldNotify lives on the property, not on the getter UFUNCTION.

	UFUNCTION(BlueprintCallable, Category = "Value")
	UE_API void SetNormalizedValue(double NewValue);

	UFUNCTION(BlueprintCallable, Category = "Value")
	double GetNormalizedValue() const { return NormalizedValue; }

	// Read-only display.

	UFUNCTION(BlueprintCallable, Category = "Value")
	const FText& GetFormattedText() const { return FormattedText; }

	// Constants from the setting; not FieldNotify.

	UFUNCTION(BlueprintCallable, Category = "Value")
	double GetSourceMin() const { return SourceMin; }

	UFUNCTION(BlueprintCallable, Category = "Value")
	double GetSourceMax() const { return SourceMax; }

	UFUNCTION(BlueprintCallable, Category = "Value")
	double GetStep() const { return Step; }

protected:
	UE_API virtual void RefreshFromSetting() override;

private:
	/**
	 * Two-way bindable. Getter / Setter (C++ accessors) auto-detect GetNormalizedValue / SetNormalizedValue
	 * by naming convention - that's what MVVM's binding compiler looks at when building the FieldNotify
	 * field handles. BlueprintGetter / BlueprintSetter route BP-side reads / writes through the same
	 * UFUNCTIONs.
	 */
	UPROPERTY(BlueprintReadWrite, Getter, Setter, FieldNotify, BlueprintGetter=GetNormalizedValue, BlueprintSetter=SetNormalizedValue, Category="Value", meta=(AllowPrivateAccess=true))
	double NormalizedValue = 0.0;

	/** One-way readable display string. */
	UPROPERTY(BlueprintReadOnly, Getter, FieldNotify, BlueprintGetter=GetFormattedText, Category="Value", meta=(AllowPrivateAccess=true))
	FText FormattedText;

	double SourceMin = 0.0;
	double SourceMax = 1.0;
	double Step = 0.01;
};

#undef UE_API
