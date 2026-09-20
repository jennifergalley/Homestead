# Design

## Context

See `proposal.md` for the requested change and prerequisite. This design is a
reviewable plan, not evidence of implemented UI or accepted art. Baseline
inspection: `61b3b30bbf695bdebd032d81aea617985c72e924`, isolated worktree
`E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-solid-memory`.
The active environment implementation can advance independently; rebase/reconcile
only after its accepted completion, under a later implementation authorization.

`docs\game-plan.md` and `PRODUCT.md` govern product truth. `DESIGN.md` describes
the incumbent native Canvas UI, not the proposed grid. Its warm Pine/cream/Gold
language is retained; this is a new composition inside that identity. Impeccable
shape guidance informs the interaction brief below. Its context script did not
recognize the explicit Windows/Unreal platform and emitted web advice; no web
detector, CSS, browser game or platform-file rewrite follows from that mismatch.

The first supplied Coral Island image was reviewed: icon navigation, paperdoll,
item/equipment grids, details and translucent world backdrop. The other two
images were not available under the image quota. No private portrait was opened.
No runtime, tooling, installed engine, content or assets were changed or launched
for this plan. Source inspection is not an observed playtest of the proposed UI.

### Observed implementation and implications

| Evidence | Observed contract | Consequence |
| --- | --- | --- |
| `HomesteadController.cpp`, `SetupInputComponent`, `OpenBook`, `Rows`, `ActivateRow`; `HomesteadHUD.cpp`, `DrawBook` | Seven pages: Pack, Craft, Build, Notes, Settings, Credits, Look; selected-row-following seven-row window at supported 16:9 sizes | Preserve purposes, not the obsolete row layout or index-based tests |
| Controller lines 617-643 and 795-798 | Save and quit is index 9, the tenth Settings row; successful `SaveSlot` precedes `QuitGame` | Existing operation is correct in principle but initially offscreen; this is a discovery/failure-flow repair, not simply another button |
| Controller lines 126-157 and 489-564 | Escape/B from gameplay opens Settings; I/Tab/Menu opens Pack; horizontal arrows and D-pad currently change pages, LB/RB also change pages; no pointer hit-testing | Introduce explicit menu-only binding rules for grids and native mouse controls |
| Controller `Interact`, `Back`, `ToggleBook`, `QuickSave`, failure transition in `Tick` | Failed state sends confirm/back to retry, prevents normal book/save access and closes book/planning | A separate recovery Settings/quit route is required |
| `HomesteadPromptIntent.cpp/.h`, Controller `InputKey` | Sole accepted-event hint classifier; short-window signed mouse travel, axis shaping, held-stick protection | Widgets must not add competing detector writers or bypass classification |
| Controller `SaveSlot`, `ReadSave`, `LoadLatest`, `ApplySave`, `RetryCheckpoint` | Protected temp/backup save; loader validates wrapper and simulation, ignores failed saves, same-world healthy recovery; in-memory fallback stores simulation only | Preserve IO, make wardrobe/appearance checkpoint-coherent; exit cannot save a failed world as a usable checkpoint |
| `Simulation\HomesteadSimulation.h/.cpp` | 14 fungible item kinds, five recipes, fixed arrays; pack and each chest total 120 units; chest reach 280 cm; copy/validate-style atomic transactions | Add wearable identity without converting to a slot economy or rewriting world rules |
| Simulation `State`, `Step`, `SetWarmOutfit` | A persisted `warmOutfit` flag exists; negative warmth rate receives +2 when true; runtime cosmetic outfit code does not connect to it | Preserve it as legacy simulation state, do not infer a new garment bonus |
| `HomesteadSave.h`; portable `Serialize`/`Deserialize` | UE wrapper writes v4, accepts v1-v4; `HOMESAV1` envelope with CRC32; portable writes `HOMESTEAD 3`, accepts v2/v3, size plus FNV-style checksum | Two migrations, not one; old enum-array layout must be parsed with its old size |
| `HomesteadAppearance`; `HomesteadCharacter::LoadHeroineAssets/ApplyAppearance` | 3 body x 3 hair x 2 outfit = 18 joined meshes; tint palette; no garment slots/instances | UI slots alone cannot implement clothing |
| `Scripts\Characters\export_heroine.py`, `prepare_body`, `main` | Deletes covered torso vertices and vertices below Z 0.145 m, then joins body/hair/clothes before export | Do not strip the tunic/shoes from cooked meshes; reconstruct a covered modular base from source |
| `Assets\Characters\provenance.json`; `build_heroine.py`; `extend_variants.py` | Original tunic/apron; documented CC0 MakeHuman sources, game_engine rig, shoes01 includes shoes/socks; adult phenotype retained | Reuse admitted source and rig; footwear includes socks as one instance; no face/hair redesign |
| `SurvivalGame.Build.cs` | Engine/InputCore/EnhancedInput/etc., no UMG/Slate/CommonUI dependency | A native widget layer is an explicit small dependency change |
| `Tests\README.md`, existing native and engine fixtures | Rules/save tests, rendering guards, prompt tests, book clarity, active feedback, action lifecycle and preview routing | Preserve semantic assertions; adapt old row-index/mesh-count assumptions deliberately |

## Goals / Non-Goals

