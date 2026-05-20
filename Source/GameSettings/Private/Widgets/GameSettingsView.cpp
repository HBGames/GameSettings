// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Widgets/GameSettingsView.h"

#include "CommonUITypes.h"
#include "Containers/Ticker.h"
#include "Engine/LocalPlayer.h"
#include "GameSettingsLog.h"
#include "GameSettingsViewModelSubsystem.h"
#include "Input/CommonUIInputTypes.h"
#include "Messaging/CommonGameDialog.h"
#include "Messaging/CommonMessagingSubsystem.h"
#include "ViewModels/GameSettingsScreenViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsView)

#define LOCTEXT_NAMESPACE "GameSettingsView"

UGameSettingsView::UGameSettingsView()
{
	// A settings screen always owns Back: unsaved changes need a discard
	// confirm rather than a silent close. Set here (not left to the BP) so
	// the behavior holds regardless of how the screen is parented - same
	// rationale as GetDesiredInputConfig being owned in C++.
	bIsBackHandler = true;
}

TOptional<FUIInputConfig> UGameSettingsView::GetDesiredInputConfig() const
{
	// Menu mode, no mouse capture so the cursor stays usable for clicking
	// settings rows. The valid (set) return is also what registers this
	// widget as an active input root - required for the bound action bar.
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

UGameSettingsScreenViewModel* UGameSettingsView::GetScreenViewModel() const
{
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UGameSettingsViewModelSubsystem* Sub = LP->GetSubsystem<UGameSettingsViewModelSubsystem>())
		{
			return Sub->GetScreenViewModel();
		}
	}
	return nullptr;
}

void UGameSettingsView::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Register the bound actions ONCE, here, for the widget's whole lifetime -
	// the same lifecycle slot Lyra's ULyraSettingScreen uses. Registering in
	// NativeOnActivated instead is broken on reopen: UCommonUserWidget hands
	// its ActionBindings to the action router from OnWidgetRebuilt
	// (NotifyUserWidgetConstructed, gated on ActionBindings.Num() > 0). A
	// binding created later, and re-created on every reactivation, is bound
	// against the wrong/stale router node when the screen is pushed a second
	// time and never surfaces (the exact "Apply gone on reopen" symptom).
	// Visibility is toggled afterwards via AddActionBinding /
	// RemoveActionBinding on these persistent handles - never by
	// re-registering and never by invalidating the handles.

	// HARD REQUIREMENT: a UInputAction-only FBindUIActionArgs registered while
	// Enhanced Input support is disabled falls through FUIActionBinding's
	// constructor to GetLegacyInputActionData() + check(LegacyActionData),
	// which asserts (we have no legacy data). If the project setting is off,
	// skip the whole input setup and surface why instead of crashing.
	if (!CommonUI::IsEnhancedInputSupportEnabled())
	{
		UE_LOG(LogGameSettings, Warning,
			TEXT("%s: Enhanced Input support is disabled (Project Settings -> Common Input -> Enable Enhanced Input Support). "
				 "Apply / Reset bound actions are not registered. Enable that setting to get the settings action bar."),
			*GetName());
		return;
	}

	// Apply is present the whole time the screen is the active input root. Its
	// handler no-ops when nothing is dirty (Screen.Apply early-returns), so
	// leaving it bound for life is safe; the router only surfaces it while
	// this widget is active, so no activate/deactivate juggling is needed.
	if (ApplyInputAction)
	{
		FBindUIActionArgs ApplyArgs(
			ApplyInputAction,
			/*bShouldDisplayInActionBar=*/true,
			FSimpleDelegate::CreateUObject(this, &UGameSettingsView::HandleApplyAction));
		ApplyBindingHandle = RegisterUIActionBinding(ApplyArgs);
	}

	// Reset is registered now too so its handle is stable for the widget's
	// life, then immediately pulled from the active set - it only shows while
	// a visible setting differs from default (SyncResetBindingToDirtyState).
	if (ResetToDefaultInputAction)
	{
		FBindUIActionArgs ResetArgs(
			ResetToDefaultInputAction,
			/*bShouldDisplayInActionBar=*/true,
			FSimpleDelegate::CreateUObject(this, &UGameSettingsView::HandleResetToDefaultAction));
		ResetBindingHandle = RegisterUIActionBinding(ResetArgs);
		RemoveActionBinding(ResetBindingHandle);
	}

	UE_LOG(LogGameSettings, Verbose,
		TEXT("%s::NativeOnInitialized: bound actions registered. ApplyValid=%d ResetValid=%d"),
		*GetName(),
		ApplyBindingHandle.IsValid() ? 1 : 0,
		ResetBindingHandle.IsValid() ? 1 : 0);
}

void UGameSettingsView::NativeOnActivated()
{
	Super::NativeOnActivated();

	// Subscribe to the screen VM's CanResetToDefaults FieldNotify and bring the
	// Reset binding to the right state for the current values. The bindings
	// themselves are owned for the widget's lifetime (NativeOnInitialized);
	// here we only manage the per-activation FieldNotify subscription.
	if (UGameSettingsScreenViewModel* VM = GetScreenViewModel())
	{
		if (!CanResetChangedHandle.IsValid())
		{
			CanResetChangedHandle = VM->AddFieldValueChangedDelegate(
				UGameSettingsScreenViewModel::FFieldNotificationClassDescriptor::CanResetToDefaults,
				INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(
					this, &UGameSettingsView::HandleScreenFieldChanged));
		}
	}

	RequestResetBindingSync();
}

