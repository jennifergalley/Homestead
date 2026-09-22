# Design

## Context and governing policy

Jenny's September 20 22:35 instructions supersede the earlier planning hold:
implement isolated nonconflicting source lanes now, reconcile and deliver
environment first and UI second. Existing saves are disposable test data.
Prioritize working integrated increments, not historical migration/profile
preservation infrastructure. Time/credits/capture counts are not work-stop caps.
Actual errors, controller behavior, visuals and current-version persistence
still require proof.

The inspected starting source is `61b3b30` plus this plan's two documentation
commits. This isolated worktree is not the active environment checkout.
`docs\game-plan.md`, `PRODUCT.md` and the parent's relayed working policy govern
product intent; `DESIGN.md` describes the incumbent implementation. The first
Coral Island reference image was reviewed, the other two were unavailable under
the image quota. No private portrait was needed. Impeccable's context detector
misclassifies Windows Unreal as web; no CSS, browser implementation or platform
rewrite follows from that warning.

Observed source:

- Canvas HUD, seven pages, seven visible rows. Settings index 9 is Save and quit,
  below initial viewport. `SaveSlot` succeeds before `QuitGame` is called.
- `Back` opens Settings in ordinary play but retries in failed state; ToggleBook
  is disabled on failure. Recovery therefore needs an independent exit route.
- `HomesteadPromptIntent` is the sole accepted-event device classifier. Mouse
  noise and held sticks must not acquire a competing writer through widgets.
- Simulation has 14 fungible item kinds and 120-unit pack/chest limits, not a
  slot economy. Chest access is within 280 cm.
- `HomesteadAppearance` chooses 18 complete body/hair/outfit meshes.
  `export_heroine.py::prepare_body` removes covered torso/foot vertices before
  joining meshes; hiding clothing materials cannot implement safe unequipping.
- Portable save currently writes v3 (reads v2/v3); UE wrapper writes v4, with
  `HOMESAV1` CRC and portable checksum. No historical migration is required now.
- `warmOutfit` is independent simulation state; cosmetic apron has no insulation.

## Goals / Non-Goals

Deliver discoverable safe exit, fullscreen translucent icon-tab menus, truthful
grids/details with controller/keyboard/mouse parity, genuine owned clothing,
safe compatible rendering and reliable current-version saves. Preserve world
rewards, music, control meanings, pause, VSync and normal Lit save/load.

No CommonUI framework, web UI, new shops/buffs/armor/durability/winter balance,
face/hair redesign, copied reference assets, automatic schedules or historical
save-migration/profile-snapshot project.

For subsequent feature rounds, Jenny's 23:28 preference is focused free/reusable
prior-art research before planning custom components. This round reused native
Slate layout/focus/scroll controls, engine scene capture and existing simulation/
save authority; the 32 original icons were already authored without an external
icon-library comparison. Do not claim that comparison occurred or redo them just
to manufacture process compliance. Remaining runtime checks will extend the
existing smoke actor/screenshot route, not introduce another testing framework.

## Ownership and execution lanes

| Lane | Owned files / contract | Dependency |
| --- | --- | --- |
| UI (this worktree) | `Source\SurvivalGame\UI\*`, `HomesteadHUD.*`, Controller menu/input/exit integration, `SurvivalGame.Build.cs`, UI tests/icons | Can implement against existing item rows and transactions immediately |
| Wardrobe authority | `Simulation\*`, `HomesteadSave.*`, portable item/save tests; typed snapshots/transactions | Supplies real owned-item/equipment APIs; coordinates Controller save adapter with UI owner |
| Character/assets | `HomesteadCharacter.*`, garment exporters/assets and shared presentation adapter | Supplies compatible renderable garments and appearance/preview APIs |
| Environment/integration | World/environment and sole approved engine authoring/build/runtime lane | Reconciles source and runs shared pipeline; delivery remains environment then UI |

No engine, UBT/UAT/cook or unadmitted MSVC/helper processes from this worktree
while the environment lane owns execution. Source work, original icon generation
and non-engine checks are permitted. Request a coordinated build/runtime slot,
report source-only status honestly, and continue useful nonconflicting work.
Central plan is UI-owned; siblings use uniquely named lane addenda.

## Decisions

### Current integrated milestone: save/reset safety and real inventory transactions

The selected rollback entering this milestone is `woodland-polish-06` /
`woodland-polish-v9`. Preserve its generated woodland, regional water,
Timber/Firewood, controls, saves and performance while completing the current
native menu rather than introducing another UI framework.

