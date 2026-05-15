// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "EditCondition/GameSettingEditConditionSpec.h"
#include "UObject/PrimaryAssetId.h"

#include "GameSettingEditConditionSpec_DependsOnScalar.generated.h"

#define UE_API GAMESETTINGS_API

UENUM(BlueprintType)
enum class EGameSettingScalarCompare : uint8
{
	LessThan,
	LessOrEqual,
	Equal,
	GreaterOrEqual,
	GreaterThan,
	NotEqual,
};

/**
 * Gate the owning setting on a numeric comparison against another Scalar
 * contribution. Predicate fires when the comparison returns false.
 *
 * Example: "Disable HDR brightness slider unless HDR is enabled" is best
 * expressed as a DependsOnToggle, but "Show advanced options only when
 * resolution scale is above 0.75" is a clean DependsOnScalar case.
 */
UCLASS(MinimalAPI, DisplayName = "Depends On Scalar")
class UGameSettingEditConditionSpec_DependsOnScalar : public UGameSettingEditConditionSpec
{
	GENERATED_BODY()

public:
	/** Target Scalar contribution whose value is read each refresh. */
	UPROPERTY(EditAnywhere, Category = "Dependency", meta = (AllowedTypes = "GameSettingsScalar"))
	FPrimaryAssetId TargetSetting;

	/** Comparison operator applied as: TargetValue Op Threshold. */
	UPROPERTY(EditAnywhere, Category = "Dependency")
	EGameSettingScalarCompare Op = EGameSettingScalarCompare::GreaterThan;

	/** Right-hand-side of the comparison. */
	UPROPERTY(EditAnywhere, Category = "Dependency")
	double Threshold = 0.0;

	/** Tolerance used by Equal / NotEqual comparisons. */
	UPROPERTY(EditAnywhere, Category = "Dependency",
		meta = (EditCondition = "Op == EGameSettingScalarCompare::Equal || Op == EGameSettingScalarCompare::NotEqual", EditConditionHides))
	double Epsilon = 0.0001;

	UE_API virtual void GetSettingDependencies(TArray<FPrimaryAssetId>& OutIds) const override;
	UE_API virtual TSharedPtr<FGameSettingEditCondition> BuildCondition(UGameSettingRegistry& Registry, UGameSetting& Owner) const override;
	UE_API virtual FString DebugDescribe() const override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

#undef UE_API
