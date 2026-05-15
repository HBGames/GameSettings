# Edit conditions

Every typed contribution (`UGameSettingsContribution_*`) carries an `EditConditions` array of polymorphic spec objects. The specs run on the resulting `UGameSetting` at registration time and continue to evaluate as referenced settings change. Specs can hide, disable, kill (hide + non-resetable + non-analytics), or per-option-disable the setting.

The array is implicit AND: every spec contributes to the same `FGameSettingEditableState`. A single spec can call any combination of `Hide`, `Disable`, `Kill`, `DisableOption`, `UnableToReset`, `HideFromAnalytics` on the state.

## Built-in spec types

| Class | What it does |
| --- | --- |
| `Depends On Toggle` | Predicate fails when target Toggle's bool value differs from `bRequiredValue`. |
| `Depends On Discrete` | Predicate fails when target Discrete's option string is not in `RequiredValues` (or IS in, when `bInvertMatch`). Editor shows a checkbox picker sourced from the target's `Options`. |
| `Depends On Scalar` | Predicate fails when the comparison `TargetValue Op Threshold` is false. |
| `Platform Trait` | Predicate fails when the supplied `Platform.Trait.*` tag is missing (or present, when `When == IfPresent`). Wraps `FWhenPlatformHasTrait`. Action must be Disable or Kill. |
| `Primary Player Only` | Wraps `FWhenPlayingAsPrimaryPlayer`. Disables the setting for splitscreen guests. Action must be Disable. |
| `Disable Discrete Options When` | Per-option disabling on a Discrete contribution. Holds an inner Predicate (one of the four above); when the predicate fires, the listed option values are removed from the user-selectable list. |
| `Blueprint Condition Bridge` | Authoring entry point for one-off Blueprint conditions. Override `BP_EvaluateState`; mutate the supplied state via the BP helper library. |

## Action

Every spec carries an `Action` enum:

- `Disable`: visible but greyed out. Requires `DisableReason` (user-facing FText).
- `Hide`: visually removed only. Requires `DevReason` (dev-only FString).
- `Kill`: hidden + non-resetable + hidden from analytics. Requires `DevReason`.

`Platform Trait` and `Primary Player Only` constrain which actions are legal; the validators flag invalid combinations.

## Cross-setting wiring

When a spec references another setting via `FPrimaryAssetId`, the registry:

1. Resolves the target by id at install time.
2. Calls `AddEditDependency(Target)` on the owning setting so the owner refreshes when the target changes.
3. Defers installation if the target hasn't been registered yet. The deferred queue flushes after every `AddSetting` / `AddCollection`.

Contributions can arrive in any order. A row that references a target in another GameFeature plugin will wire up as soon as the target's GFP activates.

When the referenced target is removed (`RemoveById` or `RemoveByHandle`, e.g. on GFP unload), the registry walks every applied condition that pointed at the target, drops the condition from the owner, and refreshes the owner's editable state. Owner-side cleanup is handled by the existing setting-tear-down path.

## Lyra VSync, translated

Lyra wires VSync in C++:

```cpp
Setting->AddEditCondition(MakeShared<FGameSettingEditCondition_FramePacingMode>(ELyraFramePacingMode::DesktopStyle));
Setting->AddEditDependency(WindowModeSetting);
Setting->AddEditCondition(MakeShared<FWhenCondition>([WindowModeSetting](const ULocalPlayer*, FGameSettingEditableState& InOutEditState) {
    if (WindowModeSetting->GetValue<EWindowMode::Type>() != EWindowMode::Fullscreen) {
        InOutEditState.Disable(LOCTEXT("FullscreenNeededForVSync", "This feature only works if 'Window Mode' is set to 'Fullscreen'."));
    }
}));
```

The same setup as authored data on `DA_Setting_VSync.uasset`:

```
EditConditions:
  - Platform Trait
      VisibilityTag: Platform.Trait.FramePacing.DesktopStyle
      When: IfMissing
      Action: Kill
      DevReason: "VSync is a desktop-style frame-pacing concept."
  - Depends On Discrete
      TargetSetting: DA_Setting_WindowMode
      RequiredValues: ["Fullscreen"]
      bInvertMatch: false
      Action: Disable
      DisableReason: "This feature only works if 'Window Mode' is set to 'Fullscreen'."
```

Add the platform trait `Platform.Trait.FramePacing.DesktopStyle` to platforms that support the C++ frame-pacing-mode gate.

## Blueprint conditions

Subclass `UGameSettingEditConditionSpec_BlueprintBridge` ("Blueprint Condition Bridge"). Implement `BP_EvaluateState(LocalPlayer, State)`. Mutate `State` via the `Edit Condition` BP function library (`Disable`, `Hide`, `Kill`, `Disable Option`, `Unable To Reset`, `Hide From Analytics`). The state pointer is only valid inside that event; do not store it.

If you need to react to another setting's value, find the screen view model or the registry and call `FindSettingById` inside the event.

## Limitations

- OR is intentionally not supported declaratively. Multiple entries in `EditConditions` AND together. For OR, write a single Blueprint Condition Bridge that gathers from both branches and applies the looser result.
- Per-option disabling cannot nest. Validation rejects a `Disable Discrete Options When` predicate that is itself another `Disable Discrete Options When`.
- A `Disable Discrete Options When` whose owning contribution is not a Discrete is rejected at asset save.
- Specs read target values via the verified public APIs: `UGameSettingValueBool::GetBoolValue`, `UGameSettingValueDiscreteDynamic::GetValueAsString`, `UGameSettingValueScalar::GetValue`. Custom `UGameSetting` subclasses that override only `Initialize` and not the value path may not interact correctly.
- Hot-reloading a contribution asset in PIE is supported; the spec lambdas read live data so changing field values updates behavior at the next refresh. Adding or removing specs while PIE is running does not retroactively wire-up; restart PIE.

## See also

- `Docs/CONTRIBUTING_SETTINGS.md` for the contribution authoring flow.
- `Docs/MVVM_GUIDE.md` for the view model layer that surfaces `IsEnabled`, `IsVisible`, and `DisabledReasons` to widgets.
- `EditCondition/WhenCondition.h`, `WhenPlatformHasTrait.h`, `WhenPlayingAsPrimaryPlayer.h` for the underlying C++ machinery.
