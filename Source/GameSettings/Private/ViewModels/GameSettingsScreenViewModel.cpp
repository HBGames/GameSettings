// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "ViewModels/GameSettingsScreenViewModel.h"

#include "GameSetting.h"
#include "GameSettingAction.h"
#include "GameSettingCollection.h"
#include "GameSettingRegistry.h"
#include "GameSettingValue.h"
#include "GameSettingValueDiscrete.h"
#include "GameSettingValueScalar.h"
#include "GameSettingsSubsystem.h"
#include "GameSettingValueBool.h"
#include "Templates/SubclassOf.h"
#include "ViewModels/GameSettingActionViewModel.h"
#include "ViewModels/GameSettingCollectionViewModel.h"
#include "ViewModels/GameSettingDiscreteViewModel.h"
#include "ViewModels/GameSettingScalarViewModel.h"
#include "ViewModels/GameSettingToggleViewModel.h"
#include "ViewModels/GameSettingViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsScreenViewModel)

void UGameSettingsScreenViewModel::Initialize(UGameSettingsSubsystem* InSettingsSubsystem)
{
	SettingsSubsystem = InSettingsSubsystem;
	if (!SettingsSubsystem)
	{
		return;
	}

	UGameSettingRegistry* Registry = SettingsSubsystem->GetOrCreateRegistry();
	if (!Registry)
	{
		return;
	}

	ChangeTracker.WatchRegistry(Registry);

	// Subscribe to registry events for our own purposes (separate from the tracker).
	Registry->OnStructureChangedEvent.AddUObject(this, &UGameSettingsScreenViewModel::HandleStructureChanged);
	Registry->OnSettingChangedEvent.AddUObject(this, &UGameSettingsScreenViewModel::HandleSettingChanged);

	RebuildTabs();
	RebuildVisibleSettings();
}

void UGameSettingsScreenViewModel::Shutdown()
{
	ChangeTracker.StopWatchingRegistry();

	if (SettingsSubsystem)
	{
		if (UGameSettingRegistry* Registry = SettingsSubsystem->GetRegistry())
		{
			Registry->OnStructureChangedEvent.RemoveAll(this);
			Registry->OnSettingChangedEvent.RemoveAll(this);
		}
	}
}

void UGameSettingsScreenViewModel::BeginDestroy()
{
	Shutdown();
	Super::BeginDestroy();
}

void UGameSettingsScreenViewModel::SetCurrentTab(UGameSettingViewModel* InTab)
{
	if (CurrentTab == InTab)
	{
		return;
	}
	CurrentTab = InTab;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetCurrentTab);

	// Clear the navigation stack and reset to a tab-rooted filter.
	FilterNavigationStack.Reset();
	FilterState = FGameSettingFilterState();
	if (CurrentTab)
	{
		if (UGameSetting* Setting = CurrentTab->GetSetting())
		{
			FilterState.AddSettingToRootList(Setting);
		}
	}
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanPopNavigation);

	RebuildVisibleSettings();

	// Auto-focus the first visible setting on the new tab so the detail view
	// has content immediately. Lyra's panel does the same after tab swap.
	// Clears focus when the tab is empty so the detail panel doesn't show
	// stale data from the previous tab.
	UGameSettingViewModel* FirstVisible = VisibleSettings.Num() > 0 ? VisibleSettings[0] : nullptr;
	SetFocusedSetting(FirstVisible);
}

void UGameSettingsScreenViewModel::SetFocusedSetting(UGameSettingViewModel* InVM)
{
	if (FocusedSetting == InVM)
	{
		return;
	}
	FocusedSetting = InVM;
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetFocusedSetting);
}

void UGameSettingsScreenViewModel::Apply()
{
	if (!ChangeTracker.HaveSettingsBeenChanged())
	{
		return;
	}

	ChangeTracker.ApplyChanges();
	if (SettingsSubsystem)
	{
		if (UGameSettingRegistry* Registry = SettingsSubsystem->GetRegistry())
		{
			Registry->SaveChanges();
		}
	}
	ChangeTracker.ClearDirtyState();
	RefreshDirtyState();
	RefreshResetState();
}

void UGameSettingsScreenViewModel::Cancel()
{
	ChangeTracker.RestoreToInitial();
	RefreshDirtyState();
	RefreshResetState();
}

