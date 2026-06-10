// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameSettingHandle.h"

#include "GameSettingsContribution.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSettingRegistry;

/**
 * A unit of settings contribution. Subclasses describe a setting (or
 * group of settings) and apply themselves to a registry.
 *
 * Three contribution paths consume this type:
 *   1. Asset auto-discovery (default). Save a UGameSettingsContribution
 *      DataAsset anywhere under a mounted content path; the
 *      UGameSettingsAssetDiscoverySubsystem finds it via the asset
 *      registry and applies it to every LocalPlayer's
 *      UGameSettingsSubsystem. Set bEnabled = false to skip without
 *      loading the asset.
 *   2. UGameSettingsAutoContributor. C++ subclass with a CDO that the
 *      runtime discovers via GetDerivedClasses at module startup. Use
 *      this when the contribution is code-only.
 *   3. UGameFeatureAction_RegisterGameSettings. Advanced cherry-picking
 *      list, useful when you want explicit ordering or to register
 *      contributions that live outside the GFP's content folder.
 *
 * Apply() must push every allocated handle onto OutHandles. The caller
 * keeps those handles and removes by handle on teardown; that's how the
 * system avoids leaks across plugin unload.
 */
UCLASS(MinimalAPI, BlueprintType, Abstract,
	PrioritizeCategories = ("Display", "Value", "Binding", "Edit Conditions", "Registration"))
class UGameSettingsContribution : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	/**
	 * When false, the asset is skipped by the auto-discovery subsystem.
	 * AssetRegistrySearchable so the discovery scan can read the value
	 * from the asset registry without loading the full uasset.
	 *
	 * Has no effect on UGameSettingsAutoContributor (CDO-discovered) or
	 * direct UGameFeatureAction_RegisterGameSettings usage.
	 */
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Registration")
	bool bEnabled = true;

	/**
	 * Primary asset id derived from GetContributionPrimaryAssetType() and the
	 * asset name. Each typed contribution subclass overrides
	 * GetContributionPrimaryAssetType so the asset manager (and the
	 * AllowedTypes filter on FPrimaryAssetId pickers) can distinguish
	 * Tab/Toggle/Scalar/Discrete/Action without loading the asset.
	 */
	UE_API virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 * Subclass hook: the primary asset type this contribution exposes to the
	 * asset manager. Override in each typed contribution. The base returns the
	 * catch-all "GameSettingsContribution" type, which still matches a
	 * recursive scan but doesn't help typed pickers.
	 */
	UE_API virtual FPrimaryAssetType GetContributionPrimaryAssetType() const;

	/**
	 * Add this contribution's settings to the registry. Append every
	 * resulting handle to OutHandles so the caller can later call
	 * UGameSettingRegistry::RemoveByHandle on teardown.
	 */
	UE_API virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles)
		PURE_VIRTUAL(UGameSettingsContribution::Apply, );

	/** Catch-all primary asset type; matches a recursive registry scan. */
	static UE_API const FPrimaryAssetType PrimaryAssetType;
};

#undef UE_API
