// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingRegistry.h"

#include "DataSource/GameSettingDataSource.h"
#include "EditCondition/GameSettingEditConditionSpec.h"
#include "GameSettingAction.h"
#include "GameSettingCollection.h"
#include "GameSettingFilterState.h"
#include "GameSettingValue.h"
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

	// Any settings registered before we learned our LocalPlayer (e.g.
	// contributions applied prior to this call) never got Initialize() - and
	// therefore never ran Startup()/OnInitialized(). Catch them up now.
	// UGameSetting::Initialize early-returns when the LocalPlayer already
	// matches, so this is idempotent for anything WireSettingTree already did.
	if (InLocalPlayer)
	{
		// Copy: Initialize can mutate the tree (child init / deferred flushes).
		TArray<TObjectPtr<UGameSetting>> SettingsSnapshot = RegisteredSettings;
		for (UGameSetting* Setting : SettingsSnapshot)
		{
			if (Setting)
			{
				Setting->Initialize(InLocalPlayer);
			}
		}
	}
}

void UGameSettingRegistry::OnInitialize(ULocalPlayer* InLocalPlayer)
{
	// Default: do nothing. Projects that prefer the Lyra-shape pattern
	// override this to seed their settings; everyone else uses AddSetting /
	// AddCollection and lets the registry stay empty until contributors push.
}

