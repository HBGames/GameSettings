// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsRowContribution.h"
#include "GameplayTagContainer.h"
#include "GameSettingsContribution_Action.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Button-style setting ("Open HDR calibration", "Reset bindings to
 * default", that kind of thing). Clicking the button broadcasts
 * NamedAction through the registry's OnSettingNamedActionEvent. Subscribe
 * to that event from the settings screen to handle the click.
 */
UCLASS(MinimalAPI, DisplayName = "Game Setting Action")
class UGameSettingsContribution_Action : public UGameSettingsRowContribution
{
	GENERATED_BODY()
public:
	/** Localized label on the button itself. */
	UPROPERTY(EditAnywhere, Category = "Display")
	FText ActionText;

	/** Tag broadcast when the button is clicked; the screen routes by this. */
	UPROPERTY(EditAnywhere, Category = "Action")
	FGameplayTag NamedAction;

	UE_API virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override;

	UE_API virtual FPrimaryAssetType GetContributionPrimaryAssetType() const override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

	static UE_API const FPrimaryAssetType ContributionPrimaryAssetType;
};

#undef UE_API
