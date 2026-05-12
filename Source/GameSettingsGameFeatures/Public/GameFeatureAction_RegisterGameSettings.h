// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameFeatureAction.h"
#include "GameSettingHandle.h"

#include "GameFeatureAction_RegisterGameSettings.generated.h"

#define UE_API GAMESETTINGSGAMEFEATURES_API

class UGameSettingsContribution;
class UGameSettingsSubsystem;

/**
 * Game Feature Action that registers UGameSettingsContribution instances
 * with the LocalPlayer's UGameSettingsSubsystem on activation and
 * removes them on deactivation.
 *
 * To use: add this action to your UGameFeatureData asset, then fill the
 * Contributions array with whatever UGameSettingsContribution subclasses
 * you want (the typed Toggle/Scalar/etc., or your own subclass).
 *
 * Register and remove are symmetric via FGameSettingHandle. If a
 * subsystem goes away while the action is active (split-screen leave,
 * world teardown), the deactivation pass skips it through the weak-ref
 * check.
 */
UCLASS(MinimalAPI, DisplayName = "Register Game Settings")
class UGameFeatureAction_RegisterGameSettings : public UGameFeatureAction
{
	GENERATED_BODY()
public:
	/**
	 * Advanced cherry-picking list. Most settings are auto-discovered
	 * from any UGameSettingsContribution DataAsset under a mounted
	 * content path; only use this array when you need explicit ordering
	 * or want to register a contribution that lives outside the GFP's
	 * own content folder.
	 */
	UPROPERTY(EditAnywhere, Category = "Settings")
	TArray<TObjectPtr<UGameSettingsContribution>> Contributions;

	UE_API virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	UE_API virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

private:
	struct FActiveContribution
	{
		TWeakObjectPtr<UGameSettingsSubsystem> Subsystem;
		TArray<FGameSettingHandle> Handles;
	};

	/** Bag of handles produced by this action's most recent activation, keyed by subsystem. */
	TArray<FActiveContribution> ActiveContributions;
};

#undef UE_API