**Goals:** A small native menu shell that delegates mutations to existing game
authority; safe ownership and save conversion; real independent garments; a
usable exit from all in-session states; bounded visual/performance proof.

**Non-Goals:** A UI framework, new gameplay progression, generalized RPG
equipment/stat system, drag/drop in this first release, live cloth simulation,
whole-character replacement, engine patch, new networking exception or
automatic candidate promotion. Existing shortcomings outside this request,
including mid-back waves and the blonde-bob redesign, stay deferred.

## Decisions

### 1. Narrow C++-authored UMG shell; retain gameplay Canvas

Recommended path: C++-authored `UUserWidget` composition using UMG/Slate's layout,
focus, scroll, hit testing, buttons and tooltips. Add only required UMG, Slate
and SlateCore module dependencies. Keep `AHomesteadHUD` for the normal world HUD,
planning and existing diagnostic accessors until replacements are specifically
wired; suppress the old book/Look draw when the new shell owns the screen.
Keep game rules and save/exit authority in controller/simulation, not widget
callbacks. No CommonUI/CommonInput plugin in this change.

Why not extend Canvas: grid hit-testing, clipping, pointer hover, focus regions,
modal stacks and reflow would require substantial bespoke infrastructure in
`DrawHUD`. Canvas is still appropriate for the incumbent world HUD; doing all
of this there would be a bigger custom framework than the bounded widget layer.
Why not CommonUI: seven local pages do not yet justify a new input router,
activation stack and independent device classifier. Revisit only in another
proposal if the native spike proves a concrete need.

Proposed small units, names finalized during implementation:

- `UI\HomesteadMenuWidget`: shell, tab strip, page host, reserved feedback and
  footer. `UI\HomesteadItemGrid`, `HomesteadDetailsWidget` and small dialogs are
  components, not a universal screen framework.
- `UI\HomesteadMenuModel`: typed subjects/actions and stable focus keys. Models
  reference `ItemDefinitionId`, wearable instance ID, recipe/piece ID and chest
  ID; never interpret row text as gameplay state. Reuse `Result` messages.
- `UI\HomesteadIconRegistry`: definition/tab keys to hard-referenced cooked
  texture brushes plus provenance inventory. No runtime download/font-icon CDN.
- `HomesteadController`: owns open page, transaction submissions, pause reason,
  exit state and accepted-device intent; builds snapshots from simulation and
  listens for successful commits to refresh affected widgets.

Migrate in a candidate-only feature switch during development: first shell +
Settings, then inventory/details, craft/build/guide/appearance. The switch is
not a player-facing option and never writes alternate save representations.
Remove the old menu renderer only after parity checks; retain untouched normal
HUD/planning behavior. Preserve diagnostic APIs with semantic equivalents rather
than returning fabricated old rows to make assertions green.

### 2. Interaction brief and layout

**Job/mode:** Operate for possessions, equipment and Settings; Read for
Guidebook/Credits. Jenny opens the menu from ordinary controller play to find
what she owns, wear/store/use it, or leave safely. The memorable interaction is
selecting a real garment, seeing its details, equipping it and seeing the same
item on the heroine without leaving Inventory.

**Selected direction:** Wide translucent world overlay, quiet Pine backings,
cream text, Gold focus/active marks and original softly illustrated icons.
Keep bright scenery from becoming text backing. Use a tinted scrim without an
expensive full-screen live blur; content and footer have near-opaque readable
panels. Current native readable font is the starting point, not a copied Coral
Island face. More decorative art is subordinate to readable names and counts.

Logical 1280x720 starting layout, scaled with a single UMG DPI policy to 4K:

```text
+--------------------------------------------------------------------------+
| [bag] Inventory [tools] Craft [house] Build [book] Guidebook ... [gear]    |
| active underline + labels                   Settings  Credits Appearance |
| Reserved feedback line: full message / expand for persistent error        |
+----------------+-----------------------------------+---------------------+
| Character      | YOUR PACK   37 / 120 units        | Selected / hovered  |
| preview        | [Carried] [Nearby chest: 24/120]  | icon + item name    |
|                | [icon 12] [icon 4] [icon 1] ...   | location, quantity  |
| Equipment      | [icon  1] [icon 1] [icon 8] ...   | truthful details    |
| Tunic          | scrollable cells; no slot cap    | dye / requirements  |
| Apron          |                                  |                     |
| Footwear       | Transfer destination named       | explicit actions    |
| [Appearance]   | Empty/disabled guidance in place | Equip / Move / ...  |
+----------------+-----------------------------------+---------------------+
| time paused   LB/RB tabs   LT/RT regions   A actions   B back   ...       |
+--------------------------------------------------------------------------+
```

This is a structural wireframe, not final artwork. At 720p use approximately
224/624/320 px column allocations, 16 px gutters, 20 px outer margins, a
72 px top band, a reserved 56-64 px feedback band and 48-56 px footer.
Minimum normal text 16 physical px at 720p, action targets at least 44x44 px,
focus border at least 2 px, normal text contrast target 4.5:1 and focus/large
text 3:1 against final composed backing. Record actual measured pixels, not
only chosen token values. Allow detail-body scrolling instead of shrinking.
Validate 200%-length synthetic labels; essential action names cannot ellipsize.