Reconcile `improve-menu-directional-navigation` from exact delivered evidence.
Candidate06 proves controller/keyboard directional grids, short/full/scrolled
and empty content, quantity edit/cancel, focus restoration and separate-process
resume at 720p/4K. That satisfies this change's grid-navigation integration
task. It does not prove full mouse/device switching, every held-stick comfort
case or the separate change's complete upper-boundary matrix; keep the broader
stable-input task open and disclose those limits.

Use the current `Simulation` typed APIs and `HomesteadMenuInventory` adapter for
the visible transaction slice. Pack/chest/equipment cards and details remain
views over real layout/ownership. Move, split, merge, amount, equip, unequip and
recolor capture the current revision and commit once through authority.
Cancellation, stale revision and repeated confirm must leave state unchanged;
menu routing consumes activation before Slate buttons can click through.

Current-version save/reset work stays in the existing Controller/save wrapper:
validate complete candidate simulation, world identity, wardrobe presentation
and routing before replacing live state. Manual, autosave, recovery and session
snapshots replace rather than merge. Exercise temporary write, readback,
backup, final replacement and retry through sandbox-only failure injection.
Unsupported test versions show an explicit reset dialog that defaults to
cancel; reset creates a fresh test world without deleting unrelated profiles.

Acceptance uses synthetic portable fixtures first, then the serialized engine
slot. Run 720p and 4K producers plus separate consumers, explicit incompatible
reset, save-failure/retry, unsaved/recovery/Settings routes, ordinary gameplay,
and repeated menu open/close cleanup. Mouse is required only for already
implemented click/hover equivalents; no unsupported drag-and-drop or broad
device-parity claim is introduced.

### Native shell, not another UI framework

Use a narrow C++-authored Slate widget directly in the game viewport. The early
implementation uses native typed callbacks/layout without an unnecessary UMG
host. Add only Slate/SlateCore dependencies actually used; a later preview must
justify any additional module. Keep Canvas for gameplay/planning. One menu surface
owns drawing, focus and feedback; no duplicate old-book draw beneath the shell.
Controller remains action/save authority; widgets never mutate simulation arrays.

Canvas extension was considered, but pointer hit-testing, clipping, scrolling,
focus and modals would duplicate native widget infrastructure. CommonUI would
introduce another activation/input detector for only seven pages; not justified.
Use typed subjects/actions, not parsing row labels to invent state. Early
existing-item grids can consume the existing `Rows`/stock/action APIs. Wardrobe
integration uses sibling snapshots, not cosmetic outfit toggles labeled as items.

### Visual and interaction brief

Operate for Inventory/Settings, Read for Guidebook/Credits. Preserve warm
Pine/cream/Gold, with original distinguishable icons, readable names, active-tab
underline and backed high-contrast focus. Fullscreen translucent scrim shows the
paused world; content has stronger backing, no costly live blur.

```text
 [bag Inventory] [tools Craft] [house Build] [book Guidebook]
 [gear Settings] [credits] [Appearance]             Time paused
 -----------------------------------------------------------------
 Reserved feedback, complete error text
 -----------------------------------------------------------------
 Character /    | Your pack / Nearby chest | Icon, name, quantity
 equipment      | icon grid, real counts   | description / costs
 (when real     | selected cell highlight  | explicit valid actions
 APIs ready)    | scrolling, no slot cap   | hover OR focus details
 -----------------------------------------------------------------
 Current-device hints: tabs / regions / activate / back
```

Start from 1280x720 logical sizing scaled to 4K. Use 16px or larger body text
at 720p, >=44px action targets and strong backed contrast (4.5:1 normal text,
3:1 focus/large text). Scroll details instead of clipping/shrinking. Validate
long labels and real messages. Empty inventory has honest guidance; disabled
actions explain why. No invented value/rarity/durability/stat fields.
Settings uses appropriate labeled control sections, not fake item cells.
Craft/Build display output icons and Needs, not ownership. Keep Guidebook and
Credits readable with existing content/attribution.

Settings pins Resume and Save and quit outside scrolled settings, with initial
focus Resume; Right then Activate reaches exit in three presses from gameplay
(Escape/B -> Right -> Activate). Other pages expose the Settings tab. Restart
is a distinct cancel-default confirmation.

