// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingRegistry.h"

#include "GameSettingAction.h"
#include "GameSettingCollection.h"
#include "GameSettingsLog.h"
#include "UObject/WeakObjectPtr.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingRegistry)

#define LOCTEXT_NAMESPACE "GameSetting"

//--------------------------------------
// UGameSettingRegistry
//--------------------------------------

UGameSettingRegistry::UGameSettingRegistry()
{
}

void UGameSettingRegistry::Initialize(ULocalPlayer* InLocalPlayer)
{
	OwningLocalPlayer = InLocalPlayer;
	OnInitialize(InLocalPlayer);
}

void UGameSettingRegistry::OnInitialize(ULocalPlayer* InLocalPlayer)
{
	// Default: do nothing. Projects that prefer the Lyra-shape pattern
	// override this to seed their settings; everyone else uses AddSetting /
	// AddTab and lets the registry stay empty until contributors push.
}

void UGameSettingRegistry::Regenerate()
{
	for (UGameSetting* Setting : RegisteredSettings)
	{
		Setting->MarkAsGarbage();
	}
	RegisteredSettings.Reset();
	TopLevelSettings.Reset();
	SettingsByHandle.Reset();

	OnInitialize(OwningLocalPlayer);

	OnStructureChangedEvent.Broadcast(this);
}

bool UGameSettingRegistry::IsFinishedInitializing() const
{
	bool bReady = true;
	for (UGameSetting* Setting : RegisteredSettings)
	{
		if (!Setting->IsReady())
		{
			bReady = false;
			break;
		}
	}

	return bReady;
}

void UGameSettingRegistry::SaveChanges()
{
}

void UGameSettingRegistry::GetSettingsForFilter(const FGameSettingFilterState& FilterState, TArray<UGameSetting*>& InOutSettings)
{
	TArray<UGameSetting*> RootSettings;
	if (FilterState.GetSettingRootList().Num() > 0)
	{
		RootSettings.Append(FilterState.GetSettingRootList());
	}
	else
	{
		RootSettings.Append(TopLevelSettings);
	}

	for (UGameSetting* TopLevelSetting : RootSettings)
	{
		if (const UGameSettingCollection* TopLevelCollection = Cast<UGameSettingCollection>(TopLevelSetting))
		{
			TopLevelCollection->GetSettingsForFilter(FilterState, InOutSettings);
		}
		else
		{
			if (FilterState.DoesSettingPassFilter(*TopLevelSetting))
			{
				InOutSettings.Add(TopLevelSetting);
			}
		}
	}
}

UGameSetting* UGameSettingRegistry::FindSettingByTag(const FGameplayTag& Id) const
{
	if (!Id.IsValid())
	{
		return nullptr;
	}

	for (UGameSetting* Setting : RegisteredSettings)
	{
		if (Setting->GetSettingId() == Id)
		{
			return Setting;
		}
	}

	return nullptr;
}

// --- Contribution API --------------------------------------------------

FGameSettingHandle UGameSettingRegistry::AddTab(UGameSettingCollection* InTab)
{
	if (!InTab)
	{
		UE_LOG(LogGameSettings, Warning, TEXT("AddTab called with null collection"));
		return FGameSettingHandle{};
	}

	const FGameSettingHandle NewHandle = FGameSettingHandle::Generate();
	InTab->SetHandle(NewHandle);

	TopLevelSettings.Add(InTab);
	WireSettingTree(InTab);

	OnStructureChangedEvent.Broadcast(this);
	return NewHandle;
}

