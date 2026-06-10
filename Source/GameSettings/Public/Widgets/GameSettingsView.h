// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "CommonActivatableWidget.h"
#include "Input/UIActionBindingHandle.h"

#include "GameSettingsView.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSettingsScreenViewModel;
class UInputAction;
enum class ECommonMessagingResult : uint8;

/**
 * Top-level settings widget shell.
 *
 * Project widget BPs derive from this and install the MVVM extension
 * with a viewmodel slot pointing at UGameSettingsScreenViewModel,
 * resolved through UGameSettingsViewModelResolver. Layout, bindings, and
 * child widgets are all authored in BP.
 *
 * The C++ side owns the CommonUI bound actions: it registers an Apply
 * action for the screen's whole active lifetime and a Reset-To-Default
 * action only while there are pending changes (driven by the screen VM's
 * CanResetToDefaults FieldNotify). Set the two UInputActions in the BP and
 * point the inherited InputMapping property at an IMC that maps them - the
 * IMC is auto-added/removed by UCommonActivatableWidget on activate/
 * deactivate, and CommonUI resolves the action's keys against the active
 * mapping contexts at runtime.
 *
 * Requires Project Settings -> Common Input -> Enable Enhanced Input
 * Support = true, otherwise CommonUI ignores the UInputAction path.
 */
UCLASS(MinimalAPI, Abstract, meta = (DisableNativeTick, Category = "Game Settings"))
class UGameSettingsView : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	UE_API UGameSettingsView();

	/** Convenience accessor for BP. Returns the resolved screen VM, or null if not yet bound. */
	UFUNCTION(BlueprintPure, Category = "Game Settings")
	UE_API UGameSettingsScreenViewModel* GetScreenViewModel() const;

protected:
	UE_API virtual void NativeOnInitialized() override;
	UE_API virtual void NativeOnActivated() override;
	UE_API virtual void NativeOnDeactivated() override;
	UE_API virtual void NativeDestruct() override;

	/**
	 * Back/Escape on a settings screen. If there are unsaved changes, route
	 * through a CommonGame confirmation dialog (discard vs keep editing)
	 * instead of closing immediately; on confirm we Cancel() the pending
	 * changes and deactivate. With a clean screen, defer to the base handler
	 * (default deactivate). Always returns true - this widget is the back
	 * handler and fully owns the action.
	 */
	UE_API virtual bool NativeOnHandleBackAction() override;

	/**
	 * A settings screen is unconditionally a menu: Menu input mode, no mouse
	 * capture (cursor must work for clicking rows). Returning a valid config
	 * here is also what makes this widget an active input root in the CommonUI
	 * action router - without it GatherActiveBindings never surfaces the Apply
	 * / Reset bound actions and the action bar stays empty. Owned here rather
	 * than relying on a project base class so the screen always behaves
	 * correctly regardless of what it's parented to.
	 */
	UE_API virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/**
	 * Enhanced Input action that commits pending changes (Screen.Apply).
	 * Bound for the whole time the screen is active. Leave null to skip.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Game Settings|Input")
	TObjectPtr<const UInputAction> ApplyInputAction;

	/**
	 * Enhanced Input action that resets the current tab to defaults
	 * (Screen.ResetToDefaults). Bound only while CanResetToDefaults is true,
	 * so the action-bar prompt appears only when there are pending changes.
	 * Leave null to skip.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Game Settings|Input")
	TObjectPtr<const UInputAction> ResetToDefaultInputAction;

private:
	void HandleApplyAction();
	void HandleResetToDefaultAction();

	/**
	 * Drop the CanResetToDefaults FieldNotify subscription. Idempotent.
	 * Deliberately does NOT touch the action bindings - those are owned for
	 * the widget's lifetime (registered in NativeOnInitialized, cleaned up by
	 * the engine in UCommonUserWidget::NativeDestruct). Removing/re-creating
	 * them per activation is what broke Apply on screen reopen.
	 */
	void RemoveScreenFieldSubscription();

	/** Add or remove the Reset binding so it matches the VM's CanResetToDefaults. */
	void SyncResetBindingToDirtyState();

	/**
	 * Queue a SyncResetBindingToDirtyState for the next tick (coalesced).
	 *
	 * The CanResetToDefaults FieldNotify is broadcast synchronously from the VM,
	 * which can itself be executing inside CommonUI's action dispatch
	 * (HandleResetToDefaultAction -> ResetToDefaults -> broadcast). Mutating the
	 * action bindings there is unsafe: FActionRouterBindingCollection::
	 * ProcessNormalInput range-iterates ActionBindings while invoking the
	 * handler, and RemoveActionBinding erases from that same array mid-iteration
	 * - which corrupts the collection so the Reset binding can never be
	 * re-surfaced. Deferring the add/remove off the dispatch (same FTSTicker
	 * pattern UCommonBoundActionBar::UpdateDisplay uses) makes it safe.
	 */
	void RequestResetBindingSync();

	/** FieldNotify callback for Screen.CanResetToDefaults. */
	void HandleScreenFieldChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId);

	/** Result handler for the unsaved-changes discard dialog. */
	void HandleDiscardConfirmation(ECommonMessagingResult Result);

	FUIActionBindingHandle ApplyBindingHandle;
	FUIActionBindingHandle ResetBindingHandle;

	/** Handle for the CanResetToDefaults FieldNotify subscription, released on deactivate. */
	FDelegateHandle CanResetChangedHandle;

	/** True while a next-tick reset-binding sync is already queued (coalesce). */
	bool bResetBindingSyncQueued = false;

	/** True while the discard-changes dialog is up, so Back doesn't stack dialogs. */
	bool bDiscardDialogActive = false;
};

#undef UE_API
