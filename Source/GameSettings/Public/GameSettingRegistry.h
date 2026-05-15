// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSetting.h"
#include "GameplayTagContainer.h"
#include "Templates/Casts.h"

#include "GameSettingRegistry.generated.h"

#define UE_API GAMESETTINGS_API

//--------------------------------------
// UGameSettingRegistry
//--------------------------------------

class ULocalPlayer;
class UGameSettingCollection;
class UGameSettingEditConditionSpec;
class FGameSettingEditCondition;
struct FGameSettingFilterState;

enum class EGameSettingChangeReason : uint8;

/**
 * One entry in the registry's deferred-placement queue. Held by the registry
 * when a setting is added under a parent that hasn't been registered yet; the
 * entry is consumed when the matching parent later arrives via AddCollection.
 *
 * The setting is kept alive by RegisteredSettings on the registry, so this
 * struct only needs to remember the desired parent id alongside the setting
 * pointer for the re-parenting pass.
 */
USTRUCT()
struct FGameSettingDeferredPlacement
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UGameSetting> Setting = nullptr;

	UPROPERTY(Transient)
	FPrimaryAssetId ParentContainerId;
};

/**
 * One entry in the registry's deferred edit-condition queue. Held by the
 * registry when a contribution's spec references a target setting that
 * isn't registered yet; the entry is reflushed every time a new setting is
 * added and is installed once every target resolves.
 *
 * Owner and Spec are kept alive elsewhere (Owner by RegisteredSettings, Spec
 * by the contribution UPRIMARY data asset), so the struct holds them as
 * regular UPROPERTYs without rooting concerns.
 */
USTRUCT()
struct FGameSettingDeferredEditCondition
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UGameSetting> Owner = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UGameSettingEditConditionSpec> Spec = nullptr;

	UPROPERTY(Transient)
	TArray<FPrimaryAssetId> MissingTargets;
};

/**
 * Holds the tree of UGameSetting instances and forwards per-setting
 * events out to whoever's listening (the settings widget, mostly).
 *
 * Not abstract. A project usually constructs a plain UGameSettingRegistry
 * and pushes settings into it through AddSetting / AddCollection from a
 * UGameFeatureAction, a UGameSettingsAutoContributor, or direct C++.
 * Subclassing still works for projects that prefer to seed everything
 * in OnInitialize, but it's no longer required.
 */
UCLASS(MinimalAPI, BlueprintType)
class UGameSettingRegistry : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_EVENT_TwoParams(UGameSettingRegistry, FOnSettingChanged, UGameSetting*, EGameSettingChangeReason);
	DECLARE_EVENT_OneParam(UGameSettingRegistry, FOnSettingEditConditionChanged, UGameSetting*);

	FOnSettingChanged OnSettingChangedEvent;
	FOnSettingEditConditionChanged OnSettingEditConditionChangedEvent;

	DECLARE_EVENT_TwoParams(UGameSettingRegistry, FOnSettingNamedActionEvent, UGameSetting* /*Setting*/, FGameplayTag /*GameSettings_Action_Tag*/);
	FOnSettingNamedActionEvent OnSettingNamedActionEvent;

	/** Navigate to the child settings of the provided setting. */
	DECLARE_EVENT_OneParam(UGameSettingRegistry, FOnExecuteNavigation, UGameSetting* /*Setting*/);
	FOnExecuteNavigation OnExecuteNavigationEvent;

	/**
	 * Broadcast when settings are added, removed, or the registry is
	 * regenerated. Settings widgets subscribe to know when to rebuild
	 * their visible list.
	 */
	DECLARE_EVENT_OneParam(UGameSettingRegistry, FOnStructureChanged, UGameSettingRegistry* /*Registry*/);
	FOnStructureChanged OnStructureChangedEvent;

