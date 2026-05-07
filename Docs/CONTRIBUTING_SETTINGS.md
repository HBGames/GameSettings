# Adding settings to the registry

Three contribution paths. Pick the one that matches where your settings
code lives.

| You're writing... | Use |
|---|---|
| A GameFeaturePlugin (cosmetic pack, beta features, content drop) | GFP action |
| A runtime plugin (core engine module, gameplay framework) | Auto-contributor |
| Game code that needs to add a setting at a specific moment | Direct C++ |

All three end up at the same registry and the same handle-based
register/remove contract.

---

## 1. GFP action: `UGameFeatureAction_RegisterGameSettings`

This one fits content shipped as a GameFeaturePlugin. Settings show up
when the GFP activates and go away when it deactivates.

### Setup

1. Open your `UGameFeatureData` asset (`GameFeatureData_<YourGFP>.uasset`).
2. Add a new action of type "Register Game Settings".
3. In the action's `Contributions` array, add an entry for each
   `UGameSettingsContribution` subclass you want to ship. The pickers
   here are `Instanced`: pick a class, fill in its properties inline.

### Typed contribution subclasses

The plugin ships five inline-editable contribution types:

| Class | Purpose |
|---|---|
| `Game Setting Tab` | Top-level tab keyed by `SettingId`. Other contributions reference its tag as their `ParentTab` to nest. |
| `Game Setting Toggle` | Bool setting bound to a subsystem getter/setter. |
| `Game Setting Scalar` | Slider with source range, step, optional clamps, and one of eight format functions. |
| `Game Setting Discrete` | Option-list setting (graphics quality preset, language picker, etc.) bound to a string-keyed getter/setter. |
| `Game Setting Action` | Button that broadcasts a `NamedAction` tag. The screen handler routes by tag. |

The typed contributions use `FGameSettingsBinding` for property paths.
Pick the subsystem class (any `ULocalPlayerSubsystem`,
`UGameInstanceSubsystem`, `UWorldSubsystem`, or `UEngineSubsystem`),
then type or pick the getter/setter `UFUNCTION` names. Editor
validation checks the functions actually exist on the class at asset
save time.

### Code-side contribution subclass

If the typed ones don't fit, write your own subclass:

```cpp
UCLASS(EditInlineNew)
class UMyPlugin_Settings_Foo : public UGameSettingsContribution
{
    GENERATED_BODY()
public:
    virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override
    {
        UGameSettingValueDiscreteDynamic_Bool* Setting = NewObject<UGameSettingValueDiscreteDynamic_Bool>(&Registry);
        Setting->SetSettingId(FGameplayTag::RequestGameplayTag("Settings.MyPlugin.Foo"));
        Setting->SetDisplayName(NSLOCTEXT("MyPlugin", "Foo_Name", "Enable Foo"));
        Setting->SetDescriptionRichText(NSLOCTEXT("MyPlugin", "Foo_Desc", "Toggles whatever Foo does."));
        Setting->SetDynamicGetter(/* property path to your getter */);
        Setting->SetDynamicSetter(/* property path to your setter */);
        Setting->SetDefaultValue(true);

        const FGameSettingHandle Handle = Registry.AddSetting(
            Setting,
            FGameplayTag::RequestGameplayTag("Settings.Tab.Audio"));   // parent tab tag

        OutHandles.Add(Handle);
    }
};
```

The action handles the rest: per-LocalPlayer apply on activation,
symmetric remove on deactivation. If a LocalPlayer leaves mid-session
(split-screen drop), the deactivation pass skips them via weak-ref.

Pick this path when your settings ship inside a GFP and should track
the GFP's activation state.

---

## 2. Auto-contributor: `UGameSettingsAutoContributor`

This one fits plugins that always want their settings present, no
matter what shape the project takes. The plugin's class loader
discovers your contributor at module startup and applies it to every
LocalPlayer.

```cpp
UCLASS()
class UMyPlugin_AutoContributor : public UGameSettingsAutoContributor
{
    GENERATED_BODY()
public:
    virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override
    {
        // Same as the GFP action's subclass. Build settings, add them to
        // the registry, append every handle to OutHandles.
    }

    /** Optional. Gate registration at runtime. */
    virtual bool ShouldAutoContribute() const override
    {
        return FApp::CanEverRender();   // or a CVar, or a platform check
    }
};
```

That's it. The CDO is discovered automatically; no registration call is
needed. New LocalPlayers (split-screen join, level transition) pick it
up through `UGameSettingsSubsystem`'s subscription to discovery events.

