# GameSettings tag conventions

Settings are identified by `FGameplayTag`. The hierarchy is the API
contract: it gives you free namespace isolation between contributors,
lets the UI group settings by parent tag, and lets you deprecate or
rename a tag without touching call sites that reference its parent.

## Roots owned by the plugin

The plugin ships exactly two root tags in `Config/Tags/GameSettings.ini`:

- `Settings`. Reserved as the root namespace; never use it directly.
- `Settings.Tab`. Root for top-level tabs.

Everything else is contributed by callers.

## Tab tags

Tabs are top-level pages in the settings screen. Two patterns exist.

Shared tabs are owned by the project and pushed into by multiple
contributors. Define these in your project's tag table:

```
+GameplayTagList=(Tag="Settings.Tab.Audio")
+GameplayTagList=(Tag="Settings.Tab.Video")
+GameplayTagList=(Tag="Settings.Tab.Gameplay")
+GameplayTagList=(Tag="Settings.Tab.Controls")
```

Plugin-specific tabs belong to one plugin or GameFeaturePlugin that
wants its own dedicated tab:

```
+GameplayTagList=(Tag="Settings.Tab.MyPlugin")
```

Use the plugin name verbatim. That's the namespace boundary.

## Setting tags

Settings nest under their tab tag, with an optional group level in
between:

```
Settings.Audio.Volume.Master            <- under Settings.Tab.Audio
Settings.Audio.Volume.Music
Settings.Audio.Volume.SFX
Settings.Audio.Headphones.Mode
Settings.Video.Display.Resolution
Settings.Video.Display.WindowMode
Settings.Video.Quality.Shadow
Settings.Video.Quality.AntiAliasing
```

The `Settings.<Contributor>` segment is the namespace boundary; the
rest is for grouping. Stick to one segment per logical group. Long
chains read poorly in the editor's tag picker.

For plugin-owned settings:

```
Settings.MyPlugin.SomeFeature.Toggle
Settings.MyPlugin.SomeFeature.Threshold
```

## Where tag tables live

If you own a plugin, ship a `Config/Tags/<PluginName>.ini` inside it.
UE auto-loads tag tables from any enabled plugin's `Config/Tags/`
directory; no `.uplugin` change is needed.

For the project, add to its own `Config/Tags/<anything>.ini`, or drop
the entries straight into `DefaultGameplayTags.ini` if you prefer the
legacy spot.

GameFeaturePlugins follow the same convention as plugins:
`Config/Tags/<GFP>.ini`.

## Don'ts

- Don't put a setting directly under `Settings`. Use a tab namespace.
- Don't reuse another contributor's tag namespace.
- Don't register the same tag twice in one registry. Collisions log a
  warning, but they're a smell. Pick distinct tags.
- Don't overload a tab tag (`Settings.Tab.Audio`) with non-setting
  metadata. Tabs are pages; settings belong under them.
