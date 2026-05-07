# GameSettings

A settings system for UE5 projects. The original came from Lyra; this fork
keeps Lyra's good parts (the value/scalar/discrete hierarchy, edit
conditions, the change tracker) and rebuilds the registry around
contribution from arbitrary modules and GameFeaturePlugins instead of one
hardcoded subclass.

## What's in here

The plugin gives you four things:

- A settings registry that lives on the LocalPlayer for that player's
  whole life. Ask it about settings any time, not just while a menu is open.
- Three ways to register settings: via a GameFeature action, via an
  auto-discovered CDO, or by calling Add directly.
- An MVVM-driven UMG widget set (screen, list, detail, entry widgets)
  built on UE5's `ModelViewViewModel` plugin. UI artists wire bindings
  in the View Bindings panel; no per-widget C++. See `MVVM_GUIDE.md`.
- A change tracker so each menu open is a transaction that can be
  committed (Apply) or rolled back (Cancel).

If you've used Lyra's GameSettings, the parts you remember are still here.
The shape that changed: you no longer have to subclass
`UGameSettingRegistry` and write one giant `OnInitialize`. You can if you
want to, but contributing from a plugin or GFP doesn't require it.

## The shape of things

```
ULocalPlayer
  └── UGameSettingsSubsystem        owns the registry for this player
        └── UGameSettingRegistry    tree of settings, lifetime = LocalPlayer
              ├── TopLevelSettings (tabs)
              │     └── UGameSettingCollection
              │           └── child UGameSetting instances
              └── SettingsByHandle  FGameSettingHandle to setting lookup
```

Three things put settings into the registry:

- A GameFeaturePlugin ships a `UGameFeatureAction_RegisterGameSettings` on
  its data asset. When the GFP activates, every contribution in the action
  is applied to every LocalPlayer. When it deactivates, they're removed
  cleanly.
- A runtime plugin (an always-on plugin, not a GFP) defines a
  `UGameSettingsAutoContributor` subclass. The plugin's class loader finds
  it at startup; the subsystem applies it to every LocalPlayer that exists
  or arrives later.
- Game code calls `Subsystem->GetOrCreateRegistry()->AddSetting(...)`
  directly when it has a specific moment to register.

Every Add returns a `FGameSettingHandle`. Whoever called Add holds onto it
and calls RemoveByHandle on teardown. The registry doesn't track
contributors; the handle is the cleanup token.

The settings screen reads the registry through the subsystem. It owns a
per-screen `FGameSettingRegistryChangeTracker` that watches for in-flight
edits and either applies or restores them when the user hits the buttons.
Closing the screen doesn't destroy anything; the registry stays on the
LocalPlayer.

## Quick start: add one setting from your game module

Say your game has a "show damage numbers" toggle on `UMyGameSettings`:

```cpp
// MyGameSettings.h
UCLASS(Config = Game)
class UMyGameSettings : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(Config) bool bShowDamageNumbers = true;
    UFUNCTION() bool GetShowDamageNumbers() const { return bShowDamageNumbers; }
    UFUNCTION() void SetShowDamageNumbers(bool b) { bShowDamageNumbers = b; SaveConfig(); }
};
```

To get a settings entry for it, write an auto-contributor:

```cpp
// MyGameSettings_AutoContributor.h
UCLASS()
class UMyGameSettings_AutoContributor : public UGameSettingsAutoContributor
{
    GENERATED_BODY()
public:
    virtual void Apply(UGameSettingRegistry& Registry,
                       TArray<FGameSettingHandle>& OutHandles) override
    {
        UGameSettingValueDiscreteDynamic_Bool* Toggle =
            NewObject<UGameSettingValueDiscreteDynamic_Bool>(&Registry);

        Toggle->SetSettingId(
            FGameplayTag::RequestGameplayTag("Settings.MyGame.ShowDamageNumbers"));
        Toggle->SetDisplayName(
            NSLOCTEXT("MyGame", "ShowDamage_Name", "Show damage numbers"));
        Toggle->SetDescriptionRichText(
            NSLOCTEXT("MyGame", "ShowDamage_Desc",
                "Display a floating number when an enemy is hit."));
        Toggle->SetDefaultValue(true);
        // (Binding to the getter/setter omitted for brevity. See
        //  Docs/CONTRIBUTING_SETTINGS.md for the full pattern.)

        OutHandles.Add(Registry.AddSetting(
            Toggle,
            FGameplayTag::RequestGameplayTag("Settings.Tab.Gameplay")));
    }
};
```

