# GameSettings MVVM guide

For UI artists wiring up settings widgets. Written assuming you've used the
UMG Viewmodel panel before; if not, the engine docs at
`Engine -> Edit -> Editor Preferences -> Plugins -> ModelViewViewModel` are
the place to start.

The plugin is built around three view-model types you bind against:

- `UGameSettingsScreenViewModel` drives the whole screen: tabs, the
  visible-settings list, dirty state, reset state, focus, navigation. It
  lives on `UGameSettingsViewModelSubsystem` (one per LocalPlayer) and
  survives screen open/close.
- `UGameSettingViewModel` and its subclasses (Toggle / Scalar / Discrete /
  Action / Collection) each wrap a single setting. The list view feeds one
  per row.
- The detail view binds to whichever VM is currently focused on the screen
  VM.

You don't need to write C++ to use the plugin. Configure your widgets in BP,
point the MVVM resolver at the plugin, and the rest is bindings.

---

## Setting up the screen widget

1. Create a Widget Blueprint deriving from `UGameSettingsView` (or any
   `UCommonActivatableWidget` subclass).
2. Open the View Bindings panel (Window -> View Bindings).
3. Add a viewmodel: Class = `UGameSettingsScreenViewModel`, ViewModelName =
   `Screen`.
4. Set the creation type to `Resolver`. Pick `UGameSettingsViewModelResolver`
   from the picker.

The resolver walks `Widget -> OwningLocalPlayer -> UGameSettingsViewModelSubsystem
-> screen VM`. No per-widget code, no manual `SetViewModel` call.

From there you bind whatever you need:

- An Apply button's IsEnabled to `Screen.CanApply`.
- An Apply button's OnClicked to `Screen.Apply`.
- A Cancel button's OnClicked to `Screen.Cancel`.
- A Reset-to-defaults button's IsEnabled to `Screen.CanResetToDefaults` and
  OnClicked to `Screen.ResetToDefaults`. (Reset state is decoupled from
  dirty state: the button shows whenever a visible setting differs from its
  default, even on a fresh screen, and a reset still has to be confirmed with
  Apply.)
- A back button's IsEnabled to `Screen.CanPopNavigation` and OnClicked to
  `Screen.PopNavigation`.
- A tab strip iterating `Screen.Tabs` (one tab button per collection VM).
  On click, call `Screen.SetCurrentTab` with that tab's VM; bind the strip's
  current selection to `Screen.CurrentTab`.
- A dirty indicator's Visibility to a conversion of `Screen.IsDirty` (use
  `Conv_BoolToSlateVisibility` from `UMVVMConversionLibrary`).

---

## The settings list

Inside your screen widget, drop a `UGameSettingsListView` (BP class deriving
from it). On its details panel, set `Bindings` to your project's
`UGameSettingsViewBindings` asset (see the next section).

In the View Bindings panel:

- Bind the list view's `ListItems` source to `Screen.VisibleSettings`.
- Bind the list view's `OnItemSelectionChanged` to a project-side function
  that calls `Screen.SetFocusedSetting` with the selected VM.

The list view picks the entry widget class per row from your bindings asset,
then auto-wires each entry's MVVM context. You don't write any glue.

Each entry widget BP is its own widget, derived from `UGameSettingEntryBase`,
with View Bindings configured against the appropriate VM type. The list view
sets each entry's VM into a slot literally named `Setting`:

- Toggle entry: viewmodel `UGameSettingToggleViewModel`. Bind a checkbox or
  styled toggle button's IsChecked to `Setting.IsChecked`, two-way. (The VM
  anchors IsChecked on a single bindable field so both the read and write
  sides resolve; no conversion needed.)
- Scalar entry: viewmodel `UGameSettingScalarViewModel`. Bind a slider's
  Value to `Setting.NormalizedValue` (0..1), two-way. Bind a value-text
  TextBlock to `Setting.FormattedText`. Use `Setting.SourceMin` /
  `SourceMax` / `Step` if your widget needs the raw range.
