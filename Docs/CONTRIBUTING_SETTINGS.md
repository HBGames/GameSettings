# Adding settings to the registry

Four ways in. The first is the default and needs no code; the rest are for
cases the default doesn't cover.

> Identity and naming (how a contribution's `FPrimaryAssetId` is formed, how
> rows reference their parent) live in [IDENTITY_AND_ASSETS.md](IDENTITY_AND_ASSETS.md).
> Visibility / enable rules live in [EDIT_CONDITIONS.md](EDIT_CONDITIONS.md).

| You're adding... | Use |
|---|---|
| A normal setting authored in the editor | DataAsset (auto-discovered) |
| Settings owned by an always-on plugin, kept in C++ | Auto-contributor |
| Settings that need explicit ordering or out-of-folder registration in a GFP | GameFeature action |
| A setting built at runtime from data you don't have until then | Direct C++ |

All four end at the same registry and the same handle-based register/remove
contract.

---

## 1. DataAsset (the default)

Author the setting as a `UGameSettingsContribution` DataAsset and save it
under a mounted content path. `UGameSettingsAssetDiscoverySubsystem` watches
the asset registry, finds it, and applies it to every LocalPlayer's
`UGameSettingsSubsystem`. New content mounts (a GFP activating) and editor
saves are caught the same way. Set `bEnabled = false` on the asset to skip
it without loading the uasset.

### The typed contribution classes

Right-click in the Content Browser, Miscellaneous, Data Asset, then pick:

| Class | Purpose |
|---|---|
| Game Setting Tab | Top-level page. Other contributions name it as their `ParentContainer` to nest. Binds to nothing. |
| Game Setting Section | Grouping container under a Tab (or another Section). Binds to nothing. |
| Game Setting Toggle | Bool setting. Binds to a bool getter/setter. |
| Game Setting Scalar | Slider. `SourceRange`, `SourceStep`, optional `MinimumLimit` / `MaximumLimit`, and a `DisplayFormat` (eight format options from raw to percent). Binds to a numeric getter/setter. |
| Game Setting Discrete | Option list. A static `Options` array of value/label pairs, or an `OptionsProvider` for a runtime-generated list, or a custom `SettingClass` that self-manages. Binds to a string getter/setter. |
| Game Setting Action | Button. Carries an `ActionText` label and a `NamedAction` tag broadcast on click. |

Every typed contribution shares DisplayName, Description, SortPriority, and
the `EditConditions` array. Rows and sections also share `ParentContainer`.

### The binding

Toggle / Scalar / Discrete each carry an `FGameSettingsBinding`. It points
at the storage that actually holds the value:

- `TargetClass` is the class that owns the getter/setter. It must derive
  from one of: a `USubsystem` (LocalPlayer / GameInstance / World / Engine),
  `UGameUserSettings`, or `ULocalPlayerSaveGame`. The binding picks the right
  data-source family from the class shape at runtime.
- `GetterFunctionName` and `SetterFunctionName` are `UFUNCTION`s on that
  class. The editor dropdown lists only functions whose shape matches the
  contribution (a zero-arg bool getter for a Toggle, etc.) and ranks them by
  name similarity to the asset name, so `Toggle_Subtitles` floats
  `GetSubtitlesEnabled` to the top.
- `SaveGameSlotName` only applies when `TargetClass` is a
  `ULocalPlayerSaveGame`; leave it blank to default to the class name.

The editor validates the binding at save time. If the named functions don't
exist on the class, or the getter returns the wrong type, you get a data
validation error before the asset ever reaches the registry.

By default Reset-To-Default reads the value the getter returns on the
`TargetClass` CDO, so it tracks the C++ default and can't silently drift.
Uncheck `bUseClassDefaultValue` to set an explicit default field instead.

In this project the usual targets are `UEFPSettingsLocal` (video / audio, a
`UGameUserSettings` subclass) and `UEFPSettingsShared` (accessibility /
mouse, a `ULocalPlayerSaveGame`).

---

## 2. Auto-contributor (C++, always on)

For settings owned by an always-on plugin that you'd rather keep in code
than ship as a content asset. Subclass `UGameSettingsAutoContributor` and
implement `Apply`. The module's class loader finds the CDO via
`GetDerivedClasses` at startup and applies it to every LocalPlayer, including
ones that join later.

