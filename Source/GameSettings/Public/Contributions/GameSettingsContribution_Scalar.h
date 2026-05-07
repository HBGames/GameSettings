// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsBinding.h"
#include "Contributions/GameSettingsTypedContribution.h"

#include "GameSettingsContribution_Scalar.generated.h"

#define UE_API GAMESETTINGS_API

/** Format functions exposed from UGameSettingValueScalarDynamic. */
UENUM(BlueprintType)
enum class EGameSettingsScalarFormat : uint8
{
	/** Print the source value as a plain integer (e.g. "42"). */
	Raw,
	/** Source value, one decimal place. */
	RawOneDecimal,
	/** Source value, two decimal places. */
	RawTwoDecimals,
	/** Treat 0..1 as a percentage (e.g. 0.5 prints as "50%"). */
	ZeroToOnePercent,
	/** Treat 0..1 as a percentage with one decimal place. */
	ZeroToOnePercent_OneDecimal,
	/** Print the underlying source range proportion (0..1) as "X%". */
	SourceAsPercent1,
	/** Print the underlying source range proportion (0..100) as "X%". */
	SourceAsPercent100,
	/** Print as a plain integer. */
	SourceAsInteger,
};

/**
 * Floating-point slider setting bound through FGameSettingsBinding to a
 * subsystem getter/setter pair. Both functions must be UFUNCTION-reflected
 * and take/return a numeric type that converts to/from a string (the path
 * resolver handles the conversion).
 */
UCLASS(MinimalAPI, DisplayName = "Game Setting Scalar")
class UGameSettingsContribution_Scalar : public UGameSettingsTypedContribution
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Identity")
	FGameplayTag ParentTab;

	UPROPERTY(EditAnywhere, Category = "Value")
	double DefaultValue = 0.0;

	/** The full source range the slider can move across. */
	UPROPERTY(EditAnywhere, Category = "Value")
	FVector2D SourceRange = FVector2D(0.0, 1.0);

	/** Step size between snapped values. */
	UPROPERTY(EditAnywhere, Category = "Value")
	double SourceStep = 0.01;

	/** Optional clamp tighter than SourceRange.X. */
	UPROPERTY(EditAnywhere, Category = "Value")
	bool bUseMinimumLimit = false;

	UPROPERTY(EditAnywhere, Category = "Value", meta = (EditCondition = "bUseMinimumLimit"))
	double MinimumLimit = 0.0;

	UPROPERTY(EditAnywhere, Category = "Value")
	bool bUseMaximumLimit = false;

	UPROPERTY(EditAnywhere, Category = "Value", meta = (EditCondition = "bUseMaximumLimit"))
	double MaximumLimit = 1.0;

	/** How the current value is formatted into the on-screen text. */
	UPROPERTY(EditAnywhere, Category = "Display")
	EGameSettingsScalarFormat DisplayFormat = EGameSettingsScalarFormat::ZeroToOnePercent;

	UPROPERTY(EditAnywhere, Category = "Value")
	FGameSettingsBinding Binding;

	UE_API virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

#undef UE_API
