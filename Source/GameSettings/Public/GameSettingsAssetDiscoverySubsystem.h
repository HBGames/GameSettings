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
 * via OnAssetAdded (covers GFP content mounts and newly created editor
 * assets), gone assets caught via OnAssetRemoved. Editor-only churn is
 * covered too: OnAssetRenamed re-keys the tracked path, and OnAssetUpdated
 * (re-saves of existing assets, including bEnabled flips and edited
 * bindings) removes and re-applies the contribution.
 *
 * Not created on dedicated servers: contributions only feed per-LocalPlayer
 * UGameSettingsSubsystems, and a dedicated server never has LocalPlayers.
 */
UCLASS(MinimalAPI)
class UGameSettingsAssetDiscoverySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UE_API virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
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
	UE_API void OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath);
	UE_API void OnAssetUpdated(const FAssetData& AssetData);

	UE_API bool IsContributionAsset(const FAssetData& AssetData) const;
	static UE_API bool IsEnabledByTag(const FAssetData& AssetData);

	UE_API void TryAdd(const FAssetData& AssetData);
	UE_API void RemoveByPath(const FSoftObjectPath& Path);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGameSettingsContribution>> KnownContributions;

	/** Fast lookup of contribution by asset path for OnAssetRemoved. */
	UPROPERTY(Transient)
	TMap<FSoftObjectPath, TObjectPtr<UGameSettingsContribution>> ContributionsByPath;

	/**
	 * Class-path cache backing IsContributionAsset for assets whose class
	 * isn't loaded. Positive set comes from IAssetRegistry::GetDerivedClassNames
	 * (matches unloaded BP subclasses); the negative set bounds the refresh to
	 * once per unique unknown class path, since GetDerivedClassNames rebuilds
	 * the registry's inheritance buffer on every call. Mutable: lazily filled
	 * from the logically-const IsContributionAsset query.
	 */
	mutable TSet<FTopLevelAssetPath> CachedContributionClassPaths;
	mutable TSet<FTopLevelAssetPath> CachedNonContributionClassPaths;

	FDelegateHandle FilesLoadedHandle;
	FDelegateHandle AssetAddedHandle;
	FDelegateHandle AssetRemovedHandle;
	FDelegateHandle AssetRenamedHandle;
	FDelegateHandle AssetUpdatedHandle;

	bool bRegistryReady = false;
};

#undef UE_API