```cpp
UCLASS()
class UMyPlugin_AutoContributor : public UGameSettingsAutoContributor
{
    GENERATED_BODY()
public:
    virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override
    {
        UGameSettingValueBool* Toggle = NewObject<UGameSettingValueBool>(&Registry);
        Toggle->SetSettingId(FPrimaryAssetId(TEXT("GameSettingsToggle"), TEXT("Toggle_HighContrastUI")));
        Toggle->SetDisplayName(NSLOCTEXT("MyPlugin", "HighContrast_Name", "High contrast UI"));
        Toggle->SetDescriptionRichText(NSLOCTEXT("MyPlugin", "HighContrast_Desc", "Increase UI contrast for readability."));
        Toggle->SetDynamicGetter(GET_SUBSYSTEM_SETTINGS_FUNCTION_PATH(UMyPluginSubsystem, GetHighContrast));
        Toggle->SetDynamicSetter(GET_SUBSYSTEM_SETTINGS_FUNCTION_PATH(UMyPluginSubsystem, SetHighContrast));

        OutHandles.Add(Registry.AddSetting(
            Toggle,
            FPrimaryAssetId(TEXT("GameSettingsTab"), TEXT("Tab_Accessibility"))));   // parent tab
    }

    /** Optional. Gate registration at runtime. */
    virtual bool ShouldAutoContribute() const override
    {
        return FApp::CanEverRender();   // or a CVar, or a platform check
    }
};
```

Note the difference from the DataAsset path: in code you set the setting's
`FPrimaryAssetId` and its parent id by hand, and you bind storage with the
`GET_*_SETTINGS_FUNCTION_PATH` macros rather than an `FGameSettingsBinding`.
That's the low-level mechanism the typed contributions wrap. The macros and
the available value types are covered under "Plugin-owned backing storage"
below.

Pick this path for an engine module, an accessibility shim, debug toggles,
anything that shouldn't depend on a content asset or a GFP being active.

---

## 3. GameFeature action (cherry-pick)

Most settings shipped inside a GameFeaturePlugin register automatically
through discovery once the GFP's content mounts. Reach for
`UGameFeatureAction_RegisterGameSettings` only when you need explicit
ordering, or want to register a contribution that lives outside the GFP's
own content folder. (Listing an in-folder asset that discovery also finds
is safe — the subsystem de-duplicates per contribution — it's just
redundant.)

1. Open your `UGameFeatureData` asset.
2. Add a "Register Game Settings" action.
3. Fill its `Contributions` array with the `UGameSettingsContribution`
   assets (or subclasses) you want registered when this GFP activates.

The action applies its contributions per LocalPlayer on activation and
removes them symmetrically on deactivation. If a LocalPlayer leaves
mid-session (split-screen drop), the deactivation pass skips it through a
weak-ref check.

There's a sibling action, `Add Game Settings View Bindings`, that pushes a
GFP-specific widget bindings asset onto an override stack. See
`MVVM_GUIDE.md`.

---

## 4. Direct C++

Use this when you have an explicit moment to register and a specific
LocalPlayer in mind. One slider per detected audio output device. One keybind
row per active mappable input. Debug toggles that only exist in non-shipping.

```cpp
ULocalPlayer* LocalPlayer = ...;
UGameSettingsSubsystem* Subsystem = LocalPlayer->GetSubsystem<UGameSettingsSubsystem>();
UGameSettingRegistry* Registry = Subsystem->GetOrCreateRegistry();

UGameSettingAction* OpenCalibrationButton = NewObject<UGameSettingAction>(Registry);
OpenCalibrationButton->SetSettingId(FPrimaryAssetId(TEXT("GameSettingsAction"), TEXT("Action_CalibrateHDR")));
OpenCalibrationButton->SetDisplayName(LOCTEXT("HDRCalibrate_Name", "Calibrate HDR"));
OpenCalibrationButton->SetActionText(LOCTEXT("HDRCalibrate_Action", "Open"));
OpenCalibrationButton->SetCustomAction(FGameSettingCustomAction::CreateLambda(
    [this](ULocalPlayer*, UGameSetting*) { OpenHDRCalibrationScreen(); }));

const FGameSettingHandle MyHandle = Registry->AddSetting(
    OpenCalibrationButton,
    FPrimaryAssetId(TEXT("GameSettingsTab"), TEXT("Tab_Video")));

// Hold MyHandle. Call Registry->RemoveByHandle(MyHandle) when you're done.
```