void UGameSettingRegistry::Regenerate()
{
	// Surface any deferred placements that never resolved before we drop them.
	// A leftover entry here means a row referenced a tab/section that no
	// contribution ever provided - almost always a typo on ParentContainer.
	for (const FGameSettingDeferredPlacement& Orphan : DeferredPlacements)
	{
		if (UGameSetting* OrphanSetting = Orphan.Setting)
		{
			UE_LOG(LogGameSettings, Warning,
				TEXT("Regenerate: setting '%s' never resolved its parent container '%s'; check that a contribution provides that id."),
				*OrphanSetting->GetSettingId().ToString(),
				*Orphan.ParentContainerId.ToString());
		}
	}
	DeferredPlacements.Reset();

	// Surface orphaned edit conditions (referenced a target id no contribution
	// ever provided - typo or unloaded GFP).
	for (const FGameSettingDeferredEditCondition& Orphan : DeferredEditConditions)
	{
		if (UGameSetting* OrphanOwner = Orphan.Owner)
		{
			FString MissingList;
			for (const FPrimaryAssetId& Id : Orphan.MissingTargets)
			{
				if (!MissingList.IsEmpty()) MissingList.Append(TEXT(", "));
				MissingList.Append(Id.ToString());
			}
			UE_LOG(LogGameSettings, Warning,
				TEXT("Regenerate: setting '%s' has an unresolved edit condition '%s' (missing targets: %s)."),
				*OrphanOwner->GetSettingId().ToString(),
				Orphan.Spec ? *Orphan.Spec->GetClass()->GetName() : TEXT("<null spec>"),
				MissingList.IsEmpty() ? TEXT("<none>") : *MissingList);
		}
	}
	DeferredEditConditions.Reset();
	AppliedEditConditions.Reset();

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
	ULocalPlayer* LP = OwningLocalPlayer;

	// Collect the distinct backing stores referenced by any registered value
	// setting and flush each exactly once. De-dup by GetPersistKey so e.g.
	// every GameUserSettings-bound setting collapses into a single
	// ApplySettings call rather than one per setting.
	//
	// Note this persists the whole store, not just the dirty subset - the same
	// behaviour as Lyra's ULyraGameSettingRegistry::SaveChanges, which calls
	// ApplySettings on the entire LocalSettings / SharedSettings objects. By
	// the time this runs the change tracker has already cleared its dirty set
	// anyway, so a per-store flush is both correct and Lyra-faithful.
	TMap<FString, TSharedPtr<FGameSettingDataSource>> StoresToPersist;
	for (UGameSetting* Setting : RegisteredSettings)
	{
		UGameSettingValue* Value = Cast<UGameSettingValue>(Setting);
		if (!Value)
		{
			continue;
		}

		TSharedPtr<FGameSettingDataSource> DataSource = Value->GetPersistableDataSource();
		if (!DataSource.IsValid())
		{
			continue;
		}

		const FString Key = DataSource->GetPersistKey();
		if (Key.IsEmpty())
		{
			continue;
		}

		StoresToPersist.FindOrAdd(Key) = DataSource;
	}

	for (const TPair<FString, TSharedPtr<FGameSettingDataSource>>& Pair : StoresToPersist)
	{
		Pair.Value->Persist(LP);
	}
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

UGameSetting* UGameSettingRegistry::FindSettingById(const FPrimaryAssetId& Id) const
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

FGameSettingHandle UGameSettingRegistry::AddCollection(UGameSettingCollection* InCollection, FPrimaryAssetId ParentContainerId)
{
	if (!InCollection)
	{
		UE_LOG(LogGameSettings, Warning, TEXT("AddCollection called with null collection"));
		return FGameSettingHandle{};
	}

	const FGameSettingHandle NewHandle = FGameSettingHandle::Generate();
	InCollection->SetHandle(NewHandle);

	// Register the collection in our bookkeeping immediately so handle / id
	// lookups work even before it lands at its final position in the tree.
	WireSettingTree(InCollection);

	UGameSettingCollection* ParentCollection = ParentContainerId.IsValid() ? FindCollectionById(ParentContainerId) : nullptr;

	if (ParentCollection)
	{
		ParentCollection->AddSetting(InCollection);
	}
	else if (ParentContainerId.IsValid())
	{
		// Parent specified but not yet registered. Hold the collection in the
		// deferred queue and let FlushDeferredPlacements re-parent it once the
		// parent arrives. The collection stays in RegisteredSettings, so id
		// lookups against it succeed - which means rows that want to nest
		// under THIS collection can still find it as their parent.
		DeferredPlacements.Add({ InCollection, ParentContainerId });
		UE_LOG(LogGameSettings, Verbose,
			TEXT("AddCollection('%s') deferred: parent '%s' not yet registered."),
			*InCollection->GetSettingId().ToString(),
			*ParentContainerId.ToString());
	}
	else
	{
		// No parent specified - this is a top-level tab page.
		TopLevelSettings.Add(InCollection);
	}

	// The new collection might unblock previously-deferred children, and the
	// new id may resolve a deferred edit condition that was waiting for it.
	FlushDeferredPlacements();
	FlushDeferredEditConditions();

	OnStructureChangedEvent.Broadcast(this);
	return NewHandle;
}

FGameSettingHandle UGameSettingRegistry::AddSetting(UGameSetting* InSetting, FPrimaryAssetId ParentContainerId)
{
	if (!InSetting)
	{
		UE_LOG(LogGameSettings, Warning, TEXT("AddSetting called with null setting"));
		return FGameSettingHandle{};
	}

	// Collections route through AddCollection so they participate as parents
	// for further deferred placements. Forwarding here keeps callers from
	// having to know about the distinction.
	if (UGameSettingCollection* AsCollection = Cast<UGameSettingCollection>(InSetting))
	{
		return AddCollection(AsCollection, ParentContainerId);
	}

	const FGameSettingHandle NewHandle = FGameSettingHandle::Generate();
	InSetting->SetHandle(NewHandle);

	// Register immediately so handle lookups work even while the setting is
	// in the deferred queue.
	WireSettingTree(InSetting);

	UGameSettingCollection* ParentCollection = ParentContainerId.IsValid() ? FindCollectionById(ParentContainerId) : nullptr;

	if (ParentCollection)
	{
		ParentCollection->AddSetting(InSetting);
	}
	else if (ParentContainerId.IsValid())
	{
		// Parent specified but not registered yet. Queue and wait.
		DeferredPlacements.Add({ InSetting, ParentContainerId });
		UE_LOG(LogGameSettings, Verbose,
			TEXT("AddSetting('%s') deferred: parent '%s' not yet registered."),
			*InSetting->GetSettingId().ToString(),
			*ParentContainerId.ToString());
	}
	else
	{
		// No parent specified - top-level row.
		TopLevelSettings.Add(InSetting);
	}

	// The new id may unblock a deferred edit condition that was waiting for it.
	FlushDeferredEditConditions();

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

	// Detach from wherever the setting currently lives: parent collection,
	// the top-level array, OR the deferred-placement queue (in which case it
	// has no parent and isn't top-level either).
	if (UGameSettingCollection* Parent = Cast<UGameSettingCollection>(Setting->GetSettingParent()))
	{
		Parent->RemoveSetting(Setting);
	}
	TopLevelSettings.Remove(Setting);
	RemoveFromDeferred(Setting);

	const int32 RemovedCount = UnregisterSettingTree(Setting);
	UE_LOG(LogGameSettings, Verbose, TEXT("RemoveByHandle %s removed %d setting(s)"),
		   *Handle.ToString(), RemovedCount);

	OnStructureChangedEvent.Broadcast(this);
	return true;
}

bool UGameSettingRegistry::RemoveById(const FPrimaryAssetId& Id)
{
	UGameSetting* Setting = FindSettingById(Id);
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

UGameSettingCollection* UGameSettingRegistry::FindCollectionById(const FPrimaryAssetId& Id) const
{
	if (!Id.IsValid())
	{
		return nullptr;
	}

	// RegisteredSettings holds every setting in the tree including nested
	// collections, so a single linear walk handles both top-level tabs and
	// nested sections. Linear is fine: registered counts in practice are in
	// the dozens to low hundreds.
	for (UGameSetting* Setting : RegisteredSettings)
	{
		if (Setting && Setting->GetSettingId() == Id)
		{
			return Cast<UGameSettingCollection>(Setting);
		}
	}
	return nullptr;
}

void UGameSettingRegistry::FlushDeferredPlacements()
{
	// Loop until no further progress: a section whose tab just arrived may
	// unblock rows that were waiting for that section, which may in turn
	// unblock further nested entries. Bounded by the deferred-array size, so
	// the worst-case cost is O(N^2) over a small N.
	bool bMadeProgress = true;
	while (bMadeProgress)
	{
		bMadeProgress = false;
		for (int32 Index = DeferredPlacements.Num() - 1; Index >= 0; --Index)
		{
			const FGameSettingDeferredPlacement& Entry = DeferredPlacements[Index];
			UGameSettingCollection* Parent = FindCollectionById(Entry.ParentContainerId);
			if (!Parent || Parent == Entry.Setting)
			{
				// Parent not registered yet, or the parent IS this entry (self-reference; treat as orphan).
				continue;
			}

			UGameSetting* PendingSetting = Entry.Setting;
			DeferredPlacements.RemoveAt(Index);

			if (PendingSetting)
			{
				Parent->AddSetting(PendingSetting);
				bMadeProgress = true;
			}
		}
	}
}

void UGameSettingRegistry::RemoveFromDeferred(UGameSetting* InSetting)
{
	if (!InSetting)
	{
		return;
	}
	for (int32 Index = DeferredPlacements.Num() - 1; Index >= 0; --Index)
	{
		if (DeferredPlacements[Index].Setting == InSetting)
		{
			DeferredPlacements.RemoveAt(Index);
		}
	}
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

	// Drive the setting's lifecycle here, the moment it joins the registry,
	// rather than relying on a parent collection already having a LocalPlayer
	// (the contribution model never seeds that the way Lyra's hand-built
	// registries do via Screen->Initialize before AddSetting). Without this,
	// Startup()/OnInitialized() never run for contribution settings - harmless
	// for the ones whose options/values come from Apply or a lazy getter, but
	// fatal for settings that build their state in OnInitialized (e.g. the
	// language picker enumerating cultures). Contribution Apply fully
	// configures the setting (getter/setter/default/options) before calling
	// AddSetting, so the data sources are ready by the time we get here.
	// Idempotent: UGameSetting::Initialize early-returns once the LocalPlayer
	// matches, so the later parent-collection AddSetting cascade is a no-op.
	if (OwningLocalPlayer)
	{
		InSetting->Initialize(OwningLocalPlayer);
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
		const FPrimaryAssetId IncomingId = InSetting->GetSettingId();
		if (UGameSetting* const* Existing = ObjectPtrDecay(RegisteredSettings).FindByPredicate(
				[IncomingId](UGameSetting* ExistingSetting) { return ExistingSetting && ExistingSetting->GetSettingId() == IncomingId; }))
		{
			UE_LOG(LogGameSettings, Warning,
				   TEXT("SettingId collision: '%s' already registered (existing=%s, incoming=%s). Both will live in the registry; give each setting a distinct contribution asset."),
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

	// Drop any condition wires that pointed at this setting before its memory
	// is reclaimed - prevents stale TWeakObjectPtr targets from sticking around
	// in spec lambdas after a GameFeature unload.
	CleanupEditConditionsForRemovedTarget(InSetting);

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

// --- Edit-condition specs ---------------------------------------------

void UGameSettingRegistry::ApplyEditConditionSpecs(UGameSetting* Owner,
	const TArray<TObjectPtr<UGameSettingEditConditionSpec>>& Specs)
{
	if (!Owner || Specs.IsEmpty())
	{
		return;
	}

	TArray<FAppliedEditConditionRecord>& OwnerRecords = AppliedEditConditions.FindOrAdd(Owner);

	for (const TObjectPtr<UGameSettingEditConditionSpec>& SpecPtr : Specs)
	{
		UGameSettingEditConditionSpec* Spec = SpecPtr.Get();
		if (!Spec)
		{
			continue;
		}

		// Idempotent re-apply: skip if this exact spec object is already wired
		// onto this owner. Hot-reload of a contribution that mutates the spec
		// in place reads live data through the lambda; recreating the
		// contribution drops the owner side first, so the map self-recovers.
		const bool bAlreadyApplied = OwnerRecords.ContainsByPredicate(
			[Spec](const FAppliedEditConditionRecord& Rec) { return Rec.Spec.Get() == Spec; });
		if (bAlreadyApplied)
		{
			continue;
		}

		// Check every dependency target. Anything missing pushes onto the
		// deferred queue rather than blocking install on the satisfied targets.
		TArray<FPrimaryAssetId> DepIds;
		Spec->GetSettingDependencies(DepIds);

		TArray<FPrimaryAssetId> MissingTargets;
		for (const FPrimaryAssetId& Id : DepIds)
		{
			if (Id.IsValid() && FindSettingById(Id) == nullptr)
			{
				MissingTargets.Add(Id);
			}
		}

		if (MissingTargets.IsEmpty())
		{
			InstallSpec(Owner, Spec);
		}
		else
		{
			FGameSettingDeferredEditCondition Pending;
			Pending.Owner          = Owner;
			Pending.Spec           = Spec;
			Pending.MissingTargets = MoveTemp(MissingTargets);
			DeferredEditConditions.Add(MoveTemp(Pending));
		}
	}
}

void UGameSettingRegistry::InstallSpec(UGameSetting* Owner, UGameSettingEditConditionSpec* Spec)
{
	if (!Owner || !Spec)
	{
		return;
	}

	TSharedPtr<FGameSettingEditCondition> Condition = Spec->BuildCondition(*this, *Owner);
	if (!Condition.IsValid())
	{
		// Spec opted out (rare; mostly defensive against BP subclasses).
		return;
	}

	Owner->AddEditCondition(Condition.ToSharedRef());

	FAppliedEditConditionRecord Record;
	Record.Spec      = Spec;
	Record.Condition = Condition;

	TArray<FPrimaryAssetId> DepIds;
	Spec->GetSettingDependencies(DepIds);
	for (const FPrimaryAssetId& DepId : DepIds)
	{
		if (UGameSetting* DepSetting = FindSettingById(DepId))
		{
			Owner->AddEditDependency(DepSetting);
			Record.Targets.Add(DepSetting);
		}
	}

	AppliedEditConditions.FindOrAdd(Owner).Add(MoveTemp(Record));

	// Recompute now so a setting applied after its targets doesn't sit at
	// default-enabled until the user touches something. Notify subscribers so
	// the view-model mirror picks up the new state.
	Owner->RefreshEditableState(/*bNotifyEditConditionsChanged=*/true);
}

void UGameSettingRegistry::FlushDeferredEditConditions()
{
	if (DeferredEditConditions.IsEmpty() && AppliedEditConditions.IsEmpty())
	{
		return;
	}

	// Compact stale weak-key entries first so per-owner record arrays don't
	// grow unbounded after repeated GFP unloads.
	for (auto It = AppliedEditConditions.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	// Loop until quiescent: a freshly-installed spec may itself satisfy a
	// later-queued spec's dependency through cascade effects.
	bool bMadeProgress = true;
	while (bMadeProgress)
	{
		bMadeProgress = false;
		for (int32 Index = DeferredEditConditions.Num() - 1; Index >= 0; --Index)
		{
			FGameSettingDeferredEditCondition& Entry = DeferredEditConditions[Index];

			Entry.MissingTargets.RemoveAll([this](const FPrimaryAssetId& Id)
			{
				return FindSettingById(Id) != nullptr;
			});

			if (!Entry.MissingTargets.IsEmpty())
			{
				continue;
			}

			UGameSetting* Owner = Entry.Owner;
			UGameSettingEditConditionSpec* Spec = Entry.Spec;
			DeferredEditConditions.RemoveAt(Index);

			if (Owner && Spec)
			{
				InstallSpec(Owner, Spec);
				bMadeProgress = true;
			}
		}
	}
}

void UGameSettingRegistry::CleanupEditConditionsForRemovedTarget(UGameSetting* RemovedTarget)
{
	if (!RemovedTarget)
	{
		return;
	}

	// Drop any pending deferred entry pointing at this owner specifically -
	// the owner is going away (recursive UnregisterSettingTree call) so its
	// queued specs can't ever install. Owner-side targets are handled below.
	DeferredEditConditions.RemoveAll([RemovedTarget](const FGameSettingDeferredEditCondition& Entry)
	{
		return Entry.Owner == RemovedTarget;
	});

	for (auto It = AppliedEditConditions.CreateIterator(); It; ++It)
	{
		UGameSetting* Owner = It.Key().Get();
		if (!Owner)
		{
			It.RemoveCurrent();
			continue;
		}

		TArray<FAppliedEditConditionRecord>& Records = It.Value();
		bool bOwnerNeedsRefresh = false;

		for (int32 RecIndex = Records.Num() - 1; RecIndex >= 0; --RecIndex)
		{
			FAppliedEditConditionRecord& Record = Records[RecIndex];

			const bool bTargetsRemoved = Record.Targets.RemoveAll(
				[RemovedTarget](const TWeakObjectPtr<UGameSetting>& T) { return T.Get() == RemovedTarget; }) > 0;

			if (bTargetsRemoved && Record.Condition.IsValid())
			{
				Owner->RemoveEditCondition(Record.Condition.ToSharedRef());
				Records.RemoveAt(RecIndex);
				bOwnerNeedsRefresh = true;
			}
		}

		if (Records.IsEmpty())
		{
			It.RemoveCurrent();
		}

		if (bOwnerNeedsRefresh)
		{
			Owner->RefreshEditableState(/*bNotifyEditConditionsChanged=*/true);
		}
	}
}

#undef LOCTEXT_NAMESPACE