void UGameSettingsScreenViewModel::ResetToDefaults()
{
	// Reset the visible settings on the current tab. Each ResetToDefault
	// fires OnSettingChangedEvent, which the change tracker turns into a
	// dirty entry - so the user still has to Apply to persist, or Cancel to
	// roll the reset back. Matches Lyra's reset-then-confirm behaviour.
	for (UGameSettingViewModel* VM : VisibleSettings)
	{
		if (!VM)
		{
			continue;
		}
		if (UGameSettingValue* SettingValue = Cast<UGameSettingValue>(VM->GetSetting()))
		{
			SettingValue->ResetToDefault();
		}
	}

	RefreshDirtyState();
	RefreshResetState();
}

void UGameSettingsScreenViewModel::NavigateToTabById(FPrimaryAssetId TabId)
{
	if (!TabId.IsValid())
	{
		return;
	}
	for (UGameSettingViewModel* Tab : Tabs)
	{
		if (Tab && Tab->GetSetting() && Tab->GetSetting()->GetSettingId() == TabId)
		{
			SetCurrentTab(Tab);
			return;
		}
	}
}

void UGameSettingsScreenViewModel::NavigateToSettingById(FPrimaryAssetId SettingId)
{
	if (!SettingsSubsystem || !SettingId.IsValid())
	{
		return;
	}
	UGameSettingRegistry* Registry = SettingsSubsystem->GetRegistry();
	if (!Registry)
	{
		return;
	}
	UGameSetting* Setting = Registry->FindSettingById(SettingId);
	if (!Setting)
	{
		return;
	}

	FilterNavigationStack.Push(FilterState);
	FilterState = FGameSettingFilterState();
	FilterState.AddSettingToRootList(Setting);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanPopNavigation);

	RebuildVisibleSettings();
}

void UGameSettingsScreenViewModel::PopNavigation()
{
	if (FilterNavigationStack.Num() == 0)
	{
		return;
	}
	FilterState = FilterNavigationStack.Pop();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanPopNavigation);

	RebuildVisibleSettings();
}

void UGameSettingsScreenViewModel::RebuildTabs()
{
	Tabs.Reset();

	if (!SettingsSubsystem)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetTabs);
		return;
	}
	UGameSettingRegistry* Registry = SettingsSubsystem->GetRegistry();
	if (!Registry)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetTabs);
		return;
	}

	// TopLevelSettings is the source of truth for tabs.
	FGameSettingFilterState EmptyFilter;
	TArray<UGameSetting*> RootSettings;
	Registry->GetSettingsForFilter(EmptyFilter, RootSettings);
	// GetSettingsForFilter walks into collections; we want the top-level
	// containers themselves. The registry's direct top-level array is
	// protected, so walk what GetSettingsForFilter returns and dedupe up
	// to the parent collection chain.
	TSet<UGameSetting*> SeenTabs;
	for (UGameSetting* Setting : RootSettings)
	{
		if (!Setting)
		{
			continue;
		}
		// Find the top-level ancestor.
		UGameSetting* Ancestor = Setting;
		while (UGameSetting* Parent = Ancestor->GetSettingParent())
		{
			Ancestor = Parent;
		}
		if (UGameSettingCollection* Tab = Cast<UGameSettingCollection>(Ancestor))
		{
			if (!SeenTabs.Contains(Tab))
			{
				SeenTabs.Add(Tab);
				if (UGameSettingViewModel* VM = GetOrCreateViewModelFor(Tab))
				{
					Tabs.Add(VM);
				}
			}
		}
	}

	// Order tabs deterministically by their setting's SortPriority, SettingId
	// tiebreak - same contract as UGameSettingCollection child ordering, so
	// tab order doesn't depend on asset-discovery arrival order.
	Tabs.StableSort([](const UGameSettingViewModel& A, const UGameSettingViewModel& B)
		{
			const UGameSetting* SettingA = A.GetSetting();
			const UGameSetting* SettingB = B.GetSetting();
			if (!SettingA || !SettingB)
			{
				return SettingA != nullptr;
			}
			const int32 PriorityA = SettingA->GetSortPriority();
			const int32 PriorityB = SettingB->GetSortPriority();
			if (PriorityA != PriorityB)
			{
				return PriorityA < PriorityB;
			}
			return SettingA->GetSettingId().ToString() < SettingB->GetSettingId().ToString();
		});

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetTabs);

	// Default focus the first tab if nothing's selected.
	if (!CurrentTab && Tabs.Num() > 0)
	{
		SetCurrentTab(Tabs[0]);
	}
}