Inventory defaults to six columns when space permits, stable logical ordering,
count-based capacity. Nearby chest is identified by stable ID and revalidated
on action. No remote storage access. Appearance remains body/hair/skin/eye
customization. Real equipment/preview joins Inventory when sibling APIs exist.

### Input and lifecycle

Only menu-owned input changes: D-pad/arrows navigate four ways, LB/RB or
Ctrl+Tab/Ctrl+Shift+Tab change tabs, LT/RT or Tab/Shift+Tab change regions.
A/Enter/E activate; B/Escape cancels innermost dialog then closes; I/Menu closes
with no nested dialog. Mouse hover previews details, click selects/activates;
wheel scrolls menus rather than zooming gameplay camera. Focused preview alone
can consume orbit input. No hover-only capability; no drag/drop initially.

Grid edges clamp, short final rows select nearest valid column, selected cells
scroll into view. Preserve focus by subject ID; after removal select nearest
survivor or empty guidance. Device switch clears transient pointer details.
Disabled/empty controls can explain themselves but cannot mutate state.
Explicit Move/Split dialogs show source/destination/amount with One/All,
bounded increments, Confirm and Cancel. Nonstacking garments cannot split.

Route accepted events exactly once through existing `FHomesteadPromptIntent`.
Focused native widgets must not swallow intent or bypass test input isolation;
no second CommonInput writer or duplicate native/controller navigation.
Keep normal world mappings and F5/F9 semantics unchanged. Menu pause uses
simulation pause, not engine pause; explicit existing craft/eat actions retain
their resource/time costs. Opening menus cancels placement without spending,
stops movement and cancels animation without repeating rewards. Closing modals
does not unpause their parent. Load/recovery clears transaction drafts.
Feedback has reserved space and cannot cover tabs/actions/details.

### Exit state machine

Settings -> Confirm (Stay default) -> Saving -> verified save -> Desktop.
Failure -> persistent Retry save and quit / Return to Settings / Quit without
saving. Unsaved exit has its own cancel-default warning. No timeout/held input
discards progress. While Saving reject duplicate actions and mutation shortcuts.
Use current protected SaveSlot path; capture its actual failure rather than
inferring success from a toast. State last successful save time only when known.

Recovery retains Retry and independently exposes Settings/Quit with Y/G and
mouse. Never save failed state over usable checkpoint: explain disabled save,
allow explicit Quit to desktop. Back returns to recovery, not another retry.
Invalid startup routing still fails before unsafe access. Graphics-config save
failures remain distinct from world-save failures with an explicit unsaved
preference decision. Resetting incompatible disposable test data must be labeled,
never silently described as restored progress.

### Real ownership and current-version saves

Wardrobe authority owns stable definitions/instances and atomic transactions.
Proposed initial definitions: linen tunic (Torso+Legs), separate apron requiring
a tunic, existing shoes/socks (Feet), original woven footwraps (Feet).
Tunic/apron/footwrap craft costs 12/6/8 Fiber plus existing knife requirement
remain review defaults. Existing leather shoes are starter items, not falsely
craftable from fiber. No free item spawning through appearance UI.

Each instance has one owner (pack, identified chest, equipped), stable ID and
per-instance dye. Equipment references are not copies. One tunic may occupy two
declared slots without duplicate ownership. Validate final capacity before
atomic swaps; full-pack one-for-one replacement works, removing tunic+apron
requires two pack units. Pack/chest remain 120 total units; equipped/base layers
are not carried capacity. Stable ordered groups partition fungible counts,
splits add no capacity cost. Cancel/stale/failed transactions change nothing.

Moss/Wine/Slate/Flax remains a free cosmetic per-owned-garment recolor, not a new
dye economy. Preserve current survival state; don't connect appearance to
`warmOutfit` or advertise warmth.

Backend selects compatible current schema versions and validates current
quantities/IDs/owners/layout/equipment/dye on load. Keep integrity/bounds,
temporary validation, backups and truthful failures. Current-version roundtrip,
separate-process reload, invalid ownership, write failure/retry and same-world
recovery are required. Older test data may explicitly reset; no deterministic
legacy grant, golden historical corpus, snapshot/rollback or downgrade project.
Controller startup/load/reset adapter is coordinated with backend owner.

### Garment and icon presentation

