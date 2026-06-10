// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "Misc/Guid.h"

#include "GameFeatureAction_AddViewBindings.generated.h"

#define UE_API GAMESETTINGSGAMEFEATURES_API

class UGameSettingsViewBindings;

/**
 * Game Feature Action that pushes a UGameSettingsViewBindings asset onto
 * the plugin's runtime override stack on activation and removes it on
 * deactivation.
 *
 * Use it when a GFP wants to ship its own entry widget classes for
 * setting types it introduces. For example, a VR plugin can ship a
 * VR-specific scalar slider widget that only appears while the VR GFP
 * is active.
 */
UCLASS(MinimalAPI, DisplayName = "Add Game Settings View Bindings")
class UGameFeatureAction_AddViewBindings : public UGameFeatureAction
{
	GENERATED_BODY()
public:
	/** Bindings asset to push. Soft so the GFP doesn't pull it before activation. */
	UPROPERTY(EditAnywhere, Category = "Settings")
	TSoftObjectPtr<UGameSettingsViewBindings> Bindings;

	/** Higher priority is consulted first. Tie-break is insertion order. */
	UPROPERTY(EditAnywhere, Category = "Settings")
	int32 Priority = 0;

	UE_API virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	UE_API virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

private:
	/** Override handle per activation context; deactivating one context pops only its override. */
	TMap<FGameFeatureStateChangeContext, FGuid> ActiveOverrideHandles;

	/**
	 * Strong ref keeping the loaded asset alive while any context is
	 * actived. The module's override stack only holds a weak ref, so
	 * without this the override would silently vanish on GC.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UGameSettingsViewBindings> ResolvedBindings;
};

#undef UE_API