void UGameSettingsScreenViewModel::RebuildVisibleSettings()
{
	VisibleSettings.Reset();

	if (!SettingsSubsystem)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetVisibleSettings);
		return;
	}
	UGameSettingRegistry* Registry = SettingsSubsystem->GetRegistry();
	if (!Registry)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetVisibleSettings);
		return;
	}

	TArray<UGameSetting*> Flat;
	Registry->GetSettingsForFilter(FilterState, Flat);

	VisibleSettings.Reserve(Flat.Num());
	for (UGameSetting* Setting : Flat)
	{
		if (UGameSettingViewModel* VM = GetOrCreateViewModelFor(Setting))
		{
			VisibleSettings.Add(VM);
		}
	}

	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetVisibleSettings);

	// The visible set drives reset eligibility; recompute against the new list
	// (e.g. tab swap may surface settings that are off their defaults).
	RefreshResetState();
}

void UGameSettingsScreenViewModel::RefreshDirtyState()
{
	const bool bNewDirty = ChangeTracker.HaveSettingsBeenChanged();
	if (bIsDirty != bNewDirty)
	{
		bIsDirty = bNewDirty;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsDirty);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanApply);
	}
}

void UGameSettingsScreenViewModel::RefreshResetState()
{
	bool bNewCanReset = false;
	for (const UGameSettingViewModel* VM : VisibleSettings)
	{
		if (!VM)
		{
			continue;
		}
		if (const UGameSettingValue* SettingValue = Cast<UGameSettingValue>(VM->GetSetting()))
		{
			if (SettingValue->IsResettableToDefault())
			{
				bNewCanReset = true;
				break;
			}
		}
	}

	if (bCanResetToDefaults != bNewCanReset)
	{
		bCanResetToDefaults = bNewCanReset;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanResetToDefaults);
	}
}

UGameSettingViewModel* UGameSettingsScreenViewModel::GetOrCreateViewModelFor(UGameSetting* Setting)
{
	if (!Setting)
	{
		return nullptr;
	}

	for (UGameSettingViewModel* Existing : ObjectPtrDecay(AllViewModels))
	{
		if (Existing && Existing->GetSetting() == Setting)
		{
			return Existing;
		}
	}

	TSubclassOf<UGameSettingViewModel> VMClass = ResolveViewModelClass(Setting);
	if (!VMClass)
	{
		return nullptr;
	}

	UGameSettingViewModel* VM = NewObject<UGameSettingViewModel>(this, VMClass);
	VM->SetSetting(Setting);
	AllViewModels.Add(VM);
	return VM;
}

TSubclassOf<UGameSettingViewModel> UGameSettingsScreenViewModel::ResolveViewModelClass(UGameSetting* Setting) const
{
	// Default mapping. Project subclasses can override per setting class.
	// Bool is checked before Discrete (it's a peer type now, but listing it
	// here is a reminder that toggles get their own VM rather than falling
	// through to the option-list VM).
	if (Cast<UGameSettingValueBool>(Setting)) return UGameSettingToggleViewModel::StaticClass();
	if (Cast<UGameSettingValueScalar>(Setting)) return UGameSettingScalarViewModel::StaticClass();
	if (Cast<UGameSettingValueDiscrete>(Setting)) return UGameSettingDiscreteViewModel::StaticClass();
	if (Cast<UGameSettingAction>(Setting)) return UGameSettingActionViewModel::StaticClass();
	if (Cast<UGameSettingCollection>(Setting)) return UGameSettingCollectionViewModel::StaticClass();
	return UGameSettingViewModel::StaticClass();
}

void UGameSettingsScreenViewModel::HandleStructureChanged(UGameSettingRegistry* Registry)
{
	// Evict VMs whose underlying setting is gone (the setting goes null
	// because TObjectPtr clears garbage refs after MarkAsGarbage).
	ToRawPtr(MutableView(AllViewModels))->RemoveAll(
		[](const UGameSettingViewModel* VM)
			{
				return !VM || !VM->GetSetting();
			});

	RebuildTabs();
	RebuildVisibleSettings();
}

void UGameSettingsScreenViewModel::HandleSettingChanged(UGameSetting* /*Setting*/, EGameSettingChangeReason /*Reason*/)
{
	// Tracker has already updated; re-broadcast our IsDirty if it flipped.
	RefreshDirtyState();
	RefreshResetState();
}