Character lane must reconstruct a permanent modest base consisting of separate
bra and briefs visual pieces, per Jenny's direct feedback, with complete geometry
from licensed source, not hide materials on the joined/deleted body. Preserve
adult phenotype, face/hair direction, rig, skin/eye contracts and movement.
The bra and briefs are nontradeable and nonremovable, not equipment slots or
inventory items. Separate fitted tunic/apron/footwear components follow current pose with
no extra world collision/physics. Validate base-only and all supported fits
through idle/walk/gather/water/weed/clear. Resolve renderables before equip commit;
missing content is a visible error, not silent item loss/substitution.

Inventory preview shares committed equipment presentation, never creates
another inventory. Prefer one cached/on-demand preview with orbit only when
focused. No live 3D capture per item. No fake paperdoll presented as complete.

UI lane creates original local icons or admitted in-engine renders for all
existing items/recipes/plans/tabs plus garment definitions when available.
Document authorship and avoid copied Coral Island art/fonts. Native vector
line/filled illustrations are acceptable original icons without external assets;
they must remain recognizable at cell size and have names/tooltips.

## Dependency milestones and validation

| Milestone | Prerequisites | Proof |
| --- | --- | --- |
| Early Settings/exit | Current parallel-source authorization, existing protected save API, native shell/input | Reachable exit/recovery, persistent failures, pause, device navigation |
| Existing-item grids/details | Shell/input, existing item actions/counts and original icons | Truthful stock, hover/focus parity, 720p/4K readability, no reward changes |
| Functional wardrobe | Real ownership/current save APIs plus compatible assets | Equip/craft/store/dye/split conservation and actual character presentation |
| Integrated delivery | Reconcile after accepted environment; shared authorized pipeline | Compile, basic gameplay/controller/visual/current-save checks then delivery |

Partial tasks stay unchecked until all their behavior is verified. Central
plan progress distinguishes source implementation from compiled/runtime proof.
Do not block useful source work on engine access and do not claim it passed.

Use existing targeted native tests where executable policy permits and focused
menu/exit tests. Shared integration runs prompt, book clarity, feedback, VSync,
hotkey/save-routing and affected action/full-loop cases, preserving semantic
assertions rather than obsolete row counts. No broad tooling rewrite.

Review matched baseline/candidate menu states at 1280x720 and 3840x2160, then
batch demonstrated corrections and review again. Use purposeful samples of
equipment/base, details, storage, quantity, Craft/Build/Guide, Settings/exit
and actual error states. Capture normal movement/actions for garment quality.
No arbitrary two-pass/40-frame/time/cost ceiling stops agreed work. Don't
manufacture endless polish or call technical tests human aesthetic approval.
Record actual viewport/render scale and Lit/LightingOn/ShaderComplexityOff;
retain the fixed F5/F9 behavior rather than forcing Lit to conceal regressions.

Compare clean frame timings with menu closed/open/orbiting, check repeated
open/close doesn't retain widgets/render targets, and retain the 60 FPS product
goal. Diagnose real regressions; invented fixed 2ms/256MiB budgets are not
unreviewed release gates. Existing VSync/settings semantics remain.

## Risks / Trade-offs

- Slate input consumption -> single controller-owned accepted-event policy.
- Placeholder equipment -> no equipment actions until typed real authority.
- Deleted body geometry -> source-level modest base, not material hiding.
- Explicit test reset -> clear incompatibility message and conscious reset;
  current compatible save failures still preserve live state for retry.
- Parallel conflicts -> owned file boundaries, shared API contract and one
  coordinator for integration and engine execution.
- Incomplete runtime access -> publish source checks honestly and request shared
  compile/runtime slot; no silent declaration of playable success.

## Integration plan

Checkpoint coherent source in each isolated private branch, coordinate APIs,
reconcile against accepted environment, run shared approved compile/runtime
checks and fix actual failures. Prefer a latest integrated playable candidate
over elaborate preservation infrastructure. Do not auto-merge, open PRs or
start schedules. Parent owns delivery and journal.

## Main integration: canonical content and current saves

The coordinator has delivered the fern clearing and rough mid-back-wave playable
increment (`4ed4050` selection). Main now integrates frozen UI `a85e026` with
the canonical character handoff `816d3dd`, preserving the selected executable,
wave bindings, ordinary-play mesh/material telemetry and existing guard policy.
The merge retains NativeMenu/NativeMenuQuit and CaptureSessionCheckpoint.
Twenty menu source checks and six PowerShell parse checks pass; this is not
native compilation or runtime acceptance.

