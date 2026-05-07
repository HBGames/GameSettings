// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingFilterState.h"
#include "GameSettingRegistryChangeTracker.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"

#include "GameSettingsScreenViewModel.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSetting;
class UGameSettingRegistry;
class UGameSettingViewModel;
class UGameSettingsSubsystem;
enum class EGameSettingChangeReason : uint8;

/**
 * The screen-wide orchestrator. Holds tab + visible-setting lists,
 * filter and navigation state, the change tracker, and the per-setting
 * VM cache.
 *
 * Lives on UGameSettingsViewModelSubsystem (per-LocalPlayer) and
 * survives screen open/close. UI artists bind their settings widget BPs
 * to this VM via UGameSettingsViewModelResolver.
 */
UCLASS(MinimalAPI, BlueprintType, DisplayName = "Game Settings Screen")
class UGameSettingsScreenViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
public:
	/** Wire up to the model layer. Called once by the owning subsystem. */
	UE_API void Initialize(UGameSettingsSubsystem* InSettingsSubsystem);

	/** Tear down subscriptions. Called by the owning subsystem on Deinitialize. */
	UE_API void Shutdown();

	// --- FieldNotify state ---

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Tabs")
	const TArray<UGameSettingViewModel*>& GetTabs() const { return Tabs; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Tabs")
	UGameSettingViewModel* GetCurrentTab() const { return CurrentTab; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Settings")
	const TArray<UGameSettingViewModel*>& GetVisibleSettings() const { return VisibleSettings; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "State")
	bool IsDirty() const { return bIsDirty; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "State")
	bool CanApply() const { return bIsDirty; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Navigation")
	bool CanPopNavigation() const { return FilterNavigationStack.Num() > 0; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Selection")
	UGameSettingViewModel* GetFocusedSetting() const { return FocusedSetting; }

	// --- UFUNCTIONs widgets bind to ---

	UFUNCTION(BlueprintCallable, Category = "Tabs")
	UE_API void SetCurrentTab(UGameSettingViewModel* InTab);

	UFUNCTION(BlueprintCallable, Category = "Selection")
	UE_API void SetFocusedSetting(UGameSettingViewModel* InVM);

	UFUNCTION(BlueprintCallable, Category = "Action")
	UE_API void Apply();

	UFUNCTION(BlueprintCallable, Category = "Action")
	UE_API void Cancel();

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	UE_API void NavigateToTabByTag(FGameplayTag TabId);

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	UE_API void NavigateToSettingByTag(FGameplayTag SettingId);

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	UE_API void PopNavigation();

	UE_API virtual void BeginDestroy() override;

private:
	/** Look up or build the VM that wraps a given setting. */
	UE_API UGameSettingViewModel* GetOrCreateViewModelFor(UGameSetting* Setting);

	/** Pick the VM subclass for a setting. Default mapping; project subclasses can override. */
	UE_API TSubclassOf<UGameSettingViewModel> ResolveViewModelClass(UGameSetting* Setting) const;

	UE_API void RebuildTabs();
	UE_API void RebuildVisibleSettings();
	UE_API void RefreshDirtyState();

	UE_API void HandleStructureChanged(UGameSettingRegistry* Registry);
	UE_API void HandleSettingChanged(UGameSetting* Setting, EGameSettingChangeReason Reason);

	UPROPERTY(Transient)
	TObjectPtr<UGameSettingsSubsystem> SettingsSubsystem;

	/** Owns its own change tracker; the registry is shared with the model layer. */
	FGameSettingRegistryChangeTracker ChangeTracker;

	/** The active filter (root, search, etc.) and the back-stack of previous filters. */
	FGameSettingFilterState FilterState;
	TArray<FGameSettingFilterState> FilterNavigationStack;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGameSettingViewModel>> Tabs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGameSettingViewModel>> VisibleSettings;

	UPROPERTY(Transient)
	TObjectPtr<UGameSettingViewModel> CurrentTab;

	UPROPERTY(Transient)
	TObjectPtr<UGameSettingViewModel> FocusedSetting;

	/**
	 * Strong-ref cache of every per-setting VM we've built. Acts as both
	 * GC root and lookup table; linear scan by setting pointer is fast
	 * enough for typical screen sizes. Cleaned up in HandleStructureChanged
	 * when underlying settings get removed.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UGameSettingViewModel>> AllViewModels;

	bool bIsDirty = false;
};

#undef UE_API