public:
	UE_API UGameSettingRegistry();

	UE_API void Initialize(ULocalPlayer* InLocalPlayer);

	UE_API virtual void Regenerate();

	UE_API virtual bool IsFinishedInitializing() const;

	UE_API virtual void SaveChanges();

	UE_API void GetSettingsForFilter(const FGameSettingFilterState& FilterState, TArray<UGameSetting*>& InOutSettings);

	/** The LocalPlayer this registry was initialized for. */
	ULocalPlayer* GetOwningLocalPlayer() const { return OwningLocalPlayer; }

	// --- Contribution API --------------------------------------------------

	/**
	 * Register a UGameSettingCollection. With an invalid ParentContainerId it
	 * lands at the top level (a tab page). With a valid id it nests under
	 * the named parent collection (a section under a tab, or a section under
	 * another section). If the parent hasn't been registered yet, the
	 * collection is held in a deferred-placement queue and re-parented when
	 * its parent later arrives - any contribution arrival order is safe.
	 *
	 * Returns a handle that the caller retains for symmetric removal on
	 * teardown (plugin unload, GameFeature deactivation, etc.).
	 */
	UE_API FGameSettingHandle AddCollection(UGameSettingCollection* InCollection, FPrimaryAssetId ParentContainerId = FPrimaryAssetId());

	/**
	 * Register a setting under an existing parent collection (a tab or a
	 * section) looked up by primary asset id, or at the top level if
	 * ParentContainerId is invalid. If the parent hasn't been registered
	 * yet, the setting is held in the deferred-placement queue and
	 * re-parented when the parent later arrives.
	 *
	 * Returns a handle that the caller retains for symmetric removal.
	 */
	UE_API FGameSettingHandle AddSetting(UGameSetting* InSetting, FPrimaryAssetId ParentContainerId = FPrimaryAssetId());

	/** Remove a setting (and any descendants) by handle. Returns true on success. */
	UE_API bool RemoveByHandle(const FGameSettingHandle& Handle);

	/** Remove a setting (and any descendants) by primary asset id. */
	UE_API bool RemoveById(const FPrimaryAssetId& Id);

	/**
	 * Wire a contribution's authored edit-condition specs onto a setting that
	 * just landed in the registry. Eagerly installs any spec whose targets
	 * are already registered; queues anything still waiting on an unresolved
	 * target. Idempotent: re-applying the same Owner+Spec pair is a no-op.
	 *
	 * Called by every UGameSettingsContribution_* subclass's Apply() right
	 * after AddSetting / AddCollection succeeds.
	 */
	UE_API void ApplyEditConditionSpecs(UGameSetting* Owner,
		const TArray<TObjectPtr<UGameSettingEditConditionSpec>>& Specs);

	/** How many edit-condition specs are currently waiting on missing targets. */
	int32 GetNumDeferredEditConditions() const { return DeferredEditConditions.Num(); }

	// --- Lookup ------------------------------------------------------------

	/** Look up a setting by its primary asset id. Returns nullptr if not registered. */
	UE_API UGameSetting* FindSettingById(const FPrimaryAssetId& Id) const;

	/** Look up a setting by its handle. Returns nullptr if no longer present. */
	UE_API UGameSetting* FindSettingByHandle(const FGameSettingHandle& Handle) const;

	/** Primary-asset-id lookup that asserts the result and casts to T. */
	template<typename T = UGameSetting>
	T* FindSettingByIdChecked(const FPrimaryAssetId& Id) const
	{
		T* Setting = Cast<T>(FindSettingById(Id));
		check(Setting);
		return Setting;
	}

protected:
	/**
	 * Subclass hook for projects that want to seed the registry with their
	 * defaults at construction time. Not pure: the recommended pattern is
	 * to skip subclassing and contribute via AddSetting / AddCollection from
	 * a UGameFeatureAction or UGameSettingsAutoContributor.
	 */
	UE_API virtual void OnInitialize(ULocalPlayer* InLocalPlayer);

	virtual void OnSettingApplied(UGameSetting* Setting) { }

	// Internal event handlers.
	UE_API void HandleSettingChanged(UGameSetting* Setting, EGameSettingChangeReason Reason);
	UE_API void HandleSettingApplied(UGameSetting* Setting);
	UE_API void HandleSettingEditConditionsChanged(UGameSetting* Setting);
	UE_API void HandleSettingNamedAction(UGameSetting* Setting, FGameplayTag GameSettings_Action_Tag);
	UE_API void HandleSettingNavigation(UGameSetting* Setting);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGameSetting>> TopLevelSettings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGameSetting>> RegisteredSettings;

	UPROPERTY(Transient)
	TObjectPtr<ULocalPlayer> OwningLocalPlayer;

