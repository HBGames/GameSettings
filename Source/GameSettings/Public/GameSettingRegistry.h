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
struct FGameSettingFilterState;

enum class EGameSettingChangeReason : uint8;

/**
 * Holds the tree of UGameSetting instances and forwards per-setting
 * events out to whoever's listening (the settings widget, mostly).
 *
 * Not abstract. A project usually constructs a plain UGameSettingRegistry
 * and pushes settings into it through AddSetting / AddTab from a
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
	 * Register a top-level tab (a UGameSettingCollection used as a page).
	 * Returns a handle that the caller retains for symmetric removal on
	 * teardown (plugin unload, GameFeature deactivation, etc.).
	 */
	UE_API FGameSettingHandle AddTab(UGameSettingCollection* InTab);

	/**
	 * Register a setting under an existing tab (looked up by primary asset
	 * id) or at the top level if ParentTab is invalid or unknown. Returns a
	 * handle that the caller retains for symmetric removal.
	 */
	UE_API FGameSettingHandle AddSetting(UGameSetting* InSetting, FPrimaryAssetId ParentTab = FPrimaryAssetId());

	/** Remove a setting (and any descendants) by handle. Returns true on success. */
	UE_API bool RemoveByHandle(const FGameSettingHandle& Handle);

	/** Remove a setting (and any descendants) by primary asset id. */
	UE_API bool RemoveById(const FPrimaryAssetId& Id);

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
	 * to skip subclassing and contribute via AddSetting / AddTab from a
	 * UGameFeatureAction or UGameSettingsAutoContributor.
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
	/** Wires events on a setting and any descendants. Idempotent on re-add. */
	UE_API void WireSettingTree(UGameSetting* InSetting);

	/** Removes a setting and its descendants from all bookkeeping. Returns the count removed. */
	UE_API int32 UnregisterSettingTree(UGameSetting* InSetting);

	/** Resolves a tab primary-asset-id to a UGameSettingCollection in TopLevelSettings, or nullptr. */
	UE_API UGameSettingCollection* FindTabById(const FPrimaryAssetId& TabId) const;

	/**
	 * Runtime cache for handle-to-setting lookup. Not a UPROPERTY because
	 * FGameSettingHandle isn't a USTRUCT (it's a plain runtime-only counter)
	 * and the strong references are already held by RegisteredSettings.
	 * Uses TWeakObjectPtr so stale entries are detectable after a setting
	 * is destroyed out from under us.
	 */
	TMap<FGameSettingHandle, TWeakObjectPtr<UGameSetting>> SettingsByHandle;
};

#undef UE_API