That's it. Build, register the tag in your project's tag table, open your
settings screen. The toggle shows up.

## When to pick which contribution path

For most projects, an auto-contributor is the right default. It's terse,
it doesn't depend on GameFeatures, and it fires on every LocalPlayer,
including split-screen joiners.

Use the GFP action when the settings should appear and disappear with a
content drop. Beta features behind a flag, cosmetic packs, mode-specific
options. They all want symmetric activation, and that's what the action
gives you.

Use direct C++ for anything dynamic. One slider per detected audio output
device. One keybind row per active mappable input. Debug toggles that only
exist in non-shipping. The pattern is the same; you just hold the handles
yourself.

## What's where

```
GameSettings/
├── Source/
│   ├── GameSettings/                           runtime module (no GameFeatures dep)
│   │   ├── Public/
│   │   │   ├── GameSetting.h                   base class for settings
│   │   │   ├── GameSettingRegistry.h           Add/Remove/Find API
│   │   │   ├── GameSettingsSubsystem.h         per-LocalPlayer owner
│   │   │   ├── GameSettingsContribution.h      contribution base
│   │   │   ├── GameSettingsAutoContributor.h
│   │   │   ├── GameSettingsDeveloperSettings.h Project Settings page
│   │   │   ├── GameSettingHandle.h
│   │   │   ├── DataSource/                     getter/setter binding
│   │   │   ├── EditCondition/                  visibility/enable rules
│   │   │   └── Widgets/                        UMG screen + entries
│   │   └── Private/
│   └── GameSettingsGameFeatures/               bridge module (GameFeatures dep)
│       ├── Public/
│       │   └── GameFeatureAction_RegisterGameSettings.h
│       └── Private/
└── Docs/
    ├── README.md                               you are here
    ├── CONTRIBUTING_SETTINGS.md                contribution paths in detail
    ├── TAG_CONVENTIONS.md                      FGameplayTag namespace contract
    └── MVVM_GUIDE.md                           UI artist workflow
```

## Configuring the registry class

Most projects don't need to subclass `UGameSettingRegistry`. If yours does
(e.g. for a custom `SaveChanges` that hits your save backend instead of the
default GConfig + SaveGame combo):

1. Open Project Settings -> Game -> Game Settings.
2. Set Registry Class to your subclass.
3. Done. The subsystem instantiates your class on first need.

If you'd rather build the registry yourself (for example from a screen
widget), construct it and hand it over with
`Subsystem->SetRegistry(MyRegistry)`.

## Things to know

The handle invariant matters. Drop a handle on the floor and the setting
is permanent for that registry's lifetime. Not the worst leak; it doesn't
crash. But it shows up as duplicate entries on hot-reload, and
`LogGameSettings` Verbose logs every Add and Remove if you want to audit.

DevName collisions are warnings, not crashes. The original Lyra code
asserted on duplicate DevNames. With independent contributors, that's too
brittle. The plugin now logs and lets both settings live in the registry;
the right fix is to give each setting a distinct `SettingId` tag.
`FindSettingByTag` is the lookup API.

Settings outlive the menu. The registry sits on the LocalPlayer subsystem.
Ask `Subsystem->GetRegistry()->FindSettingByTag(...)` from anywhere, any
time. The menu closing doesn't tear anything down.

Late-joining LocalPlayers (split-screen joiners) automatically get
auto-contributors but don't automatically get GFP-action contributions
from already-active GFPs. If you need that, the GFP action will need an
extension; for now, late-join during gameplay is uncommon and we're not
solving it.

The plugin is a fork of Lyra's GameSettings. Most of the value-type
hierarchy and the change-tracker logic is unchanged. The architecture
work is in the registry, the subsystem, and the contribution layer. If
you've shipped Lyra-based settings code before, it'll feel familiar.

## Where to look next

- Contribution paths with copy-paste code: `CONTRIBUTING_SETTINGS.md`
- Tag namespace rules: `TAG_CONVENTIONS.md`
- UI artist workflow for the MVVM widget set: `MVVM_GUIDE.md`