void UGameSettingsView::NativeOnDeactivated()
{
	RemoveScreenFieldSubscription();
	Super::NativeOnDeactivated();
}

void UGameSettingsView::NativeDestruct()
{
	// FieldNotify only. The action bindings are torn down by the engine in
	// UCommonUserWidget::NativeDestruct (NotifyUserWidgetDestructed). We must
	// NOT RemoveActionBinding / invalidate them on deactivate or destruct -
	// re-creating them per activation is exactly what broke reopen.
	RemoveScreenFieldSubscription();
	Super::NativeDestruct();
}

void UGameSettingsView::RemoveScreenFieldSubscription()
{
	if (CanResetChangedHandle.IsValid())
	{
		if (UGameSettingsScreenViewModel* VM = GetScreenViewModel())
		{
			VM->RemoveFieldValueChangedDelegate(
				UGameSettingsScreenViewModel::FFieldNotificationClassDescriptor::CanResetToDefaults,
				CanResetChangedHandle);
		}
		CanResetChangedHandle.Reset();
	}
}

void UGameSettingsView::HandleApplyAction()
{
	if (UGameSettingsScreenViewModel* VM = GetScreenViewModel())
	{
		VM->Apply();
	}
}

void UGameSettingsView::HandleResetToDefaultAction()
{
	if (UGameSettingsScreenViewModel* VM = GetScreenViewModel())
	{
		VM->ResetToDefaults();
	}
}

void UGameSettingsView::SyncResetBindingToDirtyState()
{
	// Invalid handle = Reset was never registered (no IA, or Enhanced Input
	// support off). Nothing to toggle.
	if (!ResetBindingHandle.IsValid())
	{
		return;
	}

	UGameSettingsScreenViewModel* VM = GetScreenViewModel();
	const bool bShouldBeBound = VM && VM->CanResetToDefaults();
	const bool bIsBound = GetActionBindings().Contains(ResetBindingHandle);

	// Toggle the persistent handle in/out of the active set (Lyra's
	// OnSettingsDirtyStateChanged pattern). Never re-register or invalidate -
	// the handle must stay alive for the widget's lifetime.
	if (bShouldBeBound && !bIsBound)
	{
		AddActionBinding(ResetBindingHandle);
	}
	else if (!bShouldBeBound && bIsBound)
	{
		RemoveActionBinding(ResetBindingHandle);
	}
}

void UGameSettingsView::HandleScreenFieldChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId)
{
	// Only CanResetToDefaults is subscribed, so no FieldId branching needed.
	// Deferred, not direct: this fires synchronously from the VM and may be
	// running inside CommonUI action dispatch - see RequestResetBindingSync.
	RequestResetBindingSync();
}

void UGameSettingsView::RequestResetBindingSync()
{
	if (bResetBindingSyncQueued)
	{
		return;
	}
	bResetBindingSyncQueued = true;

	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		bResetBindingSyncQueued = false;
		// Skip if we were deactivated before the tick landed - bindings are
		// torn down on deactivate and re-established on the next activate.
		if (IsActivated())
		{
			SyncResetBindingToDirtyState();
		}
		return false;
	}));
}

bool UGameSettingsView::NativeOnHandleBackAction()
{
	UGameSettingsScreenViewModel* VM = GetScreenViewModel();

	// Clean screen (or no VM): nothing to lose, let the base handler run its
	// default close. bIsBackHandler is true so the base still returns handled.
	if (!VM || !VM->IsDirty())
	{
		return Super::NativeOnHandleBackAction();
	}

	// Already prompting - swallow repeat Back presses so dialogs don't stack.
	if (bDiscardDialogActive)
	{
		return true;
	}

	UCommonMessagingSubsystem* Messaging = nullptr;
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		Messaging = LP->GetSubsystem<UCommonMessagingSubsystem>();
	}

	// Messaging subsystem unavailable (e.g. no UI policy / dedicated path):
	// fail safe by closing rather than trapping the user on the screen.
	if (!Messaging)
	{
		UE_LOG(LogGameSettings, Warning,
			TEXT("%s: no UCommonMessagingSubsystem; closing settings without a discard prompt despite pending changes."),
			*GetName());
		VM->Cancel();
		return Super::NativeOnHandleBackAction();
	}

	UCommonGameDialogDescriptor* Descriptor = UCommonGameDialogDescriptor::CreateConfirmationYesNo(
		LOCTEXT("DiscardChangesHeader", "Unsaved Changes"),
		LOCTEXT("DiscardChangesBody", "You have unsaved changes. Discard them and leave?"));

	bDiscardDialogActive = true;
	Messaging->ShowConfirmation(
		Descriptor,
		FCommonMessagingResultDelegate::CreateUObject(this, &UGameSettingsView::HandleDiscardConfirmation));

	// We own the back action: the dialog result decides whether we close.
	return true;
}

void UGameSettingsView::HandleDiscardConfirmation(ECommonMessagingResult Result)
{
	bDiscardDialogActive = false;

	// Only Confirmed (Yes) discards and leaves. Declined / Cancelled / Killed
	// all keep the user on the screen with their edits intact.
	if (Result != ECommonMessagingResult::Confirmed)
	{
		return;
	}

	if (UGameSettingsScreenViewModel* VM = GetScreenViewModel())
	{
		VM->Cancel();
	}
	DeactivateWidget();
}

#undef LOCTEXT_NAMESPACE