Start with six item columns in the central grid; calculate columns from its
usable width/minimum cell size (approximately 88 px), not total item count.
Keep the logical order on reflow. At constrained widths show pack/chest as
labeled subpanes in the same central region, not two unreadable side-by-side
grids. Both container totals and named transfer destination remain visible.
Only reachable chests appear; select the nearest deterministically (distance,
then stable chest ID for ties), capture its ID and revalidate on commit.
No remote warehouse access or new chest-selection game system is implied.

Top tabs retain the seven purposes in existing order. Inventory includes a
paperdoll and "Appearance" shortcut; Appearance retains body/hair/skin/eye
controls and preview but removes free outfit granting. Dye acts on a selected
owned garment in Inventory. Guidebook becomes clearly named Notes content, with
new equipment instructions replacing the old "two cosmetic outfits" guidance.
Credits retains all existing attribution and appends actual new art attribution.
Craft/Build use output icons, "Needs" and availability, never "Carried" styling.
Settings is a control grid grouped into Session, Gameplay/Camera, Audio, Video;
Save/Load and restart remain distinct actions, and VSync retains override copy.
No map/social/crown/buff tab is inferred from the reference.

Settings' top pinned session bar is outside scroll content:
`Resume | Save and quit to desktop`, initial focus Resume, Right reaches exit.
Escape/B from gameplay -> Right -> confirm opens the exit dialog in three
presses; pointer users click it directly. All other pages have the labeled
Settings tab. "Start a new clearing" lives in a separate lower danger section
and receives its own cancel-default confirmation, replacing a fragile two-tap
row latch while retaining existing save files.

### 3. Input ownership, focus and lifecycle

Preserve world bindings. Only while the menu owns input:

| Input | Menu action |
| --- | --- |
| D-pad or keyboard arrows | Four-direction content/grid movement; settings directional value adjustment only inside an explicitly activated editor |
| Left stick | Optional repeated grid navigation using existing shaped stick intent; no pawn movement |
| LB/RB; Ctrl+Shift+Tab / Ctrl+Tab | Previous/next top tab, wrapping across the seven tabs |
| LT/RT; Shift+Tab / Tab | Previous/next focus region, cycling through tabs, preview/equipment, content, details actions and pinned actions as present |
| A; Enter/E; left click | Activate focused control or open the subject's explicit action list |
| X/F | Context action advertised in footer, normally Move/transfer for selected possession |
| Y/G | Secondary advertised action, including amount/split where applicable; no unadvertised destructive shortcut |
| B/Escape | Cancel innermost dialog/editor, then close menu; never simultaneous world action |
| I/Menu | Close menu only with no nested dialog; otherwise behaves as cancel one level |
| Wheel | Scroll hovered menu region, not camera zoom while menu owns the pointer |
| R3; right-stick/drag orbit | Preview control only when preview region is explicitly active; no gameplay camera leakage |

Menu Tab therefore changes from book-toggle to region focus while already open.
Outside menus Tab still opens Inventory; Left/Right and D-pad retain existing
craft/build/placement behavior. These intentional changes require guide/footer
updates and regression tests, not silently duplicated bindings. Amount modal
uses left/right +/-1, shoulders +/-10, labeled One/All buttons, Confirm and
Cancel; amount clamps to available legal quantity and gives reasons for limits.
Top-tab and region shortcuts are suspended while a modal owns those keys.

Focus key = page + region + subject ID, not array index. On tab return restore
the surviving subject; otherwise nearest surviving logical index, then empty
guidance. Horizontal grid edges clamp, vertical movement preserves desired
column and chooses nearest valid cell on a short row, no row/tab wrap. Selection
is scrolled fully into view. Region traversal is explicit; disabled controls
can receive explanatory focus but cannot execute. Pointer hover is transient
details only; pointer click selects/activates. Controller input clears hover
preview so the visible details match controller focus. No hover-only actions.
No drag/drop in this version: explicit Move/Swap + destination selection and
Split dialogs are complete with all three devices and avoid cursor-held items.

Keep `FHomesteadPromptIntent` the only hint-device classifier. A focused UMG
widget can consume events before the old controller sees them, so establish one
accepted-event adapter feeding the existing classifier for the currently owned
input surface. It performs the same synthetic-only input gate in tests and raw
axis treatment, then dispatches each event once. Do not call both controller and
widget classifier for one event or use native hover/focus as deliberate intent.
Widget-level automatic navigation must not duplicate our grid movement. Test
physical-source-style events and simulated routes, noise and held-stick
sequences. No CommonInput detector, no extra `bGamepad` writers.

Pause remains the simulation's existing menu/planning pause, not a global engine
pause that would freeze Slate, feedback lifetimes or preview capture. A small
controller-owned mode plus modal depth replaces accidental independent booleans
only where necessary: Gameplay, Planning, Menu(page), Recovery, with modal child
states. Capture movement/action cancellation and gameplay camera on entry.
Cancel planning on menu entry as `OpenBook` already does. Do not resume planning
on close. Transfer/craft mutate only after confirmation; save serializes committed
state, never an in-flight amount/destination choice. F5 during a transaction
cancels that draft visibly before saving; F9 cancels drafts before the existing
quick-load operation. Do not add an unrequested confirmation to world F9.
Pause stops ambient simulation ticking; successful explicit craft/build/eat
operations retain their existing time/resource/need costs, not a free-action
loophole. Browsing, hovering, equipping and canceling add no new time cost.
Any save/load/recovery invalidates old focus handles and restores a consistent
camera/input owner. Existing world F5/F9 meanings and hotkey safety stay intact.

