// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingsAssetDiscoverySubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/GameInstance.h"
#include "GameSettingsContribution.h"
#include "GameSettingsLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsAssetDiscoverySubsystem)

namespace
{
	const FName EnabledTagName = GET_MEMBER_NAME_CHECKED(UGameSettingsContribution, bEnabled);

	// Checked load: never null. Deinitialize deliberately doesn't use this.
	// Tt goes through IAssetRegistry::Get() so teardown can't force-load the
	// module during shutdown.
	IAssetRegistry& GetAssetRegistry()
	{
		return FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	}
}

bool UGameSettingsAssetDiscoverySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// Dedicated servers never have LocalPlayers, so there is no consumer for
	// discovered contributions; skip the subsystem (and its asset force-loads).
	if (CastChecked<UGameInstance>(Outer)->IsDedicatedServerInstance())
	{
		return false;
	}
	return Super::ShouldCreateSubsystem(Outer);
}

void UGameSettingsAssetDiscoverySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IAssetRegistry& Registry = GetAssetRegistry();

	AssetAddedHandle = Registry.OnAssetAdded().AddUObject(this, &UGameSettingsAssetDiscoverySubsystem::OnAssetAdded);
	AssetRemovedHandle = Registry.OnAssetRemoved().AddUObject(this, &UGameSettingsAssetDiscoverySubsystem::OnAssetRemoved);
	// Editor churn: renames broadcast only OnAssetRenamed (no add/remove pair),
	// re-saves of existing assets broadcast OnAssetUpdated (not OnAssetAdded).
	// The delegates exist on all targets; they simply never fire in cooked builds.
	AssetRenamedHandle = Registry.OnAssetRenamed().AddUObject(this, &UGameSettingsAssetDiscoverySubsystem::OnAssetRenamed);
	AssetUpdatedHandle = Registry.OnAssetUpdated().AddUObject(this, &UGameSettingsAssetDiscoverySubsystem::OnAssetUpdated);

	if (Registry.IsLoadingAssets())
	{
		FilesLoadedHandle = Registry.OnFilesLoaded().AddUObject(this, &UGameSettingsAssetDiscoverySubsystem::OnFilesLoaded);
	}
	else
	{
		OnFilesLoaded();
	}
}

void UGameSettingsAssetDiscoverySubsystem::Deinitialize()
{
	if (IAssetRegistry* Registry = IAssetRegistry::Get())
	{
		if (FilesLoadedHandle.IsValid())
		{
			Registry->OnFilesLoaded().Remove(FilesLoadedHandle);
			FilesLoadedHandle.Reset();
		}
		if (AssetAddedHandle.IsValid())
		{
			Registry->OnAssetAdded().Remove(AssetAddedHandle);
			AssetAddedHandle.Reset();
		}
		if (AssetRemovedHandle.IsValid())
		{
			Registry->OnAssetRemoved().Remove(AssetRemovedHandle);
			AssetRemovedHandle.Reset();
		}
		if (AssetRenamedHandle.IsValid())
		{
			Registry->OnAssetRenamed().Remove(AssetRenamedHandle);
			AssetRenamedHandle.Reset();
		}
		if (AssetUpdatedHandle.IsValid())
		{
			Registry->OnAssetUpdated().Remove(AssetUpdatedHandle);
			AssetUpdatedHandle.Reset();
		}
	}

	KnownContributions.Empty();
	ContributionsByPath.Empty();
	CachedContributionClassPaths.Empty();
	CachedNonContributionClassPaths.Empty();
	bRegistryReady = false;

	Super::Deinitialize();
}

void UGameSettingsAssetDiscoverySubsystem::OnFilesLoaded()
{
	bRegistryReady = true;

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.ClassPaths.Add(UGameSettingsContribution::StaticClass()->GetClassPathName());
	Filter.bIncludeOnlyOnDiskAssets = false;

	TArray<FAssetData> Found;
	GetAssetRegistry().GetAssets(Filter, Found);

	for (const FAssetData& Data : Found)
	{
		TryAdd(Data);
	}

	UE_LOG(LogGameSettings, Verbose, TEXT("GameSettingsAssetDiscoverySubsystem: scanned %d contribution asset(s), %d enabled."),
		Found.Num(),
		KnownContributions.Num());
}

void UGameSettingsAssetDiscoverySubsystem::OnAssetAdded(const FAssetData& AssetData)
{
	if (!bRegistryReady)
	{
		return;
	}
	if (!IsContributionAsset(AssetData))
	{
		return;
	}
	TryAdd(AssetData);
}

void UGameSettingsAssetDiscoverySubsystem::OnAssetRemoved(const FAssetData& AssetData)
{
	// No class filter: if the class can't be resolved any more (plugin
	// unloaded, BP deleted), a tracked asset must still be untracked. The map
	// lookup inside RemoveByPath is the filter.
	RemoveByPath(AssetData.GetSoftObjectPath());
}