FGameSettingHandle UGameSettingRegistry::AddSetting(UGameSetting* InSetting, FGameplayTag ParentTab)
{
	if (!InSetting)
	{
		UE_LOG(LogGameSettings, Warning, TEXT("AddSetting called with null setting"));
		return FGameSettingHandle{};
	}

	const FGameSettingHandle NewHandle = FGameSettingHandle::Generate();
	InSetting->SetHandle(NewHandle);

	// Resolve parent tab if specified; fall back to top-level on miss.
	UGameSettingCollection* ParentCollection = ParentTab.IsValid() ? FindTabByTag(ParentTab) : nullptr;
	if (ParentTab.IsValid() && !ParentCollection)
	{
		UE_LOG(LogGameSettings, Warning, TEXT("AddSetting('%s') requested parent tab '%s' but no such tab is registered; adding at top level."),
			   *InSetting->GetSettingId().ToString(),
			   *ParentTab.ToString());
	}

	if (ParentCollection)
	{
		ParentCollection->AddSetting(InSetting);
	}
	else
	{
		TopLevelSettings.Add(InSetting);
	}

	WireSettingTree(InSetting);

	OnStructureChangedEvent.Broadcast(this);
	return NewHandle;
}

bool UGameSettingRegistry::RemoveByHandle(const FGameSettingHandle& Handle)
{
	if (!Handle.IsValid())
	{
		return false;
	}

	const TWeakObjectPtr<UGameSetting>* Found = SettingsByHandle.Find(Handle);
	UGameSetting* Setting = Found ? Found->Get() : nullptr;
	if (!Setting)
	{
		return false;
	}

	// Detach from parent collection / TopLevelSettings before recursive unregister.
	if (UGameSettingCollection* Parent = Cast<UGameSettingCollection>(Setting->GetSettingParent()))
	{
		Parent->RemoveSetting(Setting);
	}
	TopLevelSettings.Remove(Setting);

	const int32 RemovedCount = UnregisterSettingTree(Setting);
	UE_LOG(LogGameSettings, Verbose, TEXT("RemoveByHandle %s removed %d setting(s)"),
		   *Handle.ToString(), RemovedCount);

	OnStructureChangedEvent.Broadcast(this);
	return true;
}

bool UGameSettingRegistry::RemoveByTag(const FGameplayTag& Id)
{
	UGameSetting* Setting = FindSettingByTag(Id);
	return Setting ? RemoveByHandle(Setting->GetHandle()) : false;
}

UGameSetting* UGameSettingRegistry::FindSettingByHandle(const FGameSettingHandle& Handle) const
{
	if (!Handle.IsValid())
	{
		return nullptr;
	}
	const TWeakObjectPtr<UGameSetting>* Found = SettingsByHandle.Find(Handle);
	return Found ? Found->Get() : nullptr;
}

UGameSettingCollection* UGameSettingRegistry::FindTabByTag(const FGameplayTag& TabId) const
{
	for (UGameSetting* TopLevel : TopLevelSettings)
	{
		if (TopLevel && TopLevel->GetSettingId() == TabId)
		{
			return Cast<UGameSettingCollection>(TopLevel);
		}
	}
	return nullptr;
}

// --- Wiring helpers ---------------------------------------------------