### 4. Exit state machine, not transient-toast-only errors

```text
Gameplay -- Escape/B --> Settings (pinned exit)
Any menu -- Settings tab --> Settings
Planning -- Back cancels / Settings opens --> Settings
Recovery -- Y/G or Settings button --> Settings (recovery origin)

Settings -- Save and quit --> Confirm (default Stay)
Confirm -- Save and quit --> Saving -- verified success --> Desktop
                                |
                                +-- failure --> Persistent save error
                                                  | Retry --> Saving
                                                  | Return --> Settings
                                                  + Quit without saving
                                                        |
                                      Unsaved warning (default Cancel)
                                                        |
                                                explicit Quit --> Desktop
```

An exit view model tracks `Idle/Confirm/Saving/Failed/UnsavedConfirm` and an
operation token; repeated activation while Saving is ignored, not reissued.
Saving owns input before the old failed-state/quick-action dispatch and blocks
other mutations, F5/F9 and restart until its immutable snapshot finishes.
Use the existing `SaveSlot` temp-readback/backup/move path and report its exact
failure. Strengthen its returned result to include stage/message/committed
revision, rather than asking the toast whether saving worked. Verify success
before `QuitGame`; do not change the OS or bind Alt-F4 as the primary flow.
Synchronous IO may remain initially with a visible pending state and input
guard; no new async save architecture is required. The save snapshot is immutable.

Track last successful durable save time and a session mutation revision for
simulation, appearance, layout and relevant settings. Unknown legacy recency is
shown as unknown. Do not say "nothing to lose" just because an autosave was
attempted. Graphics preferences persist through Unreal config separately; keep
their failed-write status until a successful retry. After successful world save,
an outstanding graphics error still requires an explicit choice to leave with
that preference unsaved. Do not roll back the good world save.

Recovery retains A/Enter/E = Retry checkpoint. Add labeled Y/G/mouse
"Settings / Quit"; Back inside recovery Settings returns to recovery, not retry.
Saving a failed state is disabled because `LoadLatest` deliberately excludes
failed saves; the pinned alternative is "Quit to desktop", using the unsaved
warning and explaining the latest usable checkpoint. Do not auto-retry or load
just to satisfy quit. With no usable checkpoint, retain clear failure text and
explicit unsaved exit. Startup routing validation occurs before this UI and
continues its explicit failure/exit without accessing another profile.

### 5. Small item definition and ownership model

Preserve the existing 14 `Item` integer values and their aggregate arrays for
fungible tools/materials/food/water. Do not treat a recipe, base layer or preview
as inventory. Add a narrow wearable catalog alongside those definitions:

| Proposed definition | Ownership/slots | Source and acquisition |
| --- | --- | --- |
| `linen-tunic` | One nonstacking instance; reserves Torso + Legs | Original existing tunic split/exported properly; craft 12 Fiber, knife required |
| `linen-apron` | One instance; Apron layer, requires compatible tunic | Original existing apron; craft 6 Fiber, knife required |
| `legacy-laceup-shoes` | One instance; Feet, includes existing socks | Existing documented CC0 shoes01; starter/legacy item, not craftable from nonexistent leather stock |
| `woven-footwraps` | One instance; Feet | New original compatible fiber footwrap geometry; craft 8 Fiber, knife required |

Costs are **proposed initial content defaults**, not existing recipes or claims
of historical tailoring accuracy. Fiber is explicitly worked into cloth in
recipe copy; no new cloth/loom/leather economy, free recipe output or village
dependency. Preserve existing tool recipes (4 Branch/3 Stone/2 Fiber hatchet,
3 Branch/1 Stone digging stick, 3 Branch/2 Fiber watering can) and cooked recipes
unchanged. Garment costs/result/requirements come from one authority used by
execution and UI, extending `RequirementsMatchTransactions`.

Each wearable instance stores world-local monotonically allocated ID, definition
ID, dye index and a tagged owner (`Carried`, `Chest(chestId)`, `Equipped`).
An equipped slot map holds references only; a two-slot tunic is one instance.
Its ID deliberately appears in both reserved slots, which is valid only when
the definition declares that exact slot set; that is not duplicate ownership.
Keep `nextWearableId` separate from existing world `nextId`. No duplicate
authoritative copies in a widget or `FHomesteadAppearance`.
Definition data holds occupied slots, allowed underlayer/fit, material roles
and craftability; presentation data maps to mesh/icon assets without putting UE
types in the portable simulation.

Capacity: `sum(fungible counts) + count(wearables owned by container) <= 120`.
Equipped clothes consume no pack units, with the modest base never an item.
All materials count one unit each exactly as today. An empty grid cell grants
no capacity, and splitting a stack consumes no extra units. A full pack can
swap one carried tunic for one equipped tunic because final-state capacity is
unchanged; removing tunic plus dependent apron requires two free units.

