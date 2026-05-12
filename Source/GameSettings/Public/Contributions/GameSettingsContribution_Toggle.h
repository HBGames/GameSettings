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
	/** Default value if the user resets to default. */
	UPROPERTY(EditAnywhere, Category = "Value")
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