Reuse the proven explicit native skeletal/texture factories, original skeleton
and incumbent material interfaces. Import exactly 21 modular meshes: the final
six LongWave/Bob bases, original three Ponytail bases and twelve garments.
Import the final six joined Bob meshes too, without replacing incumbent Bob
packages. Final-bundle mappings select canonical files only; BobReuse and
PublishedWaveParity directories are references, not import inputs. The existing
six wave trial packages remain unchanged.

Use the fresh `/Game/SurvivalGame/Characters/ModularClothing` root: existing
contract paths for the 21 fits, `JoinedBob` for six joined meshes, `Textures`
for the neutral Bob atlas and modular weave, and `Materials` for one neutral
Bob material plus the four permanent-base/footwrap materials. Exactly 34 new
packages are expected. Reuse admitted neutral wave material and original
skin/eyes/face/ponytail/garment materials. Preserve four cloth recipes' colors,
roughness and eight-times weave UV tiling; only hair and garment-linen roles
receive the appropriate runtime tint. No source reauthoring, automatic material
import, new skeleton/animation/physics asset, generic SaveAll or accepted asset
overwrite.

Check exact source triangles, slot roles/interfaces, original 54-bone reference
bind, finite centimetre bounds (base/joined height155-175; garments positive and
at most140), object inventory and persistent references before explicit saving.
Retain partial files and logs on failure. A separate fresh process must reload
and check the saved inventory before cooking. Import/verification reuse the
existing completion-driven guard primitive with live stop/pause and real native
cancellation boundaries, not an arbitrary short import timer.

The source owner corrected ignored `.blend1` receipt prerequisites in `e72676f`;
canonical media are unchanged. Main integrated the omitted historical wave
qualification and exact committed LF archive bytes. Full final canonical and
original modular source checks now pass without mutable backups or weakened
geometry/hash checks. Thirty new policy negatives and36 existing fern/cook
negatives pass. Current integrated native compiles/libraries/DLLs and genuine
UBT metadata pass; `wardrobe-native-build-01/receipt.json` records exact products
and preserves prior build-monitoring qualifications. Import/runtime acceptance
still requires the guarded operation and subsequent real-game evidence.

The first actual wardrobe import failed cooperatively before any package save:
the initial importer incorrectly assumed joined-apron material order was the
tunic order plus two appended slots. The pinned Bob source instead places apron
trim at0 and brass at12. The corrected importer binds by each actual imported
unique role/name, independently checks all27 source role sets, records source
indices separately, and validates section membership/triangle coverage. Only
the source's explicit joined-Bob hair-slot7 contract remains positional.
An offline reordered-slot positive case and38 negatives pass; genuine corrected
native products are recorded in `wardrobe-native-build-02/receipt.json`.
Distinct import02 may reuse only the original failed attempt's unchanged,
identified, completely empty ordinary namespace directory; no deletion,
quarantine, reservation reset or accepted-content overwrite is admitted.
Import01 remains failed with its original result and cleanup proof preserved.

Actual import02 passed all34 packages/27 meshes and cooperative clean shutdown;
the released marker and protected incumbent/source files were unchanged. Fresh
process reload and real-game acceptance remain separate pending gates.
After release, the coordinator-directed cache optimization replaced repeated
mutable shader-byte hashing with ordinary-tree identity/count/size observations,
and prunes that cache from old-run traversal. Unrelated historical files receive
explicitly labeled metadata comparison, not claimed byte-identity proof. Named
historical admission receipts and actual source/tool/product/Content invariants
retain their hash checks. A locked disposable cache test confirms no byte reads;
38 fern/cook/cache negatives and38 wardrobe negatives pass.

The strict UI fixture commits `26b9081`/`d8f298f` are integrated as
`9a1f1b5`/`a695fa7`. Actual game-module compilation/link/metadata passed again,
as did23 source checks and47 wardrobe/resume preflight negatives. The fresh
`wardrobe-ui-01` candidate uses the genuine exported two-compile Shipping plan,
separate guarded resource/link/manifest leaves, existing metadata verification
and loose staging. The distinct `wardrobe-cook-01` operation requires all34
persisted wardrobe packages plus the retained eight wave packages, with the
same source/content/privacy/marker/child/endpoint controls and inner/outer
SkipZenStore. Forty cook/cache/coverage negatives pass.