private:
	/**
	 * One record of an installed edit-condition spec. Held in
	 * AppliedEditConditions, keyed by Owner. Targets are weak so unloads
	 * self-evict; Condition keeps the SharedRef alive for cleanup.
	 */
	struct FAppliedEditConditionRecord
	{
		TWeakObjectPtr<UGameSettingEditConditionSpec> Spec;
		TSharedPtr<FGameSettingEditCondition>         Condition;
		TArray<TWeakObjectPtr<UGameSetting>>          Targets;
	};

	/** Build + install one spec on the owner, recording the wire-up. */
	UE_API void InstallSpec(UGameSetting* Owner, UGameSettingEditConditionSpec* Spec);

	/**
	 * Walk DeferredEditConditions and install anything whose targets have
	 * arrived since the last flush. Re-runs until quiescent so multi-hop
	 * chains converge in one call. Compacts stale weak entries on entry.
	 */
	UE_API void FlushDeferredEditConditions();

	/**
	 * Reverse-lookup cleanup. Called from UnregisterSettingTree on the
	 * target side: every applied-edit-condition record that lists this
	 * setting as a target gets dropped from its owner (RemoveEditCondition
	 * + RefreshEditableState).
	 */
	UE_API void CleanupEditConditionsForRemovedTarget(UGameSetting* RemovedTarget);

	/** Wires events on a setting and any descendants. Idempotent on re-add. */
	UE_API void WireSettingTree(UGameSetting* InSetting);

	/** Removes a setting and its descendants from all bookkeeping. Returns the count removed. */
	UE_API int32 UnregisterSettingTree(UGameSetting* InSetting);

	/**
	 * Resolves a primary-asset-id to a UGameSettingCollection anywhere in the
	 * registered tree (top-level tab or nested section). Returns null if no
	 * matching collection is registered yet.
	 */
	UE_API UGameSettingCollection* FindCollectionById(const FPrimaryAssetId& Id) const;

	/**
	 * Walks the deferred-placement list and re-parents any whose parent now
	 * exists in the registered tree. Loops until no further progress is
	 * possible so arbitrarily-deep deferred chains resolve in one call.
	 */
	UE_API void FlushDeferredPlacements();

	/** Drop the deferred entry for a specific setting, if present. Used by RemoveByHandle. */
	UE_API void RemoveFromDeferred(UGameSetting* InSetting);

	/**
	 * Runtime cache for handle-to-setting lookup. Not a UPROPERTY because
	 * FGameSettingHandle isn't a USTRUCT (it's a plain runtime-only counter)
	 * and the strong references are already held by RegisteredSettings.
	 * Uses TWeakObjectPtr so stale entries are detectable after a setting
	 * is destroyed out from under us.
	 */
	TMap<FGameSettingHandle, TWeakObjectPtr<UGameSetting>> SettingsByHandle;

	/**
	 * Settings whose ParentContainerId pointed at a collection that wasn't
	 * registered yet at AddSetting / AddCollection time. Each entry is
	 * re-checked after every successful collection add (see
	 * FlushDeferredPlacements); the setting stays in RegisteredSettings the
	 * whole time so handles returned from Add* remain stable.
	 */
	UPROPERTY(Transient)
	TArray<FGameSettingDeferredPlacement> DeferredPlacements;

	/**
	 * Edit-condition specs queued because at least one referenced target
	 * setting wasn't registered yet. Flushed on every AddSetting/AddCollection
	 * (after FlushDeferredPlacements) and once more at the end of Regenerate.
	 */
	UPROPERTY(Transient)
	TArray<FGameSettingDeferredEditCondition> DeferredEditConditions;

	/**
	 * Installed edit-condition records keyed by owner. Used for idempotent
	 * re-apply detection and for target-removal cleanup. Owners are weak so
	 * stale entries self-evict; FlushDeferredEditConditions compacts the map
	 * on each invocation.
	 */
	TMap<TWeakObjectPtr<UGameSetting>, TArray<FAppliedEditConditionRecord>> AppliedEditConditions;
};

#undef UE_API
