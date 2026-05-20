// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsBinding.h"
#include "Contributions/GameSettingsRowContribution.h"

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
class UGameSettingsContribution_Scalar : public UGameSettingsRowContribution
{
	GENERATED_BODY()
public:
	/**
	 * When true (default), Reset-To-Default uses the value the binding's getter
	 * returns on TargetClass's CDO - the C++-declared property default. Uncheck
	 * to use DefaultValue as an explicit override.
	 */
	UPROPERTY(EditAnywhere, Category = "Value")
	bool bUseClassDefaultValue = true;

	/** Explicit Reset-To-Default value. Only used when bUseClassDefaultValue is false. */
	UPROPERTY(EditAnywhere, Category = "Value", meta = (EditCondition = "!bUseClassDefaultValue"))
	double DefaultValue = 0.0;

	/** The full source range the slider can move across. */
	UPROPERTY(EditAnywhere, Category = "Value")
	FVector2D SourceRange = FVector2D(0.0, 1.0);

	/** Step size between snapped values. */
	UPROPERTY(EditAnywhere, Category = "Value")
	double SourceStep = 0.01;

	/** Optional clamp tighter than SourceRange.X. Unset means no extra low clamp. */
	UPROPERTY(EditAnywhere, Category = "Value")
	TOptional<double> MinimumLimit;

	/** Optional clamp tighter than SourceRange.Y. Unset means no extra high clamp. */
	UPROPERTY(EditAnywhere, Category = "Value")
	TOptional<double> MaximumLimit;

	/** How the current value is formatted into the on-screen text. */
	UPROPERTY(EditAnywhere, Category = "Display")
	EGameSettingsScalarFormat DisplayFormat = EGameSettingsScalarFormat::ZeroToOnePercent;

	UPROPERTY(EditAnywhere, Category = "Value")
	FGameSettingsBinding Binding;

	UE_API virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override;

	UE_API virtual FPrimaryAssetType GetContributionPrimaryAssetType() const override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

	static UE_API const FPrimaryAssetType ContributionPrimaryAssetType;
};

#undef UE_API