Actual guarded cook and fresh loose staging passed. The unpromoted
`wardrobe-ui-01` Shipping executable is
`B080BAFD69087C0B12F4111DF05BFE6628249576CFDF99AE725F8583FAA48F2F`.
Both 1280x720 and 3840x2160 producers passed all43 native menu steps, including
real modular equip/unequip/dye, five appearance controls, F5 and changed-state
F9 restoration. Separate-process consumers passed at both resolutions with
exact ownership/equipment/look/render identity and unchanged producer bytes.
Measured producer means were59.96/60.04 FPS with p95 frame times16.88/16.87ms;
these exclude startup and screenshot readback and are not human controller
comfort or general gameplay performance proof.

Actual inventory/restored images reveal an unusably small bright portrait,
clipped Turn controls, low-contrast selected labels and overflowing details
actions. Functional assertions do not establish visual usability. A bounded
UI-owned layout/portrait correction is in progress; no global lighting or
accepted geometry changes are part of it. The separate `wardrobe-quit-01`
attempt failed at reaching confirmation despite successful guard cleanup and
process exit0. Preserve that failure. Its repeated Escape chain is being
replaced with the producer's proven, separately asserted mapped Back / closed
guide / Escape / Settings entry sequence; successful save and genuine exit
still require a rebuilt runtime test.

Ordinary mapped walking, turning, stopping, orbit and actual berry gathering
recorded306 frames over43.7133 seconds in `wardrobe-walk-01`, without teleport,
time or simulation edits. Eight-Hz screenshot sampling is not an FPS test or
continuous collision/garment-coverage proof. The inspected world view shows
the prepared tunic/shoes and retained rough waves; all presets/combinations,
preview usability and final integrated acceptance remain separate gates.

The next correction batch is limited to the demonstrated native shell/portrait
defects and the shared quit-fixture entry sequence. Build a distinct
`wardrobe-ui-02` Shipping executable through the existing exact local leaves,
then stage against unchanged, verified `wardrobe-cook-01` data. Do not recook
unchanged content, overwrite candidate01, or turn obsolete free-outfit/Canvas
test assumptions into a universal first-delivery gate. Preserve gameplay and
inventory-conservation coverage when adapting affected fixtures.

Normal menu/equipment selection is unconditional production behavior, not a
NativeMenuTest enablement override; the ordinary route already records the
actual modular path without that flag. Small observations in the existing
StartupProbe distinguish native-menu creation and post-F9 modular rendering
from QA feature flags. They are instrumentation, not a separate successful
normal-launch run; the historical unguarded/off-schema startup wrapper is not
admitted as a shortcut. Review fresh720p/4K images, real quit and current saves
before promotion, preserving the observed claim limits.

That batch is now implemented and exercised: UI source5d238f5 integrated as
0067821; real Shipping executable SHA256
`241437C63D63EB905787C10F6F85C3FA237AE8379F7F8319C390F88D97D2D3FD`.
Both720p/4K43-step producers and distinct-process consumers passed. The corrected
quit fixture passed its separately observed transitions, actual saved state and
engine exit request, followed by clean supervised process death. Candidate01's
failed quit and rejected-layout evidence remain unchanged. A fresh ordinary
route captured302 frames over43.0953s, including actual berry gathering.
Firsthand720p inspection confirms substantially enlarged, non-blown-out portrait,
compact Knife card, readable Carried selection, actual details and contained
scrolling controls; lower-body/shoe lighting remains dark. This is a usable-layout
milestone, not all-presets visual approval or completed OpenSpec acceptance.
`docs/research/character-assets/wardrobe-shipping-02/receipt.json` seals the exact
build, source pins, actual images, state/save/quit and ordinary-route evidence.

Coordinator firsthand4K inventory/restored review accepted this usable prototype
increment with the stated lighting/density qualifications. `Preview.json` now
selects `wardrobe-ui-02` with fresh `jenny-review-v5` for schema5/portable4.
Existing `jenny-review` and original saves remain untouched; no migration,
silent corruption reset or human-process launch occurred. Prior wave-candidate
graphics preferences were copied byte-for-byte, and its selection/package are
retained for cheap rollback. This first delivery does not close all44 tasks.

Deliver UI and content together. Extend existing smoke fixtures for actual
equip/unequip/dye and current-version save/load/resume with owned IDs, material
presentation and quantities preserved. A menu-only pass or provisional joined
portrait cannot substitute for modular content. Current selected wave preview
remains available until a coherent candidate has real runtime evidence.