- Discrete entry: viewmodel `UGameSettingDiscreteViewModel`. Bind a combo
  box or Common Rotator's Options to `Setting.Options` and selection to
  `Setting.SelectedIndex`, two-way. For shoulder-button cycling, call
  `Setting.SelectNextOption` / `Setting.SelectPreviousOption`.
- Action entry: viewmodel `UGameSettingActionViewModel`. Bind a button's
  Text to `Setting.ActionText`, OnClicked to `Setting.Execute`.

> Toggle and Discrete are separate VMs now. A bool toggle gets
> `UGameSettingToggleViewModel` (IsChecked); an option list gets
> `UGameSettingDiscreteViewModel` (SelectedIndex + Options). They used to
> share a value type in Lyra; they don't here.

Every entry should also bind:

- A DisplayName label to `Setting.DisplayName`.
- IsEnabled (visual) to `Setting.IsEnabled`.
- Visibility to a conversion of `Setting.IsVisible`.

### Navigating into a sub-page

Drilling into a section is project glue, not a pure binding: the screen VM's
`NavigateToTabById` / `NavigateToSettingById` take an `FPrimaryAssetId`, and
the VM doesn't surface that id as a bindable field. In your
`OnItemSelectionChanged` handler, read `VM->GetSetting()->GetSettingId()` and
pass it to `NavigateToSettingById`, then drive the back button off
`Screen.PopNavigation` / `Screen.CanPopNavigation`.

---

## The detail view

`UGameSettingsDetailView` (or any `UCommonUserWidget` derivation) holds the
focused setting's detail panel. In View Bindings:

- Add a viewmodel: Class = `UGameSettingViewModel`, ViewModelName =
  `Focused`. Creation type = `Property Path`. Path = `Screen.FocusedSetting`
  (with `Screen` declared as a parent-resolved context, or duplicate the
  resolver if your detail view is detached).
- Bind a heading to `Focused.DisplayName`.
- Bind a description rich-text to `Focused.DescriptionRichText`.
- Bind a warning rich-text to `Focused.WarningRichText`, with Visibility tied
  to whether it's non-empty.
- Bind a dynamic-details line to `Focused.DynamicDetails` if you use it.
- Bind a disabled-reasons list to `Focused.DisabledReasons`.

Detail extension widgets (per-VM-type sub-panels) come from the same bindings
asset. The detail view BP iterates `Bindings.DetailExtensions` for the
focused VM's class and stamps them in a panel widget.

---

## The bindings asset

`UGameSettingsViewBindings` is a project-level Data Asset that maps:

- VM class to entry widget class (which widget renders this setting).
- VM class to detail extension widget classes (per-setting sub-panels).

Create one in your project (Content Browser: right-click -> Miscellaneous ->
Data Asset -> `Game Settings View Bindings`). Fill in:

```
EntryWidgetForViewModel:
  UGameSettingToggleViewModel   -> WBP_SettingsEntry_Toggle
  UGameSettingScalarViewModel   -> WBP_SettingsEntry_Slider
  UGameSettingDiscreteViewModel -> WBP_SettingsEntry_Dropdown
  UGameSettingActionViewModel   -> WBP_SettingsEntry_Button
  UGameSettingViewModel         -> WBP_SettingsEntry_Generic    (catch-all)

DetailExtensions:
  UGameSettingScalarViewModel -> [WBP_SettingsExtension_DefaultMarker]
```

Lookup walks the VM class chain on a miss, so a binding for the base class
(`UGameSettingViewModel`) acts as a catch-all when no more specific match is
found.

Set the Bindings property on your `UGameSettingsListView` to point at this
asset. That's it.

---

## Per-GFP overrides

A GameFeaturePlugin can ship its own bindings asset and push it onto a
runtime override stack. Add a `Game Feature Action: Add Game Settings View
Bindings` to your `UGameFeatureData`, point it at your bindings asset, set a
Priority. The list view consults overrides first (highest priority wins)
before falling back to the project's local bindings.

Use this when a VR plugin wants its scalar slider to render with VR gestures,
or a colorblind-mode plugin wants its own toggle widget.

---

## Conversion functions

UE ships some useful ones in `UMVVMConversionLibrary`:

- `Conv_BoolToSlateVisibility(bool)` for `IsVisible -> Visibility`.

For richer cases, write your own static `UFUNCTION(BlueprintPure)` anywhere
in your project. The MVVM compiler picks them up automatically.

For a function that mutates a destination value (e.g. take a brush and change
one parameter), tag it with `meta=(MVVMBindToDestination="ArgName")` where
ArgName is the destination-bound argument.

---

## Common gotchas

- A widget needs the MVVM extension installed before any bindings work. The
  View Bindings panel auto-installs it on first use.
- The `Setting` viewmodel slot on entry widgets must match the name the list
  view uses (literally the string `"Setting"`). The list view sets it in
  C++; if your BP picks a different name, the binding silently doesn't fire.
- Two-way fields are anchored on a UPROPERTY with Getter/Setter + FieldNotify
  (IsChecked, NormalizedValue, SelectedIndex). Bind to those, not to the
  read-only display fields like FormattedText.
- For sliders, use the binding's Delayed execution mode. `USlider`
  double-broadcasts on SetValue, and a non-delayed two-way binding trips the
  MVVM recursion detector.
- VM display fields broadcast through their getter UFUNCTION. If you write a
  setter in C++, broadcast via `UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED` using
  the field name the UPROPERTY anchor exposes.
- The screen VM lives for the LocalPlayer's lifetime. Multiple settings
  screens share dirty state, focus, and the visible-settings list. To start a
  screen fresh, call `Screen.Cancel` on activation.

---

## Writing a custom VM subclass

For a setting type the plugin doesn't ship a VM for, subclass
`UGameSettingViewModel` (or one of its subclasses) in C++:

```cpp
UCLASS(MinimalAPI, BlueprintType)
class UMyQualityPresetViewModel : public UGameSettingViewModel
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, FieldNotify, Category = "Quality")
    EQualityLevel GetCurrentLevel() const { return CurrentLevel; }

    UFUNCTION(BlueprintCallable, Category = "Quality")
    void SetCurrentLevel(EQualityLevel NewLevel);

protected:
    virtual void RefreshFromSetting() override;

private:
    EQualityLevel CurrentLevel = EQualityLevel::Medium;
};
```

In the implementation, override `RefreshFromSetting` to read the setting's
state, compare-and-set fields, and broadcast each FieldNotify that changed.

The screen VM picks the VM class per setting in
`ResolveViewModelClass`, keyed on the setting's value type (bool -> Toggle,
scalar -> Scalar, discrete -> Discrete, and so on). To make it instantiate
your subclass, override `ResolveViewModelClass` in a project subclass of
`UGameSettingsScreenViewModel`. Then map your VM type to its widget class in
the bindings asset.

---

## What lives where

```
Source/GameSettings/Public/
  ViewModels/
    GameSettingViewModel.h            Base for per-setting VMs
    GameSettingToggleViewModel.h      Bool toggle (IsChecked)
    GameSettingDiscreteViewModel.h    Option list (SelectedIndex + Options)
    GameSettingScalarViewModel.h      Slider (NormalizedValue + FormattedText)
    GameSettingActionViewModel.h      Button (ActionText + Execute)
    GameSettingCollectionViewModel.h  Tab / nested collection
    GameSettingsScreenViewModel.h     Screen orchestrator
  Widgets/
    GameSettingsView.h                UCommonActivatableWidget shell
    GameSettingsListView.h            UListView with entry-class lookup
    GameSettingsDetailView.h          Detail panel base
    GameSettingEntryBase.h            Entry widget base
    GameSettingsViewBindings.h        Mapping data asset
  GameSettingsViewModelSubsystem.h    Per-LocalPlayer screen-VM owner
  GameSettingsViewModelResolver.h     MVVM resolver

Source/GameSettingsGameFeatures/Public/
  GameFeatureAction_RegisterGameSettings.h
  GameFeatureAction_AddViewBindings.h
```

If you want a reference for the per-LocalPlayer-subsystem + MVVM resolver
pattern, `Engine/Plugins/Experimental/TechAudioTools/` ships a
ListView-with-per-item-VMs setup that's worth a read.