Do not introduce physical stack entities for the existing fungibles just to
draw a grid. Persist a small ordered presentation list per container containing
wearable references and fungible groups `(stableGroupId, itemKind, quantity)`.
The aggregate count remains ownership authority; groups must partition it
exactly. Default one group per item kind; Split partitions a positive amount
into another group, Merge combines same-kind groups, Move/Swap changes order.
Quantity changes update these groups in the same transaction: explicit source
group first, otherwise consume in stable display order; additions merge into
the first same-kind group or append. A zero group is removed, never displayed
as owned stock. Bounded by 120 positive-quantity groups/instances per container.
Persist a separate monotonic `nextGroupId` for stable group focus/order keys;
neither allocator reuses deleted IDs or touches resource/structure `nextId`.
This offers meaningful split/reorder persistence without a 24-slot economy.

Transactions run in the portable rules layer against a candidate state:
validate instance/source owner, amounts, reachable chest, type/fit/slot rules,
dependent displaced garments, final capacity and layout; then commit once.
Suggested APIs are typed `MoveWearable`, `EquipWearable`, `UnequipWearable`,
`CraftGarment`, `RecolorWearable`, plus existing `Transfer` and layout operations.
Use a result code/message and change revision; UI prevents duplicate operation
tokens and the authority rejects stale expected revisions. This is local atomic
mutation, not a distributed transaction framework.

Direct chest-to-equip is intentionally not offered initially: take to pack then
equip, with capacity feedback. One-click equip from pack relocates old equipment
to pack atomically. Removing a tunic offers "Remove tunic and apron" if needed;
changing to another compatible tunic leaves apron equipped. Incompatible layer
fit either displaces all affected items in the confirmed preview or rejects.
Close/cancel never changes ownership. Resolve and retain all required mesh/
material references and validate fit before submitting the game-thread equip
commit. Only then atomically change ownership and swap prepared presentation;
never commit and subsequently discover that a mesh must load. Missing content
rejects the operation, not the previous outfit. A save with valid ownership but
missing required packaged art is a content failure: preserve its bytes/state
and offer exit, never classify its possessions as corrupt and replace them.

Dye: retain Moss/Wine/Slate/Flax for tunic/apron as a free cosmetic recolor
affordance, now bound to an owned selected instance. Footwear initially has its
authored material and no unsupported dye action. Preserve legacy tunic/trim
role-specific tint behavior, not a flat replacement color. No dye-resource or
warmth economy is inferred. `warmOutfit` remains serialized with its existing
effect; clothing UI neither writes it nor advertises it.

New games create one worn starter tunic and one pair of existing footwear
directly equipped, plus the existing knife and no extra materials. The apron
must be crafted unless a legacy save actually wore it. This is a specific
starter grant, not a free outfit toggle. No world-drop/delete/sell action is
added in this change.

### 6. Real garment rendering and safe base layer

Do not separate the cooked 18 meshes by simply hiding tunic/shoe materials.
`prepare_body` already removed skin/feet, and shared materials can include
other parts. Work from verified local authoring source and reconstruction
records. Preserve the MakeHuman `game_engine` bind rig, existing scale and
adult phenotype, face/body shapes, hair meshes/materials and animation clips.
The newly installed Blender version is not automatically the approved version
for these existing MPFB scripts; prove compatibility in the authorized workflow.

Author a permanent original opaque modest fitted underlayer covering the
existing covered torso/hip region, and complete supported feet/base foot
covering. It is part of the body presentation, never removable, tradeable,
craftable or equipped. This avoids exposing unclothed states or invisible holes.
Recover/re-export the underlying geometry from source where needed; new
occlusion masks must be explicit garment/fit data applied only when covered.
Never reuse one deletion mask for every clothing combination. Validate the
base-alone state before adding garments.

Proposed runtime topology: nine retained body+hair base combinations (3x3)
on the current skeleton, each with the permanent base; separate body-fitted
tunic, apron, shoes/socks and new footwrap skeletal components. Fit variants
may share source geometry but must be demonstrated for all three bodies.
Garment components follow the same evaluated pose (leader-pose when identical
bone layout is proven; otherwise explicit copy-pose with measured cost).
No new animation blueprint per garment, duplicated world collision, physics
asset or cloth simulation. Remove old component references on replacement.
Legacy full meshes remain candidate rollback assets until parity is verified,
not hidden render duplicates.

Keep material slot contracts: ColorTint on supported garment roles, skin/hair
unchanged, IrisColor/IrisMix and DefaultLit/masked eye contract preserved.
Garment dye cannot recolor held tools, skin, hair or eyes. Validate idle,
walking, gathering, watering, weeding and hatchet action across body fits,
including coverage and hem/apron overlap. Screenshot pose checks are supplemented
by ordinary walking/action footage; no claim of fingertip contact or cloth physics.

Inventory uses one local preview proxy of the same body/equipment presentation
data, not a second inventory or gameplay pawn. An on-demand scene capture makes
the figure visible in the left panel while the real paused world remains behind
the overlay; it does not move or relight the world or persist a preview camera.
Start with 384x576 at 720p, at most 768x1152 at 4K, update on equip/appearance/
orbit and at most 15 Hz while actively orbiting; otherwise cache it. Release on
close and avoid a capture for each icon. Label preview lighting as neutral
inspection lighting; validate gameplay daylight/firelight separately. If this
bounded capture fails the performance gate, reduce update/resolution and report
quality limits, not replace the preview with a nonfunctional paperdoll.

