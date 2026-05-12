// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingsAssetDiscoverySubsystem.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "GameSettingsContribution.h"
#include "GameSettingsLog.h"
#include "UObject/UObjectIterator.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsAssetDiscoverySubsystem)

namespace
{
	const FName EnabledTagName = GET_MEMBER_NAME_CHECKED(UGameSettingsContribution, bEnabled);

	IAssetRegistry* GetAssetRegistry()
	{
		FAssetRegistryModule& Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		return &Module.Get();
	}
}

void UGameSettingsAssetDiscoverySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	IAssetRegistry* Registry = GetAssetRegistry();
	if (!Registry)
	{
		return;
	}

	AssetAddedHandle = Registry->OnAssetAdded().AddUObject(this, &UGameSettingsAssetDiscoverySubsystem::OnAssetAdded);
	AssetRemovedHandle = Registry->OnAssetRemoved().AddUObject(this, &UGameSettingsAssetDiscoverySubsystem::OnAssetRemoved);

	if (Registry->IsLoadingAssets())
	{
		FilesLoadedHandle = Registry->OnFilesLoaded().AddUObject(this, &UGameSettingsAssetDiscoverySubsystem::OnFilesLoaded);
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
	}

	KnownContributions.Empty();
	ContributionsByPath.Empty();
	bRegistryReady = false;

	Super::Deinitialize();
}

void UGameSettingsAssetDiscoverySubsystem::OnFilesLoaded()
{
	bRegistryReady = true;

	IAssetRegistry* Registry = GetAssetRegistry();
	if (!Registry)
	{
		return;
	}

	FARFilter Filter;
	Filter.bRecursiveClasses = true;
	Filter.ClassPaths.Add(UGameSettingsContribution::StaticClass()->GetClassPathName());
	Filter.bIncludeOnlyOnDiskAssets = false;

	TArray<FAssetData> Found;
	Registry->GetAssets(Filter, Found);

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
	if (!IsContributionAsset(AssetData))
	{
		return;
	}
	RemoveByPath(AssetData.GetSoftObjectPath());
}

bool UGameSettingsAssetDiscoverySubsystem::IsContributionAsset(const FAssetData& AssetData) const
{
	UClass* AssetClass = AssetData.GetClass();
	if (AssetClass)
	{
		return AssetClass->IsChildOf(UGameSettingsContribution::StaticClass());
	}

	// AssetClass can be null until the class has been loaded; fall back to
	// checking the recorded class path against any loaded subclass.
	const FTopLevelAssetPath AssetClassPath = AssetData.AssetClassPath;
	if (AssetClassPath.IsNull())
	{
		return false;
	}
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Candidate = *It;
		if (!Candidate->IsChildOf(UGameSettingsContribution::StaticClass()))
		{
			continue;
		}
		if (Candidate->GetClassPathName() == AssetClassPath)
		{
			return true;
		}
	}
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
	TObjectPtr<UGameSettingsContribution> Removed;
	if (!ContributionsByPath.RemoveAndCopyValue(Path, Removed))
	{
		return;
	}

	KnownContributions.Remove(Removed);

	UE_LOG(LogGameSettings, Verbose, TEXT("GameSettingsAssetDiscoverySubsystem: removed contribution %s"),
		*Path.ToString());

	OnContributionAssetRemoved.Broadcast(Removed);
}