Pick this path for runtime-discovered settings, tests, debugging, or
anywhere you'd otherwise be tempted to subclass the registry.

---

## The handle contract

Every `Add*` returns a `FGameSettingHandle`. Whoever called `Add` keeps that
handle and calls `Registry->RemoveByHandle` on teardown. The registry never
holds back-pointers to contributors, which is what makes it safe across
plugin unload.

In practice:

- The GameFeature action retains its handles in `ActiveContributions` and
  removes them on `OnGameFeatureDeactivating`.
- Auto-contributor and discovered-DataAsset handles are tracked by
  `UGameSettingsSubsystem` itself and torn down on `Deinitialize` (or when
  the source asset is removed).
- Direct C++ callers manage their own handles.

Drop a handle and the setting persists for the rest of the registry's
lifetime (the LocalPlayer's lifetime). It doesn't crash or corrupt anything,
but you'll see duplicate entries on hot-reload. `LogGameSettings` Verbose
logs every Add and Remove, so auditing is easy.

---

## Plugin-owned backing storage

The Lyra-style `GET_LOCAL_SETTINGS_FUNCTION_PATH` and
`GET_SHARED_SETTINGS_FUNCTION_PATH` macros resolve through the project's
`ULocalPlayer` subclass. If your plugin owns its own backing storage on a
subsystem and you don't want to drag the project's LocalPlayer into it, use
`GET_SUBSYSTEM_SETTINGS_FUNCTION_PATH` (the code equivalent of pointing an
`FGameSettingsBinding` at a subsystem `TargetClass`):

```cpp
UCLASS()
class UMyPluginSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()
public:
    UFUNCTION() bool GetMyToggle() const { return bMyToggle; }
    UFUNCTION() void SetMyToggle(bool b) { bMyToggle = b; }
private:
    UPROPERTY(Config) bool bMyToggle = false;
};
```

```cpp
Setting->SetDynamicGetter(GET_SUBSYSTEM_SETTINGS_FUNCTION_PATH(UMyPluginSubsystem, GetMyToggle));
Setting->SetDynamicSetter(GET_SUBSYSTEM_SETTINGS_FUNCTION_PATH(UMyPluginSubsystem, SetMyToggle));
```

The macro infers the subsystem family (LocalPlayer, GameInstance, World,
Engine) from the class hierarchy at resolve time, the same way
`FGameSettingsBinding` does for DataAsset-authored settings.

---

## Configuring the project's registry class

Most projects don't need to subclass `UGameSettingRegistry`. If yours does
(a global `SaveChanges` override, for example):

1. Open Project Settings -> Game -> Game Settings.
2. Set Registry Class to your subclass.
3. Done. The subsystem auto-builds it when needed.

You can also build the registry yourself and hand it to the subsystem with
`Subsystem->SetRegistry(...)` if you have a specific moment to provision it
(e.g. from a screen widget).

---

## Common mistakes

- Dropping the handle on the floor. `AddSetting` returns the only token that
  removes the setting later. The registry holds the strong ref; you hold the
  cleanup token.
- Colliding asset names. The id is `<Type>:<AssetName>`, so two toggles both
  named `Subtitles` share an id. The second `AddSetting` logs a warning and
  keeps both rather than asserting. Give each asset a distinct name.
- Adding the same `UGameSetting*` instance twice. A setting lives in one
  registry slot. The second add warns, no-ops, and returns the setting's
  existing handle.
- Holding `UGameSetting*` across hot-reload. Settings can be torn down and
  rebuilt by `Regenerate()` or a GFP unmount. Auto-contributed and
  discovered-asset settings come back automatically after a `Regenerate()`,
  but as new objects with new handles — old handles are dead, so re-resolve
  by id through `FindSettingById`. Settings registered through
  `ApplyContribution` directly (the GFP action path) or by hand with
  `AddSetting` do NOT come back; the registering code must re-apply (for a
  GFP, deactivate/reactivate the feature).