Icons are original local illustrations or approved isolated in-engine renders
of admitted assets, baked offline into an icon atlas/texture set. No reference
extraction or network generator. Reuse a shape icon plus per-instance tint where
that faithfully matches the garment, not one flat color over skin/background.
Registry must cover all 14 existing items, every new garment, five existing and
three new craft recipes, seven building pieces, seven tabs and empty slot cues.
Footwraps are the fourth wearable definition and the third added recipe; the
legacy leather shoes have no craft recipe. Verify readability at actual grid size,
text alternatives, mip/alpha behavior and cook inclusion. Missing art is logged
and blocks acceptance; development placeholders stay explicitly provisional.

### 7. Two-layer save migration and checkpoint coherence

Reserve portable schema **v4** and UE wrapper **v5** for this change, subject to
checking that the accepted environment baseline has not already claimed them.
Never reuse a version for incompatible layouts. Preserve old numeric item,
recipe and world IDs; new garment definition IDs are explicit stable values.

Portable v4 retains old simulation fields including `warmOutfit`, then bounded
wearable records, nextWearableId, equipment references, nextGroupId and container
layouts.
UE v5 retains nonclothing appearance/settings/camera/world data and embeds the
portable payload. Old `Outfit`/`TunicColor` fields remain read-only legacy inputs,
not another authoritative wardrobe. Validate a compatible wrapper/payload pair;
reject contradictory new-schema states rather than running legacy grants again.

Conversion sequence:

1. Verify outer size (existing 4 MiB bound), `HOMESAV1` magic and CRC before
   decoding wrapper. Apply existing finite/range/valid-WorldId checks. Wrapper
   versions 1-4 currently share this validation, with UPROPERTY defaults for
   absent appearance fields; there is no observed reset/generate-new-world
   migration to preserve.
2. Parse portable v2/v3 using the **old 14-element inventory/storage layout**
   and existing checksum/length and v2 roots-only crop migration. Never parse
   old arrays with a new `ItemCount`. Construct a candidate, not the live sim.
3. Pass validated legacy outfit/dye into an explicit portable conversion
   context from the UE adapter. Standalone legacy-fixture callers must provide
   that context too; do not silently guess an outfit from the portable text,
   which never stored it. A legacy parse lacking necessary context reports a
   migration-context-required error and cannot be committed/serialized as v4.
4. Give tunic ID 1, footwear ID 2 and, only for outfit index 1, apron ID 3,
   in a separate world-local wearable namespace; reserve IDs 1-3 and begin
   `nextWearableId` at 4. IDs are deterministic within the unchanged WorldId,
   including when reading an old `.bak`. Preserve TunicColor on tunic/apron,
   original footwear materials and all body/hair/skin/eye values.
5. Mark initialized wardrobe schema in the new payload; validate ownership,
   slot fit, owner/container existence, dye, counts, next IDs, bounded strings/
   vectors and layout partition. Initialize legacy layouts in existing enum
   order, then owned garment order; equipped starter grants cost no pack units.
6. Apply the whole validated candidate and its appearance together. Cache the
   decoded/migrated candidate so `ReadSave`, `LoadLatest` and `ApplySave` do not
   independently create instances or disagree about eligibility.
7. Read migration is in-memory only. On the next actual save, serialize new
   versions through existing temp validation, backup and replacement. Source
   slots are not eagerly rewritten; a crash before commit simply rereads the
   same deterministic old snapshot. Once v4/v5 exists, its ownership is the
   authority regardless of legacy fields or where old garments now reside.

`LoadLatest` still chooses newest usable saves and considers `.bak`, and recovery
still prefers same-world sheltered checkpoints with current health thresholds.
Reject duplicate IDs/owners, dangling chest IDs, incompatible layers, zero or
negative groups, count mismatch, exhausted counters and unknown definitions.
Do not "repair" a corrupt garment by dropping it or spawning a default outfit.
Log rejection and surface backup recovery. If all migration candidates fail,
offer recovery/exit; do not overwrite files or present a new clearing as success.

Replace simulation-only `SessionCheckpoint` with a coherent in-memory snapshot
including simulation/wardrobe, nonclothing appearance and matching location/view/
WorldId; capture it only at its existing logical checkpoint boundary, not whenever
an item changes. Manual/autosave/recovery snapshots use the same representation.
Loading a checkpoint replaces the current state; it never merges inventories or
copies a garment from a later snapshot. Health/progression differences between
checkpoints remain existing recovery semantics, not migration loss.

Golden fixtures: synthetic portable v2 and v3 plus actual saved wrapper versions
1-4 (freeze bytes and provenance before changing their serializer), both outfits,
all four tunic dyes and representative/all supported body/hair combinations.
Do not merely write a new wrapper with Version=1 and call it a historical binary;
label synthetic emulations and retain at least baseline-produced v4 bytes.
Include full 120-unit pack/chests, legacy warmOutfit true/false, v2 crops,
nondefault IDs, placed buildings, cooldowns, failed/healthy checkpoints,
multiple worlds and corrupt/future versions.

Failure injection covers pre-serialization, temp write/readback, backup copy,
replacement, and process interruption before/after replacement. Compare counts,
IDs, owner sets and recovery metadata after separate-process reload, not just
"load succeeded". Verify v4/v5 -> v4/v5 round trips with reordered/split groups
and stored formerly equipped legacy garments; repeat legacy reads are snapshot
reads, never merges/grants.

