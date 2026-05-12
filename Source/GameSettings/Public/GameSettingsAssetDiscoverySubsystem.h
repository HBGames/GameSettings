// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "AssetRegistry/AssetData.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "GameSettingsAssetDiscoverySubsystem.generated.h"

#define UE_API GAMESETTINGS_API

class IAssetRegistry;
class UGameSettingsContribution;
struct FAssetData;

/**
 * Watches the asset registry for UGameSettingsContribution DataAssets and
 * broadcasts when they become available. Per-LocalPlayer
 * UGameSettingsSubsystem instances listen to these events and feed the
 * contributions through their normal apply path.
 *
 * Contributions are filtered by the bEnabled registry tag, so disabled
 * assets don't trigger a full load just to be rejected.
 *
 * Existing assets discovered on first registry-ready, new assets caught
 * via OnAssetAdded (covers GFP content mounts and editor saves), gone
 * assets caught via OnAssetRemoved.
 */
UCLASS(MinimalAPI)
class UGameSettingsAssetDiscoverySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UE_API virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	UE_API virtual void Deinitialize() override;

	/** Contributions discovered so far, in arrival order. */
	const TArray<TObjectPtr<UGameSettingsContribution>>& GetKnownContributions() const { return KnownContributions; }

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnContributionAssetEvent, UGameSettingsContribution* /*Contribution*/);

	/** Fires when a previously-unseen, enabled contribution asset is ready. */
	FOnContributionAssetEvent OnContributionAssetReady;

	/** Fires when a known contribution asset goes away (content unmount, asset delete). */
	FOnContributionAssetEvent OnContributionAssetRemoved;

private:
	UE_API void OnFilesLoaded();
	UE_API void OnAssetAdded(const FAssetData& AssetData);
	UE_API void OnAssetRemoved(const FAssetData& AssetData);

	UE_API bool IsContributionAsset(const FAssetData& AssetData) const;
	static UE_API bool IsEnabledByTag(const FAssetData& AssetData);

	UE_API void TryAdd(const FAssetData& AssetData);
	UE_API void RemoveByPath(const FSoftObjectPath& Path);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGameSettingsContribution>> KnownContributions;

	/** Fast lookup of contribution by asset path for OnAssetRemoved. */
	UPROPERTY(Transient)
	TMap<FSoftObjectPath, TObjectPtr<UGameSettingsContribution>> ContributionsByPath;

	FDelegateHandle FilesLoadedHandle;
	FDelegateHandle AssetAddedHandle;
	FDelegateHandle AssetRemovedHandle;

	bool bRegistryReady = false;
};

#undef UE_API
