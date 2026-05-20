// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingCollection.h"
#include "Templates/Casts.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingCollection)

#define LOCTEXT_NAMESPACE "GameSetting"

//--------------------------------------
// UGameSettingCollection
//--------------------------------------

UGameSettingCollection::UGameSettingCollection()
{

}

void UGameSettingCollection::AddSetting(UGameSetting* Setting)
{
#if !UE_BUILD_SHIPPING
	ensureAlwaysMsgf(Setting->GetSettingParent() == nullptr, TEXT("This setting already has a parent!"));
	ensureAlwaysMsgf(!Settings.Contains(Setting), TEXT("This collection already includes this setting!"));
#endif

	Settings.Add(Setting);
	Setting->SetSettingParent(this);

	if (LocalPlayer)
	{
		Setting->Initialize(LocalPlayer);
	}
}

bool UGameSettingCollection::RemoveSetting(UGameSetting* Setting)
{
	if (!Setting || !Settings.Contains(Setting))
	{
		return false;
	}

	Settings.Remove(Setting);
	Setting->SetSettingParent(nullptr);
	return true;
}

TArray<UGameSettingCollection*> UGameSettingCollection::GetChildCollections() const
{
	TArray<UGameSettingCollection*> CollectionSettings;

	for (UGameSetting* ChildSetting : Settings)
	{
		if (UGameSettingCollection* ChildCollection = Cast<UGameSettingCollection>(ChildSetting))
		{
			CollectionSettings.Add(ChildCollection);
		}
	}

	return CollectionSettings;
}

void UGameSettingCollection::GetSettingsForFilter(const FGameSettingFilterState& FilterState, TArray<UGameSetting*>& InOutSettings) const
{
	// Emit children in deterministic order: SortPriority ascending, then
	// SettingId as a stable tiebreak so equal-priority siblings don't depend
	// on asset-discovery arrival order (which is non-deterministic). Sorting
	// at emit time rather than mutating Settings keeps deferred re-parenting
	// simple and treats ordering as the read-time concern it is.
	TArray<UGameSetting*> SortedSettings = Settings;
	SortedSettings.StableSort([](const UGameSetting& A, const UGameSetting& B)
	{
		const int32 PriorityA = A.GetSortPriority();
		const int32 PriorityB = B.GetSortPriority();
		if (PriorityA != PriorityB)
		{
			return PriorityA < PriorityB;
		}
		return A.GetSettingId().ToString() < B.GetSettingId().ToString();
	});

	for (UGameSetting* ChildSetting : SortedSettings)
	{
		// If the child setting is a collection, only add it to the set if it has any visible children.
		if (Cast<UGameSettingCollectionPage>(ChildSetting))
		{
			if (FilterState.DoesSettingPassFilter(*ChildSetting))
			{
				InOutSettings.Add(ChildSetting);
			}
		}
		else if (UGameSettingCollection* ChildCollection = Cast<UGameSettingCollection>(ChildSetting))
		{
			TArray<UGameSetting*> ChildSettings;
			ChildCollection->GetSettingsForFilter(FilterState, ChildSettings);

			if (ChildSettings.Num() > 0)
			{
				// Don't add the root setting to the returned items, it's the container of N-possible 
				// other settings and containers we're actually displaying right now.
				if (!FilterState.IsSettingInRootList(ChildSetting))
				{
					InOutSettings.Add(ChildSetting);
				}

				InOutSettings.Append(ChildSettings);
			}
		}
		else
		{
			if (FilterState.DoesSettingPassFilter(*ChildSetting))
			{
				InOutSettings.Add(ChildSetting);
			}
		}
	}
}

//--------------------------------------
// UGameSettingCollectionPage
//--------------------------------------

UGameSettingCollectionPage::UGameSettingCollectionPage()
{
}

void UGameSettingCollectionPage::OnInitialized()
{
	Super::OnInitialized();

#if !UE_BUILD_SHIPPING
	ensureAlwaysMsgf(!NavigationText.IsEmpty(), TEXT("You must provide a NavigationText for this setting."));
	ensureAlwaysMsgf(!DescriptionRichText.IsEmpty(), TEXT("You must provide a description for new page collections."));
#endif
}

void UGameSettingCollectionPage::GetSettingsForFilter(const FGameSettingFilterState& FilterState, TArray<UGameSetting*>& InOutSettings) const
{
	// If we're including nested pages, call the super and dump them all, otherwise, we pretend we have none for the filtering.
	// because our settings are displayed on another page.
	if (FilterState.bIncludeNestedPages || FilterState.IsSettingInRootList(this))
	{
		Super::GetSettingsForFilter(FilterState, InOutSettings);
	}
}

void UGameSettingCollectionPage::ExecuteNavigation()
{
	OnExecuteNavigationEvent.Broadcast(this);
}

#undef LOCTEXT_NAMESPACE

