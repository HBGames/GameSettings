# GameSettings Audit Fix Plan

Source: full plugin audit, 2026-06-09 (four-area sweep: core model, contribution/discovery,
MVVM layer, editor/GFP/tests/build). Mark items `[x]` as they complete. Each item lists the
finding, the fix shape, and how to verify. Severity tags: **CRIT / HIGH / MED / LOW / NIT**.

Conventions for this work: tests first where a test would have caught the bug
(Phase 0), surgical diffs, no drive-by cleanup beyond what an item names.

---

## Phase 0 — Regression tests for the areas we're about to touch

These exist so the Phase 1–4 refactors can't silently regress. All go in
`Source/GameSettingsTests/` following the existing helper/namespace patterns.

- [x] **Change tracker tests** — dirty tracking, ApplyChanges, RestoreToInitial,
      re-entrancy (a setting whose Apply mutates another setting), stale weak
      entries for settings removed mid-transaction. *(CL 3123)*
- [x] **Registry re-add tests** — same handle back, no double delegates, no
      re-placement, original handle still removes. *(CL 3123)*
- [x] **Subsystem contribution tests** — bEnabled, liveness-aware dedup, re-apply
      after removal, OnRegeneratedEvent ordering + dedup clearing. *(CL 3123 — real
      subsystem instance outered to a test ULocalPlayer)*
- [x] **GameFeatureAction tests** — AddViewBindings has full per-context coverage;
      RegisterGameSettings covered for per-context bookkeeping + non-matching-context
      isolation (its apply path needs an initialized LocalPlayer subsystem collection,
      unreachable in automation). *(CL 3123)*
- [x] **View-bindings override stack tests** — priority ordering, tie-break,
      remove-by-handle, dead-weak-entry filtering under forced GC, broadcast counts. *(CL 3123)*
- [x] **Discrete filtered-index contract tests** — added beyond plan; locks the P5.6
      boundary mapping incl. disabled-current-value and disabled-default fallbacks. *(CL 3123)*

---

## Phase 1 — Crashes and memory corruption

- [x] **P1.1 [CRIT] ScalabilityQuality crashes on registration as documented** *(CL 3115 — own Startup/OnInitialized; persists via FGameSettingDataSourceFromGameUserSettings so SaveChanges de-dups into the standard GameUserSettings flush; P5.9 clamp folded in)*
      `GameSettingValueDiscreteDynamic_ScalabilityQuality.h:22-26`, base Startup at
      `GameSettingValueDiscreteDynamic.cpp:108-113`.
      Used as a contribution SettingClass, `Getter`/`Setter` are null and the inherited
      `Startup()` hits `check(Getter)` (dev) / null deref (Shipping); `OnInitialized`
      derefs again; null `Setter` also means `GetPersistableDataSource()` returns null
      so the setting never persists.
      Fix: override `Startup()` (skip data sources, call `StartupComplete()`),
      `OnInitialized()`, and `GetPersistableDataSource()` (GameUserSettings-keyed).
      Verify: test registering a ScalabilityQuality contribution on a registry; persistence
      path returns a non-null source.

- [x] **P1.2 [HIGH] Re-add corrupts handles + double-binds delegates** *(CL 3115 — re-add warns, no-ops, returns the EXISTING handle)*
      `GameSettingRegistry.cpp:281-286` (handle assigned before duplicate check),
      `:447-457` vs `:480` (delegates bound before the Contains early-out),
      placement continues after the bail (`:288-307`).
      Fix: check `RegisteredSettings.Contains` and bail in `AddSetting`/`AddCollection`
      *before* generating a handle; in `WireSettingTree` move the guard above the
      delegate binds. Make the "Idempotent on re-add" comment true.
      Verify: Phase 0 re-add tests go green.