Pick this path for settings owned by an always-on plugin: an engine
module, an editor utility, an accessibility shim, anything that
shouldn't depend on a GFP being active.

---

## 3. Direct C++

Use this when you have an explicit moment to register and a specific
LocalPlayer in mind. One slider per detected audio output device. One
keybind row per active mappable input. Debug toggles that only exist
in non-shipping builds.

```cpp
ULocalPlayer* LocalPlayer = ...;
UGameSettingsSubsystem* Subsystem = LocalPlayer->GetSubsystem<UGameSettingsSubsystem>();
UGameSettingRegistry* Registry = Subsystem->GetOrCreateRegistry();

UGameSettingAction* OpenCalibrationButton = NewObject<UGameSettingAction>(Registry);
OpenCalibrationButton->SetSettingId(FGameplayTag::RequestGameplayTag("Settings.Video.HDR.Calibrate"));
OpenCalibrationButton->SetDisplayName(LOCTEXT("HDRCalibrate_Name", "Calibrate HDR"));
OpenCalibrationButton->SetActionText(LOCTEXT("HDRCalibrate_Action", "Open"));
OpenCalibrationButton->SetCustomAction(FGameSettingCustomAction::CreateLambda(
    [this](ULocalPlayer*, UGameSetting*) { OpenHDRCalibrationScreen(); }));

const FGameSettingHandle MyHandle = Registry->AddSetting(
    OpenCalibrationButton,
    FGameplayTag::RequestGameplayTag("Settings.Tab.Video"));

// Hold MyHandle. Call Registry->RemoveByHandle(MyHandle) when you're done.
```

Pick this path for runtime-discovered settings, tests, debugging, or
anywhere you'd otherwise be tempted to subclass the registry.

---

## The handle contract

Every `Add*` returns a `FGameSettingHandle`. Whoever called `Add` keeps
that handle and calls `Registry->RemoveByHandle` on teardown. The
registry never holds back-pointers to contributors, which is what makes
it safe across plugin unload.

In practice:

- The Game Feature Action retains its handles in `ActiveContributions`
  and removes them on `OnGameFeatureDeactivating`.
- Auto-contributor handles are tracked by `UGameSettingsSubsystem`
  itself and torn down on `Deinitialize`.
- Direct C++ callers manage their own handles.

If you forget to remove a handle, the setting persists for the rest of
the registry's lifetime (the LocalPlayer's lifetime). It doesn't crash
or corrupt anything, but you'll see duplicate entries on hot-reload.
The `LogGameSettings` Verbose channel logs every Add and Remove, so
auditing is easy.

---

## Tag conventions

See `Docs/TAG_CONVENTIONS.md` for the full namespace contract. The short
version:

- Tab tags: `Settings.Tab.<TabName>` (project-defined or per-plugin)
- Setting tags: `Settings.<Contributor>.<Group>.<Name>`
- Plugin namespace boundary: `Settings.<YourPlugin>.*`

DevName auto-derives from the tag's short name if you don't set it
explicitly.

---

## Plugin-owned backing storage

The Lyra-style `GET_LOCAL_SETTINGS_FUNCTION_PATH` and
`GET_SHARED_SETTINGS_FUNCTION_PATH` macros only resolve through the
project's `ULocalPlayer` subclass. If your plugin owns its own backing
storage on a subsystem and you don't want to drag the project's
LocalPlayer into it, use `FGameSettingDataSourceFromSubsystem`:

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

The macro infers the subsystem family (LocalPlayer, GameInstance,
World, Engine) from the class hierarchy at resolve time.

---

## Configuring the project's registry class

Most projects don't need to subclass `UGameSettingRegistry`. If yours
does (a global `SaveChanges` override, for example):

1. Open Project Settings -> Game -> Game Settings.
2. Set Registry Class to your subclass.
3. Done. The subsystem auto-builds it when needed.

You can also build the registry yourself and hand it to the subsystem
with `Subsystem->SetRegistry(...)` if you have a specific moment to
provision it (e.g. from a screen widget).

---

## Common mistakes

- Dropping the handle on the floor. `Registry->AddSetting(MySetting)`
  returns a handle. Drop it and you have no way to remove the setting
  later. The registry holds the strong ref; you hold the cleanup token.
- Adding the same `UGameSetting*` instance twice. A setting can only
  live in one registry slot. The second `AddSetting` warns and no-ops.
- Holding `UGameSetting*` across hot-reload. Settings can be torn down
  and rebuilt by `UGameSettingRegistry::Regenerate()` or by a GFP
  unmount. Cache by handle and re-resolve through `FindSettingByHandle`
  if you need to survive a structural change.