### 8. Target files, adapters and existing test surfaces

| Area | Current targets and proposed additions |
| --- | --- |
| Controller / UI | `Source\SurvivalGame\HomesteadController.h/.cpp`, `HomesteadHUD.h/.cpp`; new narrow `UI\` widgets/models; `SurvivalGame.Build.cs` |
| Intent | `HomesteadPromptIntent.h/.cpp` remains authority; adapter and `HomesteadPromptTest.cpp` integration |
| Rules | `Simulation\HomesteadSimulation.h/.cpp`; optionally extract wearable/catalog/layout helpers to adjacent portable files if size warrants; preserve existing transaction entry points |
| Appearance/rendering | `HomesteadAppearance.h/.cpp`, `HomesteadCharacter.h/.cpp`; shared admitted wardrobe presentation data and bounded preview component |
| Persistence | `HomesteadSave.h`, Controller save/read/apply/retry functions; `HomesteadSaveRouting.cpp/.h` routing should not require semantic change |
| Source/content | `Scripts\Characters\build_heroine.py`, `export_heroine.py`, `extend_variants.py`, `build_body_presets.py`, relevant `import_*.py`; new modular source/output manifest alongside `Assets\Characters`; admitted UE garment/icon content |
| Portable checks | `Tests\HomesteadSimulationTests.cpp` and frozen synthetic fixtures; `Scripts\Test-Native.ps1`, CMake source list only if helpers are extracted |
| Runtime checks | `HomesteadBookClarityTest.cpp`, prompt/feedback/VSync/hotkey/save-routing/presentation/action fixtures; add focused menu/wardrobe/migration/quit cases |
| Documentation | After implementation update `DESIGN.md`, `docs\game-plan.md`, `docs\playtest-feedback.md`, `docs\setup.md`, `Tests\README.md`, and actual asset provenance/credits |

Integration commands already available include `Scripts\Test-Native.ps1`,
`Scripts\Test-Game.ps1 -RequireLit`, `Test-FeedbackLayout.ps1 -RequireLit`,
`Test-HotkeySafety.ps1`, `Test-VideoSync.ps1`, `Test-PreviewSaves.ps1` and
`Tests\PreviewLauncherTests.ps1`. Extend or add a focused runner using their
existing isolated-output convention; do not claim a new flag exists today.
Development-only fixtures do not automatically work in Shipping: the approved
offline pipeline must admit the exact runner/binary or provide a reviewed
Shipping-safe observer. A blocked tool is a stop, not grounds to omit evidence.

### 9. Acceptance and bounded evidence budget

Verify each milestone's available behavior as it is built, without waiting for
wardrobe assets to prove exit and existing-item navigation. Whole-change visual
acceptance still requires functional correctness and complete icon/garment
assets. Exactly two matched visual sets at most:

- **Set A:** registered baseline plus first complete version of each surface,
  collected incrementally for the early lane and completed when wardrobe is
  integrated, using the same synthetic
  world/camera/time/weather/settled exposure and physical output sizes. Inspect
  Inventory equipped/base-only, hovered/focused material and clothing details,
  nearby/full/empty storage, quantity dialog, Craft, Build, Guidebook, Settings
  pinned exit, save-failure dialog and Appearance.
- **Set B:** same states after one batched correction, or final confirmation if
  A has no defects. Include daylight and night/firelight garment gameplay views.
  If still unacceptable, report incomplete and retain baseline; no third polish
  cycle under this plan.

Cap each set at 40 retained raw stills total across 720p and 4K and at two
ordinary-play clips of <=45 seconds each, sampled at <=8 fps for local review.
Plan contact sheets for the full body/hair/equipment compatibility matrix within
that cap; automated state/material/bounds checks can cover all combinations
without a screenshot per assertion. Keep raw dimensions and capture metadata;
contact sheets/crops are labeled derivative views. Two sets are a review ceiling,
not automatic approval or a demand to retain redundant frames.

Functional acceptance matrix:

| Area | Required proof |
| --- | --- |
| Navigation | Controller-only and keyboard-only full tab/region/partial-grid paths, mouse hover/click parity, no pointer-only capability, stable return focus, no duplicate key dispatch |
| Information | All item/recipe/plan/tab icons present; real quantities/costs; empty and disabled reasons; text/contrast at 720p and native 4K; complete details and nonoverlapping active feedback |
| Ownership | Craft/equip/swap/unequip/store/take/recolor/reorder/split/merge conservation; full containers, dependent apron, stale target, cancellation and repeated confirm |
| Assets | Base alone, all body/hair presets and garment fits, dye/eye/skin/tool materials, ordinary walking/action movement and no naked holes |
| Saves | Golden old versions, deterministic migration, new relaunch, corruption/bounds, interrupted commit, backup selection, same-world recovery, coherent in-memory fallback |
| Exit | Three-press gameplay path to confirmation, all-page Settings path, planning/recovery, actual process exit only after good save or explicit unsaved confirmation, injected failures and successful retry |
| Lifecycle | Paused time/needs/crops, action cancel without extra reward, nested cancel, no clicks in world, camera restoration, load invalidates drafts |
| Preserved contracts | F5/F9 save/load only, actual Lit/LightingOn/ShaderComplexityOff, VSync/scale copy and persistence, original/profile isolation, no extra network permissions |

Performance is measured separately, never using screenshot/readback timings.
Use the accepted environment package as matched baseline, same quality/upscaler,
VSync and profile/settings: 30-second warmup and three 30-second screenshot-free
samples for closed-menu gameplay, Inventory idle, Inventory orbit and rapid grid
navigation. Record frame/game/render/GPU percentiles and peak memory, capture
count/resolution and image/texture residency. Proposed regression limits:
closed-menu p95 <=5% above baseline; added menu CPU p95 <=2 ms and GPU p95 <=2 ms
relative to the matched paused baseline; added resident memory <=256 MiB;
no monotonic retention after 50 open/close cycles or repeated previews. Target
60 FPS remains the product goal, not a claim these deltas alone prove it.
If the environment baseline itself misses 60, report both baseline and candidate.
Any deliberate budget revision goes to coordinator review, not silent widening.

At native 4K record actual viewport and runtime primary resolution scale 100
for readability captures, plus render flags. Performance can additionally use
the accepted upscaled profile, explicitly labeled. Preserve graphics defaults,
VSync preference and override text. No physical tearing/VRR or human comfort
claim from offscreen frames. No exposure changes to hide garment defects.

## Risks / Trade-offs

- [Native input consumed before controller] -> One accepted-event adapter,
  typed classifier reuse and regression tests before migrating all pages.
- [Garment separation reveals deleted skin/feet] -> Source-level modest base
  and coverage proof first; no successful UI-only substitute for real equipment.
- [Legacy save appears valid but duplicates clothing] -> Deterministic reserved
  migration IDs, explicit schema marker, validate once/apply whole snapshot,
  interrupted-write and stored-migrated-item fixtures.
- [Display groups become a second inventory] -> Counts remain authority;
  exact partition validation and atomic layout reconciliation for every mutation.
- [UMG and scene capture add cost] -> Small shell, event-driven refresh, one
  bounded cached preview, no full-screen blur or per-cell 3D captures.
- [New garment recipes expand content effort] -> Three fiber recipes and four
  garment definitions only; leather starter shoes are not falsely craftable.
- [Save upgrade defeats package rollback] -> Preserve versioned old-profile
  snapshot and upgraded profile separately; no implicit downgrade support.
- [Environment/tool plan changes while this waits] -> Task 1.1 rechecks accepted
  baseline and permissions; no work inherits today's unfinished preflight.
- [Reference interpreted as final art approval] -> Record one reviewed image,
  original assets only, two review sets and explicit human aesthetic limits.

## Migration Plan

The dependency table in `tasks.md` permits an early visible lane after section 1:
build and verify the minimal native Settings shell/discoverable exit, then
existing-item grids and hover/focus details against current item/save authority.
Do not wait for all ownership, migration and modular garment work to show those
improvements. No early equipment claim, fake wardrobe, temporary save schema or
extra art-review cycle is permitted. Early captures belong to Set A's existing
budget; mixed-scope tasks stay unchecked until fully complete. Sections 2/3 and
compatible garment assets remain prerequisites for functional equipment and
whole-change completion, and exit tests run again with the migrated schema.

1. After environment acceptance and new apply authorization, record the accepted
   source/package, approved exact tools, branch/worktree and save isolation.
   No automatic schedule follows from artifact completion.
2. Create synthetic old-save fixtures before changing serializers. The early
   visible lane can proceed independently of portable ownership/migration and
   conservation work after section 1; preserve existing saves in that lane.
3. Prove the modular base/garment asset spike through the approved pipeline;
   reject/stop if it cannot preserve supported adult presets and coverage.
4. Complete the early shell/exit/focus with all pages, renderables and typed
   transaction actions once their ownership/persistence/asset prerequisites pass.
   Remove old free outfit writers and dependent assertions only when replacement
   behavior is tested.
5. Run the focused regression matrix and two bounded review sets; package to a
   fresh candidate directory with proper receipts. Leave `Preview.json`, the
   active player process and all personal/accepted profiles untouched.
6. Coordinator explicitly reviews acceptance. Before any human-profile upgrade,
   obtain/record consent for the one-time versioned snapshot; record schema
   compatibility in candidate receipts/launcher validation so an old candidate
   cannot silently initialize over an upgraded profile. An incompatible profile
   reports the issue and exits without saving, rather than starting a new world.
7. A rejected candidate stays unpromoted; preserve its synthetic evidence and
   return to the accepted package. If human testing has already upgraded a
   profile, rollback uses the consented old snapshot under a separate profile,
   preserving both histories. This plan never authorizes touching personal saves
   now or upgrading them as a verification shortcut.

## Proposed defaults and bounded unknowns

The tunic/apron/footwear slots, permanent modest base, no drag/drop, free
per-garment cosmetic dye and fiber recipe costs are explicit proposed defaults
for this review, not claims Jenny selected exact costs or approved final art.
They fulfill the request without adding winter balance, shops or a tailoring
tree. No material user-only decision blocks completion of the planning package.

Deferrable implementation facts: exact widget class split, authored base-layer
seams/fit corrections, atlas packing, and the accepted toolchain version at the
future start. These must satisfy the fixed contracts above. If safe modular
assets cannot meet the adult-character brief, or the approved authoring workflow
does not admit needed work, stop and present that concrete blocker to the
coordinator; do not shrink the change to cosmetic preset toggles.