void UGameSettingsAssetDiscoverySubsystem::OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath)
{
	const FSoftObjectPath OldPath(OldObjectPath);
	if (UGameSettingsContribution* Tracked = ContributionsByPath.FindRef(OldPath))
	{
		// Re-key only; the loaded contribution object (and its applied
		// settings) is unchanged by a rename.
		ContributionsByPath.Remove(OldPath);
		ContributionsByPath.Add(AssetData.GetSoftObjectPath(), Tracked);

		UE_LOG(LogGameSettings, Verbose, TEXT("GameSettingsAssetDiscoverySubsystem: contribution renamed %s -> %s"),
			*OldPath.ToString(),
			*AssetData.GetSoftObjectPath().ToString());
		return;
	}

	// Untracked under the old path (e.g. previously disabled); evaluate the
	// new path as a potential add.
	OnAssetAdded(AssetData);
}

void UGameSettingsAssetDiscoverySubsystem::OnAssetUpdated(const FAssetData& AssetData)
{
	const FSoftObjectPath Path = AssetData.GetSoftObjectPath();
	if (ContributionsByPath.Contains(Path))
	{
		// Tracked asset re-saved: drop the applied settings and re-apply the
		// current state through the normal remove/add flow. TryAdd re-checks
		// bEnabled, so a true->false flip removes without re-adding.
		RemoveByPath(Path);
		if (IsContributionAsset(AssetData))
		{
			TryAdd(AssetData);
		}
		return;
	}

	// Untracked asset that may now qualify (e.g. bEnabled flipped to true).
	OnAssetAdded(AssetData);
}

bool UGameSettingsAssetDiscoverySubsystem::IsContributionAsset(const FAssetData& AssetData) const
{
	UClass* AssetClass = AssetData.GetClass();
	if (AssetClass)
	{
		return AssetClass->IsChildOf(UGameSettingsContribution::StaticClass());
	}

	// AssetClass is null until the class has been loaded; match the recorded
	// class path against the registry's derived-class names instead, which
	// also covers unloaded BP subclasses (same coverage as the startup
	// GetAssets(bRecursiveClasses) scan).
	const FTopLevelAssetPath AssetClassPath = AssetData.AssetClassPath;
	if (AssetClassPath.IsNull())
	{
		return false;
	}
	if (CachedContributionClassPaths.Contains(AssetClassPath))
	{
		return true;
	}
	if (CachedNonContributionClassPaths.Contains(AssetClassPath))
	{
		return false;
	}

	// Unknown class path: refresh the derived set (new subclasses can appear
	// after startup, e.g. from a GFP mount) and cache the verdict either way
	// so each unique class path triggers at most one refresh.
	CachedContributionClassPaths.Reset();
	GetAssetRegistry().GetDerivedClassNames(
		{ UGameSettingsContribution::StaticClass()->GetClassPathName() },
		TSet<FTopLevelAssetPath>(),
		CachedContributionClassPaths);

	if (CachedContributionClassPaths.Contains(AssetClassPath))
	{
		return true;
	}
	CachedNonContributionClassPaths.Add(AssetClassPath);
	return false;
}

bool UGameSettingsAssetDiscoverySubsystem::IsEnabledByTag(const FAssetData& AssetData)
{
	bool bEnabledValue = true;
	if (AssetData.GetTagValue(EnabledTagName, bEnabledValue))
	{
		return bEnabledValue;
	}
	// No tag yet (asset saved before bEnabled was introduced); default true.
	return true;
}

void UGameSettingsAssetDiscoverySubsystem::TryAdd(const FAssetData& AssetData)
{
	const FSoftObjectPath Path = AssetData.GetSoftObjectPath();
	if (ContributionsByPath.Contains(Path))
	{
		return;
	}

	if (!IsEnabledByTag(AssetData))
	{
		UE_LOG(LogGameSettings, Verbose, TEXT("GameSettingsAssetDiscoverySubsystem: skipping disabled contribution asset %s"),
			*Path.ToString());
		return;
	}

	UObject* Loaded = AssetData.GetAsset();
	UGameSettingsContribution* Contribution = Cast<UGameSettingsContribution>(Loaded);
	if (!Contribution)
	{
		return;
	}

	// Defensive: bEnabled may have changed between the registry tag write and
	// the load (editor edit + save races). Trust the in-memory value.
	if (!Contribution->bEnabled)
	{
		return;
	}

	KnownContributions.Add(Contribution);
	ContributionsByPath.Add(Path, Contribution);

	UE_LOG(LogGameSettings, Verbose, TEXT("GameSettingsAssetDiscoverySubsystem: discovered contribution %s"),
		*Path.ToString());

	OnContributionAssetReady.Broadcast(Contribution);
}

void UGameSettingsAssetDiscoverySubsystem::RemoveByPath(const FSoftObjectPath& Path)
{
	UGameSettingsContribution* Removed;
	if (!ToRawPtr(MutableView(ContributionsByPath))->RemoveAndCopyValue(Path, Removed))
	{
		return;
	}

	KnownContributions.Remove(Removed);

	UE_LOG(LogGameSettings, Verbose, TEXT("GameSettingsAssetDiscoverySubsystem: removed contribution %s"),
		*Path.ToString());

	OnContributionAssetRemoved.Broadcast(Removed);
}