- [x] **P1.3 [HIGH] MVVM FilterState is GC-invisible → use-after-free** *(CL 3116 — both members now UPROPERTY(Transient); struct reflection verified)*
      `GameSettingsScreenViewModel.h:138-140` — `FilterState` and `FilterNavigationStack`
      are non-UPROPERTY members, so their inner `TObjectPtr<UGameSetting>` arrays neither
      root nor get nulled; registry removal `MarkAsGarbage()`s settings; next
      `RebuildVisibleSettings()` derefs dangling roots (`GameSettingRegistry.cpp:182`).
      Fix: make both members `UPROPERTY()` (struct is already a USTRUCT) — settings then
      null out safely; `GetSettingsForFilter` already tolerates null roots once they null.
      Verify: PIE-style test or automation: register, open-screen VM, remove the tab's
      settings via handle, force GC, rebuild — no crash, settings gone from the list.

- [x] **P1.4 [HIGH] AutoContributors array dangles on module unload + wrong GC comment** *(CL 3119)*
      `GameSettingsModule.h:70-71` (`TObjectPtr` in a plain class, comment claims it
      prevents GC — it doesn't), `GameSettingsModule.cpp:112-118` (`ModuleLoaded` only).
      Fix: switch to `TWeakObjectPtr` (matching `KnownContributorClasses` below it),
      filter dead entries at iteration sites (`GameSettingsSubsystem.cpp:163-166`),
      and handle `ModuleUnloaded` in `OnModulesChanged` to prune. Fix the comment.
      Verify: code review + existing tests still pass; manual hot-reload sanity check
      in editor (user-run).

- [x] **P1.5 [MED] FSaveGameCache: by-value GC iteration + roots entries forever** *(CL 3119 — by-ref ARO iteration; stale-player eviction runs in FindOrLoad, not during GC)*
      `GameSettingsSaveGameCache.h:50-57, 68-74`.
      `AddReferencedObjects` iterates pairs by value (collector fixups written to a
      temporary → possible dangling pointer returned later); no eviction for dead
      LocalPlayers → permanent leak per PIE session.
      Fix: iterate `Entries` by reference; prune entries whose weak LocalPlayer key is
      stale (in `AddReferencedObjects` or `FindOrLoad`).
      Verify: code review; PIE start/stop twice and confirm no accumulation (user-run obj count).

---

## Phase 2 — MVVM staleness cluster (one pass over screen-VM structure handling)

Do these together; they share `HandleStructureChanged`.

- [x] **P2.1 [MED] VM eviction runs before GC nulls pointers — removes nothing at removal time** *(CL 3116 — evicts by registry membership via FindSettingById pointer-equality)*
      `GameSettingsScreenViewModel.cpp:403-415`. On the synchronous structure-change
      broadcast the removed setting is only garbage-flagged, so `GetSetting()` is still
      non-null and the `RemoveAll` sweep misses it; removed settings stay visible/editable.
      Fix: evict by registry membership (e.g. `Registry->FindSettingByHandle` /
      `RegisteredSettings` check via an accessor), not by pointer-nullness.
- [x] **P2.2 [MED] `CurrentTab` / `FocusedSetting` never revalidated** *(CL 3116)*
      `GameSettingsScreenViewModel.cpp:283-287`. `CurrentTab` is a strong ref to the VM
      so the `!CurrentTab` re-default never fires after removal; filter stays rooted on
      the dead tab; detail view shows stale fields.
      Fix: in `HandleStructureChanged`, if the current tab's setting is no longer
      registered, fall back to `Tabs[0]` (or clear); same check for `FocusedSetting`.
- [x] **P2.3 [MED] Discrete VM: `IsAtDefault` not broadcast when only `DefaultIndex` changes; `bWasAtDefault` computed against the new default** *(CL 3116)*
      `GameSettingDiscreteViewModel.cpp:77-96`. Capture both old values before updating
      either; broadcast `IsAtDefault` on either input changing.
- [x] **P2.4 [MED] Discrete VM: `SetSelectedIndex` doesn't validate/reconcile** *(CL 3116 — validates against Options, reconciles post-write; invalid input is a no-op by design)*
      `GameSettingDiscreteViewModel.cpp:10-34`. An out-of-range index (combo clearing
      to INDEX_NONE) is cached + broadcast, model refuses via `ensure`, VM desyncs.
      Fix: validate against `Options.Num()`, re-read `GetDiscreteOptionIndex()` after the
      write (Toggle and Scalar already reconcile).
- [x] **P2.5 [LOW] Toggle/Discrete VMs broadcast mid-setter; Scalar's guard pattern is the house style**
      `GameSettingToggleViewModel.cpp:9-31`, `GameSettingDiscreteViewModel.cpp:27`.
      Apply Scalar's set-cache-first + broadcast-only-on-change discipline to both. *(CL 3116)*
      Verify (whole phase): VM tests if practical; otherwise manual settings-screen pass
      with a GFP toggled while the screen is open (user-run).

---

## Phase 3 — Licensing / IP hygiene (needs an explicit decision)

- [ ] **P3.1 [HIGH] DECISION: MIT LICENSE on Epic-copyrighted Lyra code, public GitHub remote**
      `LICENSE` (MIT, "Copyright (c) 2026 Ronald Burns"), `.git/config` →
      `https://github.com/HBGames/GameSettings.git`, 33 files with Epic headers.
      Lyra source is UE-EULA; it can't be MIT-relicensed or distributed outside
      UE-licensee channels. Options: remove/replace LICENSE + make repo private to
      licensees, or strip Lyra-derived code. Also align copyright holder with
      Hitbox Games, LLC. **Owner: Ronald.**
- [x] **P3.2 [HIGH] Wrong company header on FunctionLibrary + convention violations** *(CL 3120)*
      `GameSettingsFunctionLibrary.h/.cpp:1` — `// Copyright Mob Entertainment.` must
      become the Hitbox header. Same pass: convert to `UCLASS(MinimalAPI)` + `UE_API`,
      add `UE_INLINE_GENERATED_CPP_BY_NAME`, drop `CoreMinimal.h`, fill the empty doc comment.
- [x] **P3.3 [LOW] Copyright header sweep on substantially-modified Lyra files** *(CL 3115 — Hitbox header + Lyra-attribution second line, matching the Responsive widget files)*
      Epic headers remain on heavily-modified files (`GameSetting.h/.cpp`,
      `GameSettingValueDiscreteDynamic.cpp`, `GameSettingFilterState.h`,
      `GameSettingCollection.h`); `GameSettingCollection.cpp` is flipped while its header
      isn't. Decide the rule (per CODE_STANDARDS: Epic header only on *unmodified* files)
      and apply consistently.
- [x] **P3.4 [LOW] .gitattributes / .gitignore are project-template stock** *(CL 3122 — uasset/umap binary attrs, plugin-scope gitignore, new plugin-root .p4ignore.txt belt-and-braces for .git/)*
      Add `*.uasset binary` / `*.umap binary` pre-emptively; prune plugin-meaningless
      project patterns. Consider a `.p4ignore.txt` inside the plugin root so `.git/`
      stays ignored in workspaces that lack the root config.

---

## Phase 4 — Contribution / discovery invalidation (one design, several symptoms)

Root cause shared by all of these: bookkeeping keyed by identity captured at apply
time, with no invalidation channel for rename / update / Regenerate / unload.
Design the channel once (likely: discovery subsystem subscribes to
`OnAssetRenamed`/`OnAssetUpdated`; subsystem exposes re-apply; registry broadcasts
post-Regenerate), then knock the symptoms out.

- [x] **P4.1 [HIGH] `Regenerate()` permanently empties the registry** *(CL 3115 — OnRegeneratedEvent; subsystem resets AppliedContributions and re-applies via ApplyAllKnownContributions)*
      `GameSettingRegistry.cpp:57-107`, subsystem dedup at
      `GameSettingsSubsystem.cpp:194-199, 227-232`.
      Fix: after Regenerate, clear `AppliedContributions` and re-apply all known
      contributions (module snapshot + discovery), matching what
      `CONTRIBUTING_SETTINGS.md:262` promises.
- [x] **P4.2 [HIGH] RegisterGameSettings GFA: no late-world/player handling, no per-context state** *(CL 3117 — FPerContextData map per Lyra AddInputContextMapping; OnStartGameInstance + OnLocalPlayerAddedEvent for late arrivals; no ModularGameplay needed)*
      `GameFeatureAction_RegisterGameSettings.cpp:23-74, 96`, `.h:59`.
      Fix: adopt the engine `FPerContextData` GFA pattern (see
      `UGameFeatureAction_AddInputContextMapping`): per-context handle storage,
      `OnGameFeatureDeactivating(Context)` removes only that context's handles,
      hooks for game instances/players created after activation.
- [x] **P4.3 [HIGH] AddViewBindings GFA: loaded asset never strongly referenced** *(CL 3117 — UPROPERTY(Transient) strong ref for the active span; per-context FGuid map fixes the re-activation leak)*
      `GameFeatureAction_AddViewBindings.cpp:19,28`, `.h:31,45`,
      `GameSettingsModule.h:81`.
      Fix: hold a hard `UPROPERTY(Transient) TObjectPtr` on the action for the active
      span (cleared on deactivate); per-context handle storage while in there (the
      single `FGuid` leaks the first override on re-activation).
- [x] **P4.4 [MED] GFP action + auto-discovery double-register the same asset** *(CL 3115 — ApplyContribution dedups + honors bEnabled; dedup is liveness-aware so GFA handle-removal doesn't block reactivation)*
      `GameSettingsSubsystem.cpp:140-154` (`ApplyContribution` has no dedup, no
      `bEnabled` check) vs `ApplyAssetContribution`'s guard.
      Fix: route the GFA through (or share) the `AppliedContributions` dedup; honor
      `bEnabled`. Update `CONTRIBUTING_SETTINGS.md:129-131` guidance.
- [x] **P4.5 [MED] Discovery misses rename + update events** *(CL 3121 — OnAssetRenamed re-keys; OnAssetUpdated does remove→re-add, which also makes bEnabled flips live)*
      `GameSettingsAssetDiscoverySubsystem.cpp:34-35`.
      Fix: subscribe `OnAssetRenamed` (re-key `ContributionsByPath`) and
      `OnAssetUpdated` (re-apply: remove old settings, apply new state — also covers
      `bEnabled` flips). Fix the header/README "covers editor saves" claim if not implementing.
- [x] **P4.6 [MED] Silent failure on stale bindings in cooked builds** *(CL 3121 — every Apply() bail logs Error with asset path + concrete failure; Binding.h comment corrected)*
      All typed `Apply()` early-returns (`_Toggle.cpp:25-28`, `_Scalar.cpp:44-47`,
      `_Discrete.cpp:34-44`, `_Action.cpp:25-28`, `_Tab.cpp:25-28`, `_Section.cpp:25-28`).
      Fix: `UE_LOG(LogGameSettings, Error, ...)` with asset + binding details on every
      unresolved-binding bail. Also correct `GameSettingsBinding.h:32-33` (no
      reflection-aware rename keeps serialized FNames live).
- [x] **P4.7 [MED] Doc/code mismatch: "rename redirects keep references live" is not implemented** *(CL 3121 — decision: docs-side fix; RowContribution.h comment now states renames orphan referencing rows. IDENTITY_AND_ASSETS.md half lands in the docs CL)*
      `IDENTITY_AND_ASSETS.md:30-32`, `GameSettingsRowContribution.h:34-36`,
      lookups at `GameSettingRegistry.cpp:196-212, 364-383`.
      Decide: implement (consult `UAssetManager::GetRedirectedPrimaryAssetId` in
      FindSettingById/parent resolution) or delete the claim from docs + header.
- [x] **P4.8 [MED] Discovery fallback: per-asset `TObjectIterator<UClass>` walk; misses unloaded BP subclasses; runs on dedicated servers** *(CL 3121 — GetDerivedClassNames with positive/negative caches; unconditional RemoveByPath; ShouldCreateSubsystem gates on IsDedicatedServerInstance)*
      `GameSettingsAssetDiscoverySubsystem.cpp:125-153`, no `ShouldCreateSubsystem`.
      Fix: use `IAssetRegistry::GetDerivedClassNames` for class matching; try
      `RemoveByPath` unconditionally on removal; gate subsystem creation off for
      dedicated servers (Arcadia is DS-only — wasted IO per instance).

---

## Phase 5 — Core-model robustness

- [x] **P5.1 [MED] `ApplyChanges` re-entrancy guard** *(CL 3115 — bApplyingSettings flag mirroring restore)*
      `GameSettingRegistryChangeTracker.cpp:55-67`. Mirror the `bRestoringSettings`
      guard (or iterate a copied array) so a side-effecting `Apply()` can't mutate
      `DirtySettings` mid-iteration. Phase 0 test covers it.
- [x] **P5.2 [MED] Spec-installed edit conditions never get `Initialize(LocalPlayer)`** *(CL 3115)*
      `GameSetting.cpp:200-205`, install path `GameSettingRegistry.cpp:633-670`.
      Fix: `AddEditCondition` calls `Initialize` when the setting is already initialized.
- [x] **P5.3 [LOW] Deferred edit-condition queue defeats idempotency** *(CL 3115 — dedup on enqueue + InstallSpec backstop)*
      `GameSettingRegistry.cpp:597-629, 713-716`. Dedup the deferred queue on enqueue,
      or re-check `bAlreadyApplied` in `FlushDeferredEditConditions`.
- [x] **P5.4 [LOW] Mutual parent references → cycle → stack overflow on traversal** *(CL 3115 — ancestor walk, warn + drop placement)*
      `GameSettingRegistry.cpp:395-414`. Add an ancestor check in deferred placement;
      degrade to a warning.
- [x] **P5.5 [LOW] Multi-target condition cleanup leaves stale dependency subscriptions** *(CL 3115 — RemoveEditDependency with per-call delegate-handle bookkeeping)*
      `GameSettingRegistry.cpp:749-762`, `GameSetting.cpp:213-220`. Add a
      `RemoveEditDependency` (or document the asymmetry); consider re-deferring rather
      than dropping conditions whose target may return (GFP reload).
- [x] **P5.6 [LOW] Discrete selected-index vs filtered options mismatch** *(CL 3115 — public index space is the filtered list, mapped at the Get/Set boundary; disabled current value falls back to default)*
      `GameSettingValueDiscreteDynamic.cpp:146-160` vs `:174-194`. `GetDiscreteOptions`
      filters disabled options but `GetDiscreteOptionIndex` indexes the unfiltered list.
      Matters now that `DisableDiscreteOption` is an authored feature. Align them.
- [x] **P5.7 [LOW] `ensureAlways(Getter)` followed by unguarded deref (×3)** *(CL 3115 — ensure+bail; bail paths call StartupComplete so initialization doesn't wedge)*
      `GameSettingValueBool.cpp:67-74`, `GameSettingValueDiscreteDynamic.cpp:99-102`,
      `GameSettingValueScalarDynamic.cpp:99-102` (whose `Startup` also lacks the
      `check(Getter)` the others have). Bail after the ensure.
- [x] **P5.8 [LOW] PlatformTrait Kill path: content mistake → runtime `check`** *(CL 3115 — spec validates first, logs Error + returns null; When* API checks untouched)*
      `GameSettingEditConditionSpec_PlatformTrait.cpp:15-29`,
      `WhenPlatformHasTrait.cpp:11-12, 37-38`. Return null + log instead of `check`ing
      on empty tag/reason (the deferred machinery already tolerates null).
- [x] **P5.9 [LOW] ScalabilityQuality index can exceed capped options (subclass path)**
      `GameSettingValueDiscreteDynamic_ScalabilityQuality.cpp:76-84` vs cap at `:26-41`.
      Clamp `GetDiscreteOptionIndex` to `Options.Num()-1`. *(CL 3115 — folded into P1.1)*

---

## Phase 6 — Editor module

- [x] **P6.1 [MED] Discrete edit-condition customization: refresh delegate registered after the early-out** *(CL 3118)*
      `GameSettingEditConditionDiscreteCustomization.cpp:36-53`. Register the
      TargetSetting-changed → `ForceRefreshDetails` hookup *before* the
      unresolved-options return so the fallback state upgrades live.
- [x] **P6.2 [MED] Binding customization: `WeakThis = AsShared()` is a strong capture** *(CL 3118 — genuinely weak now; options arrays are TSharedRef members co-owned by the combo widgets, so OptionsSource can't dangle)*
      `GameSettingsBindingCustomization.cpp:363-368`. Make it genuinely weak
      (`SharedThis(this).ToWeakPtr()` + `Pin()`), and handle the
      `GetterOptions`/`SetterOptions` lifetime deliberately (the strong capture is
      currently accidentally load-bearing for the combo's raw `.OptionsSource` pointer,
      `:316-320`).
- [x] **P6.3 [LOW] `IsCandidateGetter` dead empty `if` — flag filter never applies** *(CL 3118 — dead block deleted; runtime resolves via reflection so reflected-is-enough is the real contract)*
      `GameSettingsBindingCustomization.cpp:168-171`. Finish the filter or delete the block.
- [x] **P6.4 [LOW] Fuzzy match: acronym runs don't split** *(CL 3118)*
      `GameSettingsEditorFuzzyMatch.cpp:115, 176-189`. Split upper→lower-after-acronym
      (`"HDRBrightness"` → `HDR`,`Brightness`) so exact-token bonuses apply.
- [x] **P6.5 [NIT] Editor misc** — unused `BuildFunctionOptions` param (`:252`), unused
      locals (`GameSettingEditConditionDiscreteCustomization.cpp:48, 88` + stale comment),
      missing `NotifyCustomizationModuleChanged()` on shutdown, redundant `_` check in
      `IsBoundary` (`GameSettingsEditorFuzzyMatch.cpp:17`). *(CL 3118)*

---

## Phase 7 — Build / config / hygiene

- [x] **P7.1 [LOW] Declare `EnhancedInput` plugin dependency in the .uplugin** *(CL 3122)*
      `GameSettings.Build.cs:25` depends on the module; only works via CommonUI's
      transitive enable today.
- [x] **P7.2 [LOW] Remove unused `ModularGameplay` dep** *(CL 3122 — confirmed still unused after the GFA per-context rework, which uses engine-native hooks only)*
      `GameSettingsGameFeatures.Build.cs:26` + `GameSettings.uplugin:52-54`. Nothing
      references it.
- [x] **P7.3 [LOW] Delete `Config/Tags/GameSettings.ini`** *(CL 3122 — opened for delete)*
      Never loaded (regular plugins get no tag-ini scanning); its stated back-compat
      purpose isn't served.
- [x] **P7.4 [LOW] Responsive panel fixes** *(CL 3122)*
      `SGameResponsivePanel.cpp:43-60, 135-156`: `ClearChildren` must clear `InnerSlots`
      (dangling `FSlot*` cache); `RemoveSlot` should `RefreshLayout` (stale columns/fills).
      Plus `override` on `OnArrangeChildren` (`.h:74`); `PhysialScreenSize` typo (`.h:87`).
- [x] **P7.5 [LOW] ViewBindings: sync class load per generated row** *(CL 3116 — one-time pre-resolve into a transient cache, editor invalidation via PostEditChangeProperty)*
      `GameSettingsViewBindings.cpp:20` — `LoadSynchronous` inside entry generation is a
      scroll-hitch risk; pre-load or async-load entry classes.
- [x] **P7.6 [LOW] Detail extensions not BP-accessible despite BP-driven docs** *(CL 3116 — GatherDetailExtensions is BlueprintCallable)*
      `GameSettingsViewBindings.h:48-56` vs `MVVM_GUIDE.md:130-132`. Expose
      `GatherDetailExtensions` as `UFUNCTION(BlueprintCallable)` (or correct the guide).

---

## Phase 8 — Docs + standards sweep (single pass at the end)

- [x] **P8.1 Doc-rot sweep** *(code-side comments in CLs 3115/3116/3119/3121; README + Docs/*.md in CL 3124)* — fix in one pass once behavior settles:
      "TObjectIterator" claims (README:55, CONTRIBUTING:84, `GameSettingsContribution.h:26`,
      `GameSettingsAutoContributor.h:18` — code uses `GetDerivedClasses`); stale
      `RegisterSetting` reference (`GameSetting.h:56-59`); scalar VM header contradicting
      its refresh behavior (`GameSettingScalarViewModel.h:18-20` vs `.cpp:72-75` — decide
      constant-or-dynamic and add FieldNotify if dynamic); Regenerate/re-add claims in
      CONTRIBUTING_SETTINGS.md updated to match P1.2/P4.1; duplicate-ID warning text
      stating first-wins lookup consequence (`GameSettingRegistry.cpp:485-497`).
- [x] **P8.2 Standards sweep** — `TObjectPtr ... = nullptr` initializers
      (`GameSettingRegistry.h:40, 62, 65`); `UE_API` on private members
      (`GameSettingViewModel.h:71-72`, `GameSettingsScreenViewModel.h:113-130`,
      `GameSettingsView.h:91-125`, `GameSettingsViewModelSubsystem.h:39`); unused
      `UE_API` define/undef pairs (`GameSettingEntryBase.h`, `GameSettingsDetailView.h`,
      `GameSettingsAutoContributor.h`, `GameSettingsRowContribution.h`); namespace
      `AreFTextArraysEqual` (`GameSettingViewModelUtils.h:8`); braces on single-line if
      (`GameSettingsContribution_Scalar.cpp:71-72`); `UGameSettingEditConditionLibrary`
      → `UBlueprintFunctionLibrary` (`GameSettingEditConditionSpec_BlueprintBridge.h:70`).
      *(Spread across CLs 3115/3116/3119/3121 with their owning files)*
- [x] **P8.3 Screen VM tidy** — double rebuild in `Initialize`
      (`GameSettingsScreenViewModel.cpp:44-45`), redundant `ClearDirtyState` after
      `ApplyChanges` (`:124`), unconditional `CanPopNavigation` broadcast (`:87`).
      *(CL 3116 — note: the Initialize fix is a `!CurrentTab` guard, not deletion; the
      no-tabs path needs the explicit rebuild)*
- [x] **P8.4 Test-convention alignment** — module moved to `UncookedOnly` (CL 3122);
      shared `GameSettingsTestFlags` constant in the helpers header, all 8 existing
      test cpps switched to it; Public module-interface header added; deps moved to
      `PublicDependencyModuleNames` (CL 3123). Kept the existing `System.GameSettings.*`
      path prefix for in-module consistency (deliberate deviation, all-or-nothing rename
      wasn't worth the churn).

---

## Deferred / explicitly not doing now

- `Regenerate()` does not restore contributions pushed directly through
  `ApplyContribution` (the GFP-action path) — only auto-contributors and
  discovered assets re-apply, because the subsystem can't regenerate the
  caller's retained handles. Found by the CL 3123 tests; documented in
  `OnRegeneratedEvent`'s comment and CONTRIBUTING_SETTINGS.md. Revisit only
  if Regenerate gains real (non-test) callers.

- ~~Late-join LocalPlayers receiving GFP-action contributions~~ — no longer deferred:
  the P4.2 per-context rework covers late game instances and local players; README updated.
- `ShouldAutoContribute` re-evaluation on gate flip — header doesn't promise it.
- BlueprintBridge dangling-native-pointer exposure — documented callback-scoped risk;
  keep the warning prominent.
- Per-access property-path re-parse in the three non-Dynamic data sources — perf only,
  revisit if settings screens show up in profiles.
