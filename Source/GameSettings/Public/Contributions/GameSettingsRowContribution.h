// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsTypedContribution.h"
#include "UObject/PrimaryAssetId.h"

#include "GameSettingsRowContribution.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Intermediate base for any contribution that renders as a row under a tab
 * (Toggle/Scalar/Discrete/Action). Owns the shared ParentTab field; the
 * picker is filtered to GameSettingsTab assets only.
 *
 * Tabs themselves derive directly from UGameSettingsTypedContribution
 * because they're top-level containers without a parent.
 */
UCLASS(MinimalAPI, Abstract)
class UGameSettingsRowContribution : public UGameSettingsTypedContribution
{
	GENERATED_BODY()
public:
	/**
	 * Primary asset id of the parent tab (a UGameSettingsContribution_Tab
	 * asset). If unset or unregistered, the setting is added at top level.
	 * Asset rename redirects keep this reference live.
	 */
	UPROPERTY(EditAnywhere, Category = "Identity", meta = (AllowedTypes = "GameSettingsTab"))
	FPrimaryAssetId ParentTab;
};

#undef UE_API
