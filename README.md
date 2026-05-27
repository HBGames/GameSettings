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
- Three ways to register settings. The default is to author a settings
  DataAsset and drop it in a content folder; the discovery subsystem finds
  it and registers it on every LocalPlayer. You can also register from C++
  or from a GameFeaturePlugin action. See `CONTRIBUTING_SETTINGS.md`.
- An MVVM-driven UMG widget set (screen, list, detail, entry widgets)
  built on UE5's `ModelViewViewModel` plugin. UI artists wire bindings
  in the View Bindings panel; no per-widget C++. See `MVVM_GUIDE.md`.
- A change tracker so each menu open is a transaction that can be
  committed (Apply) or rolled back (Cancel).

If you've used Lyra's GameSettings, the parts you remember are still here.
The shape that changed: you no longer subclass `UGameSettingRegistry` and
write one giant `OnInitialize`. You can if you want to, but most settings
are now authored as DataAssets and never touch C++ at all. A setting's
identity is the `FPrimaryAssetId` of the asset that contributes it, not a
gameplay tag. See `IDENTITY_AND_ASSETS.md` for what that means in practice.

## The shape of things

```
ULocalPlayer
  +-- UGameSettingsSubsystem          owns the registry for this player
  |     +-- UGameSettingRegistry      tree of settings, lifetime = LocalPlayer
  |           +-- TopLevelSettings (tabs)
  |           |     +-- UGameSettingCollection
  |           |           +-- child UGameSetting instances
  |           +-- SettingsByHandle    FGameSettingHandle to setting lookup
  +-- UGameSettingsViewModelSubsystem owns the screen view model (the UI side)

UGameInstance
  +-- UGameSettingsAssetDiscoverySubsystem  watches the asset registry for
                                            contribution DataAssets
```

Three things put settings into the registry:

- A contribution DataAsset (a `UGameSettingsContribution`) saved under any
  mounted content path. `UGameSettingsAssetDiscoverySubsystem` finds it
  through the asset registry and applies it to every LocalPlayer's
  subsystem. This is the default path and needs no code. Set `bEnabled`
  to false on the asset to skip it without loading.
- A `UGameSettingsAutoContributor` subclass for code-only settings. The
  module's class loader finds its CDO at startup and applies it to every
  LocalPlayer that exists or arrives later.
- A `UGameFeatureAction_RegisterGameSettings` on a GameFeatureData asset,
  for the cherry-picking case where you want explicit ordering or want to
  register a contribution that lives outside the GFP's own content folder.

Every Add returns a `FGameSettingHandle`. Whoever registered the setting
holds the handle and calls `RemoveByHandle` on teardown. The registry
doesn't track contributors; the handle is the cleanup token. For the two
auto paths the plugin holds the handles for you.

The settings screen reads the registry through the view-model subsystem. It
owns a per-screen `FGameSettingRegistryChangeTracker` that watches for
in-flight edits and either applies or restores them when the user hits the
buttons. Closing the screen doesn't destroy anything; the registry stays on
the LocalPlayer.

## Quick start: add one setting

Say your game has a "show damage numbers" toggle on a settings object with a
reflected getter/setter:

```cpp
UCLASS(Config = Game)
class UMyGameSettings : public ULocalPlayerSaveGame
{
    GENERATED_BODY()
public:
    UFUNCTION() bool GetShowDamageNumbers() const { return bShowDamageNumbers; }
    UFUNCTION() void SetShowDamageNumbers(bool b) { bShowDamageNumbers = b; }
private:
    UPROPERTY() bool bShowDamageNumbers = true;
};
```

You don't need any more C++. In the editor:

1. Make sure a `Game Setting Tab` asset exists for the tab you want (say
   `Tab_Gameplay`). If not, create one: Content Browser, right-click,
   Miscellaneous, Data Asset, `Game Setting Tab`.
2. Create a `Game Setting Toggle` asset (same menu). Set its DisplayName and
   Description. Under Display, set ParentContainer to `Tab_Gameplay`.
3. Under Value, set the Binding: pick `UMyGameSettings` as the TargetClass,
   then pick `GetShowDamageNumbers` and `SetShowDamageNumbers` from the
   getter/setter dropdowns (the editor ranks the candidate UFUNCTIONs by
   name similarity to the asset).