void UGameSettingRegistry::WireSettingTree(UGameSetting* InSetting)
{
	// Every setting (top-level or nested child) gets the registry pointer.
	// Previously only top-level settings were assigned, leaving OwningRegistry
	// null on every child setting.
	InSetting->SetRegistry(this);

	// Generate a handle if the contribution API didn't already.
	if (!InSetting->GetHandle().IsValid())
	{
		InSetting->SetHandle(FGameSettingHandle::Generate());
	}

	InSetting->OnSettingChangedEvent.AddUObject(this, &ThisClass::HandleSettingChanged);
	InSetting->OnSettingAppliedEvent.AddUObject(this, &ThisClass::HandleSettingApplied);
	InSetting->OnSettingEditConditionChangedEvent.AddUObject(this, &ThisClass::HandleSettingEditConditionsChanged);

	if (UGameSettingAction* ActionSetting = Cast<UGameSettingAction>(InSetting))
	{
		ActionSetting->OnExecuteNamedActionEvent.AddUObject(this, &ThisClass::HandleSettingNamedAction);
	}
	else if (UGameSettingCollectionPage* NewPageCollection = Cast<UGameSettingCollectionPage>(InSetting))
	{
		NewPageCollection->OnExecuteNavigationEvent.AddUObject(this, &ThisClass::HandleSettingNavigation);
	}

	// Soft collision policy: warn instead of crashing in shipping. A duplicate
	// SettingId is almost always a contributor-naming bug; surface it without
	// taking the game down.
	if (RegisteredSettings.Contains(InSetting))
	{
		UE_LOG(LogGameSettings, Warning, TEXT("Setting '%s' is being re-registered; skipping."), *InSetting->GetSettingId().ToString());
		return;
	}
	if (InSetting->GetSettingId().IsValid())
	{
		const FGameplayTag IncomingId = InSetting->GetSettingId();
		if (UGameSetting* const* Existing = ObjectPtrDecay(RegisteredSettings).FindByPredicate(
				[IncomingId](UGameSetting* ExistingSetting) { return ExistingSetting && ExistingSetting->GetSettingId() == IncomingId; }))
		{
			UE_LOG(LogGameSettings, Warning,
				   TEXT("SettingId collision: '%s' already registered (existing=%s, incoming=%s). Both will live in the registry; give each setting a distinct SettingId tag."),
				   *IncomingId.ToString(),
				   *(*Existing)->GetClass()->GetName(),
				   *InSetting->GetClass()->GetName());
		}
	}

	RegisteredSettings.Add(InSetting);
	SettingsByHandle.Add(InSetting->GetHandle(), InSetting);

	for (UGameSetting* ChildSetting : InSetting->GetChildSettings())
	{
		WireSettingTree(ChildSetting);
	}
}

int32 UGameSettingRegistry::UnregisterSettingTree(UGameSetting* InSetting)
{
	if (!InSetting)
	{
		return 0;
	}

	int32 RemovedCount = 1;

	// Recurse into children first so handle/list cleanup is bottom-up.
	for (UGameSetting* ChildSetting : InSetting->GetChildSettings())
	{
		RemovedCount += UnregisterSettingTree(ChildSetting);
	}

	InSetting->OnSettingChangedEvent.RemoveAll(this);
	InSetting->OnSettingAppliedEvent.RemoveAll(this);
	InSetting->OnSettingEditConditionChangedEvent.RemoveAll(this);

	if (UGameSettingAction* ActionSetting = Cast<UGameSettingAction>(InSetting))
	{
		ActionSetting->OnExecuteNamedActionEvent.RemoveAll(this);
	}
	else if (UGameSettingCollectionPage* PageCollection = Cast<UGameSettingCollectionPage>(InSetting))
	{
		PageCollection->OnExecuteNavigationEvent.RemoveAll(this);
	}

	SettingsByHandle.Remove(InSetting->GetHandle());
	RegisteredSettings.Remove(InSetting);

	InSetting->SetRegistry(nullptr);
	InSetting->MarkAsGarbage();

	return RemovedCount;
}

void UGameSettingRegistry::HandleSettingApplied(UGameSetting* Setting)
{
	OnSettingApplied(Setting);
}

void UGameSettingRegistry::HandleSettingChanged(UGameSetting* Setting, EGameSettingChangeReason Reason)
{
	OnSettingChangedEvent.Broadcast(Setting, Reason);
}

void UGameSettingRegistry::HandleSettingEditConditionsChanged(UGameSetting* Setting)
{
	OnSettingEditConditionChangedEvent.Broadcast(Setting);
}

void UGameSettingRegistry::HandleSettingNamedAction(UGameSetting* Setting, FGameplayTag GameSettings_Action_Tag)
{
	OnSettingNamedActionEvent.Broadcast(Setting, GameSettings_Action_Tag);
}

void UGameSettingRegistry::HandleSettingNavigation(UGameSetting* Setting)
{
	OnExecuteNavigationEvent.Broadcast(Setting);
}

#undef LOCTEXT_NAMESPACE
