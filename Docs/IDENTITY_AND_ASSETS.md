# Identity and asset conventions

Every setting is contributed by a `UGameSettingsContribution`, and a
contribution's identity is its `FPrimaryAssetId`. That id is what the
registry stores, what `FindSettingById` takes, and what one setting uses to
reference another (a row naming its parent container, an edit condition
naming its target). Names have to be unique within a type and parent
references have to match, or you get duplicate ids and rows that never find
their tab.

This replaces the old gameplay-tag scheme. Settings used to be keyed by
`FGameplayTag` (`Settings.Tab.Audio` and the like). That's gone. Tags now
survive in only two places, both covered at the end of this doc.

## How the id is built

`UGameSettingsContribution::GetPrimaryAssetId()` returns:

```
FPrimaryAssetId(GetContributionPrimaryAssetType(), GetFName())
```

So the id is `<Type>:<AssetName>`. A `Game Setting Toggle` asset named
`Toggle_ShowDamageNumbers` has the id
`GameSettingsToggle:Toggle_ShowDamageNumbers`. The type comes from the
contribution class; the name is the asset's name in the Content Browser.

Two consequences:

- The asset name is the identity. Rename the asset and the id changes, and
  nothing fixes up other assets that reference the old id — lookups are raw
  `FPrimaryAssetId` equality, with no redirect consultation. Renaming a tab
  or section orphans every row whose `ParentContainer` points at the old
  name: those rows sit in the deferred-placement queue and log a warning.
  Renaming a container means updating its referencing rows in the same
  change.
- The CDO has no id. `GetPrimaryAssetId` returns an invalid id for the
  class default object and archetypes, so a contribution only counts once
  it's a real saved asset.

## The typed contribution classes

Each typed contribution overrides `GetContributionPrimaryAssetType` to
return its own type, so the asset manager (and the `AllowedTypes` filter on
`ParentContainer` pickers) can tell them apart without loading the asset.

| Class | Menu name | Primary asset type |
|---|---|---|
| `UGameSettingsContribution_Tab` | Game Setting Tab | `GameSettingsTab` |
| `UGameSettingsContribution_Section` | Game Setting Section | `GameSettingsSection` |
| `UGameSettingsContribution_Toggle` | Game Setting Toggle | `GameSettingsToggle` |
| `UGameSettingsContribution_Scalar` | Game Setting Scalar | `GameSettingsScalar` |
| `UGameSettingsContribution_Discrete` | Game Setting Discrete | `GameSettingsDiscrete` |
| `UGameSettingsContribution_Action` | Game Setting Action | `GameSettingsAction` |

The base `UGameSettingsContribution` returns the catch-all
`GameSettingsContribution` type. A hand-written subclass that doesn't
override `GetContributionPrimaryAssetType` still gets discovered by the
class scan; it just won't show up under a typed `AllowedTypes` picker.

## Parenting: tabs, sections, rows

The tree has three levels of container behavior:

- A Tab sits at the top level. It has no parent. It's a page in the screen.
- A Section nests inside a Tab (or another Section). It groups rows under a
  heading.
- A row (Toggle / Scalar / Discrete / Action) nests inside a Tab or a
  Section.

Sections and rows carry a `ParentContainer` field, an `FPrimaryAssetId`
whose picker is filtered to `GameSettingsTab,GameSettingsSection`. Set it to
the tab or section you want this contribution to live under. Leave it unset
and the contribution lands at the top level.

Arrival order doesn't matter. If a row is discovered before its parent tab,
the registry parks it in a deferred-placement queue and reparents it the
moment the tab arrives. A row in one GameFeaturePlugin can name a tab
shipped by another; it wires up when that GFP activates.

> The field used to be called `ParentTab`. A CoreRedirect maps the old name,
> so assets authored before the Section rework keep working.

## Naming convention

Nothing in the code enforces a naming scheme, but a consistent one keeps the
Content Browser readable and the ids self-describing. The convention used in
this project:

```
Tab_Audio                 a Game Setting Tab
Section_Audio_Volume      a Game Setting Section under Tab_Audio
Toggle_Audio_Subtitles    a Game Setting Toggle
Scalar_Audio_Master       a Game Setting Scalar
Discrete_Video_WindowMode a Game Setting Discrete
Action_Video_CalibrateHDR a Game Setting Action
```

Prefix with the type, then the area, then the specific name. Because the id
includes the type already, the prefix is really for humans scanning a
folder. Keep names unique within a type: two `GameSettingsToggle` assets
both named `Subtitles` collide on id, and the registry logs a warning and
keeps both rather than asserting.

## Discovery needs no Asset Manager rules

The discovery subsystem scans the asset registry by class
(`UGameSettingsContribution`), not by registered primary asset type. You do
not need a `PrimaryAssetTypesToScan` entry in `DefaultGame.ini` for a
contribution to register. Drop the asset under any mounted content path and
it's found.

## Where tags still live

Two narrow uses survived the move to `FPrimaryAssetId`:

- `Game Setting Action` carries a `NamedAction` `FGameplayTag`. Clicking the
  button broadcasts that tag through the registry's
  `OnSettingNamedActionEvent`; the settings screen routes by it. This is a
  command channel, not an identity.
- The `Platform Trait` edit condition gates on a `Platform.Trait.*` tag from
  CommonUI's platform-trait set. See `EDIT_CONDITIONS.md`.

Everything else that used to be a tag is now an `FPrimaryAssetId`.