4. Save it under a mounted content path.

That's it. Open your settings screen and the toggle shows up. The asset's
name is its identity, so renaming it later moves the identity with it via
the asset manager.

For the C++ and GameFeature paths, see `CONTRIBUTING_SETTINGS.md`.

## When to pick which contribution path

For most settings, a DataAsset is the right default. No code, the editor
validates the binding at save time, and the asset auto-registers on every
LocalPlayer.

Write a `UGameSettingsAutoContributor` when the settings are owned by an
always-on plugin and you'd rather keep them in C++ than ship a content
asset: an engine module, an accessibility shim, debug toggles. The CDO is
discovered at startup; no registration call.

Use the GFP action when settings should appear and disappear with a content
drop and you need explicit control. Most GFP-shipped DataAssets register
automatically through discovery once the GFP's content mounts, so reach for
the action only when you want ordering or out-of-folder registration.

For anything dynamic (one slider per detected audio device, one row per
mappable input), build the settings in C++ and call
`Registry->AddSetting(...)` directly, holding the handles yourself.

## What's where

```
GameSettings/
+-- Source/
|   +-- GameSettings/                  runtime module (no GameFeatures dep)
|   |   +-- Public/
|   |   |   +-- GameSetting.h                       base class for settings
|   |   |   +-- GameSettingRegistry.h               Add/Remove/Find API
|   |   |   +-- GameSettingsSubsystem.h             per-LocalPlayer owner
|   |   |   +-- GameSettingsViewModelSubsystem.h    per-LocalPlayer VM owner
|   |   |   +-- GameSettingsAssetDiscoverySubsystem.h  asset-registry watcher
|   |   |   +-- GameSettingsContribution.h          contribution base
|   |   |   +-- GameSettingsAutoContributor.h
|   |   |   +-- GameSettingsDeveloperSettings.h     Project Settings page
|   |   |   +-- Contributions/                      typed DataAsset classes
|   |   |   +-- DataSource/                         getter/setter binding
|   |   |   +-- EditCondition/                      visibility/enable rules
|   |   |   +-- ViewModels/                         per-setting + screen VMs
|   |   |   +-- Widgets/                            UMG screen + entries
|   |   +-- Private/
|   +-- GameSettingsGameFeatures/      GFP bridge (RegisterGameSettings, AddViewBindings)
|   +-- GameSettingsEditor/            binding customization + UFUNCTION dropdown
|   +-- GameSettingsTests/             automation tests
+-- README.md                         you are here
+-- Docs/
    +-- CONTRIBUTING_SETTINGS.md       contribution paths in detail
    +-- IDENTITY_AND_ASSETS.md         PrimaryAssetId identity + asset naming
    +-- EDIT_CONDITIONS.md             declarative visibility/enable rules
    +-- MVVM_GUIDE.md                  UI artist workflow
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

Id collisions are warnings, not crashes. The original Lyra code asserted on
duplicate identifiers. With independent contributors that's too brittle, so
the plugin logs the collision and lets both settings live in the registry.
The fix is to give each contribution asset a distinct name, since the asset
name is the `FPrimaryAssetId`. Look settings up with `FindSettingById` or
`FindSettingByHandle`.

Settings outlive the menu. The registry sits on the LocalPlayer subsystem.
Ask `Subsystem->GetRegistry()->FindSettingById(...)` from anywhere, any
time. The menu closing doesn't tear anything down.

Late-joining LocalPlayers (split-screen joiners) automatically get
auto-contributors and discovered DataAssets, but don't automatically get
GFP-action contributions from already-active GFPs. If you need that, the
GFP action will need an extension; for now, late-join during gameplay is
uncommon and we're not solving it.

The plugin is a fork of Lyra's GameSettings. Most of the value-type
hierarchy and the change-tracker logic is unchanged. The architecture work
is in the registry, the subsystems, the contribution layer, and the move
from gameplay-tag identity to `FPrimaryAssetId`.

## Where to look next

- Contribution paths with copy-paste code: `CONTRIBUTING_SETTINGS.md`
- Identity, asset naming, and the few remaining tag uses: `IDENTITY_AND_ASSETS.md`
- Declarative visibility/enable rules: `EDIT_CONDITIONS.md`
- UI artist workflow for the MVVM widget set: `MVVM_GUIDE.md`
