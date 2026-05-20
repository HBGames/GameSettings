// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsBinding.h"
#include "Contributions/GameSettingsRowContribution.h"

#include "GameSettingsContribution_Toggle.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Boolean toggle setting bound through FGameSettingsBinding to a subsystem
 * getter/setter pair. Both functions must be UFUNCTION-reflected and take/return
 * a bool.
 */
UCLASS(MinimalAPI, DisplayName = "Game Setting Toggle")
class UGameSettingsContribution_Toggle : public UGameSettingsRowContribution
{
	GENERATED_BODY()
public:
	/**
	 * When true (default), Reset-To-Default uses the value the binding's getter
	 * returns on TargetClass's CDO - i.e. the C++-declared property default, so
	 * it can't silently drift from the explicit field below. Uncheck to use
	 * bDefaultValue as an explicit override.
	 */
	UPROPERTY(EditAnywhere, Category = "Value")
	bool bUseClassDefaultValue = true;

	/** Explicit Reset-To-Default value. Only used when bUseClassDefaultValue is false. */
	UPROPERTY(EditAnywhere, Category = "Value", meta = (EditCondition = "!bUseClassDefaultValue"))
	bool bDefaultValue = false;

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
