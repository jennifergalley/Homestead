# Round 2: the farming year and period crafting

Kicked off 2026-09-29. Plan: `openspec\changes\rework-farming-calendar-and-period-crafting` (proposal,
design, specs, tasks; on `main` at `64183cfb`). The docs agent keeps this page current; report changes to
it rather than editing lane rows yourself. Round 1's page (`round-1.md`) stays as the record of that
round; its shared-machine rules and lessons carry over through `docs\handoff\README.md` and the skills.

## Registry

**App names** follow "<one or two words> Agent" (`docs\handoff\README.md`). The worktree folder is each
session's **mailbox address**; session IDs are for `send_session_message`.

| Name | Session | Worktree (`E:\Repos\copilot-worktrees\SurvivalGame\...`) | Scope |
| --- | --- | --- | --- |
| Orchestrator Agent | `92eac339` | `jennifergalley-cautious-pancake` | coordinates only; forwards `[ready]`s, relays to Jenny |
| Integration Agent | `e251051b` | `jennifergalley-literate-eureka` (MCP 8775) | merges batches, builds, tests, packages; the reserved Unreal slot |
| Documentation Agent | `a9f10974` (project session `d99bb15c`) | `jennifergalley-stunning-dollop` | process docs, this page, findings from every lane |
| Architecture Agent | `a1648ae7` | `jennifergalley-cuddly-invention` | code steward: `docs\architecture.md`, code conventions, safe refactors, batch reviews |
| **A. Calendar Agent** | `f8b77021` | `jennifergalley-studious-doodle` | `Homestead::Calendar`, gentle hunger, crop seasons and withering; **lands first** |
| **B. Harvest Agent / temporary Gait Agent** | `65a2408b` | `jennifergalley-vigilant-fishstick` | peas, wheat, barley, leeks, winter broccoli; withered silhouettes; temporarily lowers the running foot swing apex |
| **C. Seedsman Agent** | `5cf73757` | `jennifergalley-fluffy-broccoli` | Tregear's shop, the watering can, Sunday closing |
| **D. Seasons Agent** | `fd682909` | `jennifergalley-improved-giggle` | forage seasons, `MPC_Season`, winter canopies, frost |
| **E. Crafting Agent** | `ce241dd6` | `jennifergalley-stunning-waddle` | workbench, sawhorse, fences and gate, furniture, dishes, craft categories; **the only lane editing `SHomesteadMenu`** |
| Performance Agent | `a34483d7` | `jennifergalley-refactored-doodle` (branch `jennifergalley-performance-agent`) | frame rate and pacing; holds the perf window while measuring (winter canopies are on its list) |
| Weather Agent | `89914e30` | `jennifergalley-silver-guide` | weather and water, continuing from round 1 |
| Build Speed Agent | `6e131c6a` | `jennifergalley-automatic-spork` | idle |

Lane names in the app may lag behind this table while sessions rename themselves.

## Model, reasoning and implementer slots

Jenny's standing team preference (2026-09-29). These are **required settings for future session
launches**; documenting them does not change a live session's model or reasoning level.

| Role | Model (exact ID) | Reasoning | Context |
| --- | --- | --- | --- |
| Documentation Agent | GPT-5.6 Terra (`gpt-5.6-terra`) | **high** | **long** |
| Architecture Agent | GPT-6 Sol (`gpt-6-sol`) | high | long |
| Orchestrator Agent | GPT-6 Sol (`gpt-6-sol`) | **medium** | **long** |
| Implementer (Blender, Unreal or code work) | Claude Opus 5.5 | high | long |

**At most three concurrent hands-on implementers** do Blender, Unreal or code work. This is a cap
across active work, not a role-label exemption, and is separate from the 2-Unreal-process machine cap.
The Integration Agent counts while merging, compiling, PIE testing or packaging, but not while only
coordinating; Architecture counts while editing or building code; Docs counts while implementing tooling.
Time-critical integration gets a slot by pausing a lane. The orchestrator grants the next slot before a
waiting lane resumes. An idle or waiting session schedules a wake-up and ends its turn; it doesn't hold
a slot by sleeping or polling.

**Current implementer slots (2026-09-29):** Weather Agent, temporary Gait Agent (Harvest) and
temporary Menu Agent (UI). Integration released its hands-on slot after pushing `27e2e917`; UI/Menu
resumed. These three hold the slots until the 2 PM packaging rotation.

## Lanes and ownership

The design's "Lanes and ownership" table is authoritative. In short:

- **A** owns `Homestead::Calendar`, the `Step()` hunger and energy changes, `WorkCost`, the season
  rollover hook, crop season masks, planting and focus warnings, withering, the HUD calendar text, and
  hunger and season toasts. **It exposes `Calendar` and the rollover hook to B-D first, within a day.**
- **B** owns the five `CropKind` rows, their plant and withered sets (Blender), and withered plot visuals.
- **C** owns `ShopKind::Seedsman`, Sunday closing, the seed stock move, the tin watering can, Tregear's
  building, interior and props, and the `SeedsmanDoor` anchor.
- **D** owns the forage season table, the blackberry item and picking, field mushrooms (kind, placements,
  prop), `MPC_Season` and the seasonal material edits.
- **E** owns the Workbench, Sawhorse, Fence, Gate and furniture pieces and meshes, recipes and dishes, and
  the Craft categories in `SHomesteadMenu`. It's independent of A.

## Rules this round (carried over)

- **Two Unreal processes** machine-wide, **one reserved for the Integration Agent**; every other lane
  shares the second, one at a time (`Start-EditorMcp.ps1` enforces both). Close your editor as soon as a
  verification pass is done (`Scripts\Stop-MyEditor.ps1`).
- **Perf window:** measurements run alone (`Scripts\Start-PerfWindow.ps1`); don't launch Unreal, build
  or run Blender (including a headless batch) while someone holds it. Headless Blender skewed a perf run
  about 2x.
- **Waiting means ending your turn:** schedule a wake-up with `save_session_automation`, never sleep or
  loop.
- **Build only when your C++ changed,** once per batch, with `Scripts\Invoke-UnrealBuild.ps1`; lanes don't
  build the game target.
- **Items and other data enums are append-only,** in small commits rebased onto `main`
  (`HomesteadItems` catalogue rows match enum order; never reorder or remove). Any list or per-enum array
  in a tagged save section is written with its count first.
- **`SimulationSaveVersion` is bumped once, at final integration** (design §10: `Plot::withered`, the new
  enum values, gate state, the second shop, `dayMinutes`). Lanes never bump it; tell the orchestrator
  before your `[ready]` if you add to the save format.
- **Playtest builds** on the "Homestead Estate" shortcut by 7:30 AM daily and 4:00 PM on weekdays
  (`docs\handoff\README.md`, "Playtest builds"). `main` must stay playable.
- **Jenny's playtest feedback takes priority** over round-2 lane work.
- **Launching with ray tracing off:** `Start-EditorMcp.ps1` also turns virtual shadow maps off; any other
  RT-off launch must too, or a new Estate game hangs the GPU (editor skill, table 0.1).
- Delivery: "Delivering lane work" in `docs\handoff\README.md` (`[ready]` to the orchestrator).

## Order and first increments

1. **A** lands `Calendar` and the season rollover hook first; B, C and D build on them.
2. **E** proceeds in parallel.
3. The Integration Agent merges in batches, and the save version is bumped once at the end.

## Registry: estate placement ids and scenery kinds

Claim a range here (through the docs agent or the orchestrator) before using it, and keep the comment at
`Simulation\HomesteadEstate.h` ~87 in step. `ProvisionalEstatePlacements` adds sections in id order,
because later sections keep clear of earlier ones; placements are append-only (`next++` ids), and moving
or removing them needs `table.bakeVersion` raised. Details are in round 1's registry.

| Placement ids | Owner |
| --- | --- |
| 500000+ | world lane |
| 510000+ | overgrowth |
| 520000+ | salvage piles |
| 530000+ | town (reserved) |
| 540000-540043 | berry brambles |
| 550000+ | derelict farm and estate disrepair |
| 560000-569999 | MVP woodland biome interactables |
| 570000-579999 | clear-out near the manor |
| 580000-580999 | field mushrooms (D, Seasons Agent; about 40, generated by `Scripts\Terrain\mushrooms.py` into `Simulation\HomesteadEstateMushroomPlacements.inc`) |
| *next free: 581000+* | *claim here* |

| Scenery kinds (`EstateSceneryKinds`; `scatter.py` kind bytes must match) | Owner |
| --- | --- |
| 13-18 | trees (oak, beech, sycamore, hawthorn, holly, hazel coppice) |
| 19-41 | MVP woodland biome |
| *next free: 42+* | *claim here* |

## Shared interfaces this round

- **`MPC_Season`** (D): `/Game/SurvivalGame/Environment/Seasons/MPC_Season`, scalars `SeasonBlend` (0-4),
  `Autumn`, `WinterBare` and `Frost` (0-1), written by `AHomesteadWorld`. Materials read seasons from it. When
  you change the MPC, rebuild and commit the materials that use it in the same change (editor skill, table 0.1).
- **`Simulation\HomesteadSeasons.{h,cpp}`** (D): the forage season table and `Seasons::LookAt`.
- **Enum values being appended** (append-only; keep catalogue rows in enum order): D adds `Item::Blackberries`,
  `Item::FieldMushrooms` and `ResourceKind::FieldMushrooms`. Lanes list theirs here as they claim them.
- **Seedsman anchors claimed (C; branch `4f21a2d8`, not on `main` yet):** `Anchor::SeedsmanDoor`
  `{-52050, 116000}`, yaw 0; `Anchor::SeedsmanCounter` `{-51450, 116000}`, yaw 180. The lane adds them
  to the provisional layout, `reshape.py` and `estate_layout.json` together.

## Open questions for Jenny

- **Day length:** 30-minute days (the design's default), or 60? Settings keep 60 and 120 either way.
- **Sunday closing:** should shops close on Sundays? (A per-shop data flag, so easy to turn off.)
- **Names:** Tregear's and its keeper are placeholders.

## Lane status

Not started. Lanes send `[ready]` to the orchestrator; the docs agent records what's integrated here.

## Jenny's 2026-09-29 playtest feedback

Jenny is playing the **morning package** now. Protect its running process and its `Saved` folder: do not
focus, kill, retarget or overwrite the currently running package without coordinating with her. This is **not**
a global pause: headless code and Blender work may continue, and a separate editor slot may be used under
the 2-Unreal-process and 3-implementer limits. The 4 PM build remains active. If she is still playing at 2 PM,
the Integration Agent prepares a **new** playtest folder and runs its suites without closing her game (or asks
her to pause before the final shortcut switch); the existing shortcut stays untouched until she confirms. These
are feedback items, not implementation detail; the owner creates or updates the linked OpenSpec change before
work that goes beyond a small verified correction.

| Feedback | Owner | OpenSpec reference / note |
| --- | --- | --- |
| **Approved:** a lake in the owned geographic north-west (Jenny's map reads north-up, right=east, so her visual "up-and-left" overrides her spoken "NE") at **(-50, -745) m**, about **76 × 44 m**, with roughly 110 m from the farm edge and a ~90 m farm-side access path | Weather Agent (`89914e30`) | `author-fixed-cornish-estate-map`; `add-regional-landforms-water`. Jenny can redirect this after seeing it. |
| **Approved:** a clearly signed manor-south gate/path with protected switchbacks or stairs to a **12–20 m dry beach** along the owned ~630 m cliff coast | Weather Agent (`89914e30`) | `author-fixed-cornish-estate-map`; `add-regional-landforms-water` |
| **Approved:** the wider walkable beach above (12–20 m dry width) below the owned cliffs | Weather Agent (`89914e30`) | `author-fixed-cornish-estate-map`; `add-shore-and-river-fishing` |
| The river looks as if it stops before reaching the ocean in Jenny's screenshot | Weather Agent (`89914e30`) | **Partly checked, not resolved:** PIE on `27e2e917` confirms the source, banks and water down to the beach, but the ribbon still ends a couple metres short of the ocean, separated by sand/foam. Own a later true estuary connection while widening the beach; do not mark the gap fixed. |
| Running foot kicks too high toward her butt; lower its swing apex slightly | Harvest Agent / temporary Gait Agent (`65a2408b`) | `polish-locomotion-view-distance-and-time-hud` |

**Water-lane order:** verify the river mouth in the parked 4 PM river branch first, then stage **lake → beach → route**. These are unverified slices: they do **not** edit the 4 PM package. If the terrain or water work needs placement ids, the Water Agent claims them through this page before using them (the registry starts at 581000+).

**Priority:** these playtest items take precedence over the ordinary round-2 feature queue. They are
all **pending, not shipped**. The orchestrator assigns an implementer slot before any owner starts
hands-on work; no one edits a busy lane's files or starts a fourth implementer.

## Open blockers and known bugs

None yet.

## Pending playtest feedback

- **Field book notification overlay** (Jenny, 2026-09-29): a successful transfer or craft currently
  shows a banner at the top of the inventory/field book that reflows the menu and pushes its items.
  Replace it with a brief, floating notification that looks modal over the menu but **is not a real
  modal dialog**: it auto-disappears, preserves the current focus and input, and shifts no content at
  1080p or 4K. **UI Agent / temporary Menu Agent** (`5cf73757`) owns this when the orchestrator grants
  an implementer slot. Pending; not shipped.
- **Appearance controls and naming** — **UI / Menu Agent** (`5cf73757`), when granted a slot:
  click-drag rotates the preview, WASD orbits it, wheel zooms it, and the default heroine faces the menu
  regardless of the wall or world yaw. Rename the user-facing **Curly Bob** option to **Long bob**.
- **Hair groom** — **temporary Gait Agent** (`65a2408b`), after the running-heel and scythe work:
  investigate the intermittent exploding/sticking-out groom.
- **Gait and scythe** — **temporary Gait Agent** (`65a2408b`), after its current run-heel slice:
  lower the running foot swing apex slightly; at rest the scythe must sit in her hand, and its blade
  must not clip the terrain during a sweep.
- **Sprint toggle** — **temporary Gait Agent** (`65a2408b`), after the run-heel work: sprint becomes
  a toggle on controller L3 and PC Shift. Update the controls and hints; pending PIE verification.
- **Contextual hotbar eating** — **UI / temporary Menu Agent** (`5cf73757`), after the book-overlay
  toast: selected hotbar berries are edible with controller X or A **only when no higher-priority
  focused action exists**. Preserve focused interactions, and update the controls and hints; pending
  PIE verification.
- **Music variety** — **Architecture Agent** (`a1648ae7`) first does a read-only trace and licensing
  review; the implementation owner is **TBD**. The result should have multiple randomized tracks and
  longer ambient-only silence between them.
- **Context hint** — **UI Agent** (`5cf73757`): the hint says Ctrl+wheel zooms, but gameplay uses the
  wheel to cycle the hotbar. Correct the context copy.
- **Invisible weeds** — **Clearing / Props lane** (owner **TBD** when an implementer slot opens):
  inspect forage-node visuals and culling; a visible weed asset must exist wherever the prompt says
  `"Weeds  E  Pull"`.
- **Manor debris** — **Clearing / Props lane** (owner **TBD** when a slot opens): slate and shingle
  piles that look clearable should become suitable saved clearables, rather than static scenery.
- **Rusted hoe wayfinding** — **UI Agent / Docs review**: add in-game guide or wayfinding to the rusted
  hoe blade; a documentation-only answer is insufficient.
- **Starter food** — **balance owner TBD**: add more food at the start, balancing the starter kit
  without removing hunger prematurely.
- **Hunger and energy model** — **Planning Agent** (evidence/design recommendation): investigate whether
  the two systems should collapse to one bar like *Coral Island*. Do not silently remove either system.
- **Hearth, birds and door** — **Audio / door owner TBD** after slots open: increase the fireplace
  modestly while keeping it inaudible outside its room; muffle outdoor bird ambience indoors; add a
  working wooden standing-room door that opens and closes automatically on entering or leaving.

## Decisions during the round

None yet.

## Pending doc updates on merge

- Seedsman (`4f21a2d8`, not on `main` yet): Tregear's replaces `Town_Blockout_EastHouse`; the lane
  deletes that World Partition external actor and `town_massing.py` stops spawning it. When it lands:
  document the `WorldPartitionBlueprintLibrary.get_actor_descs()` + `load_actors(guids)` step before
  `get_all_level_actors` (otherwise a re-run duplicates unloaded massing), the external-actor save that
  removes the deleted actor file, and the shop-trade pointer rule (FindShop pointers dangle after Sell/Buy;
  keep the shop id and look it up again). Add a store-recipe cross-link to the Blender -Y/+Y `Prop()` yaw
  convention.

- Performance (`6841f429`, not on `main` yet): `PerfLock.ps1` treats `blender.exe` as a build, so
  `Start-PerfWindow.ps1` names and refuses it. When it lands, update the editor skill perf-window
  bullet to say the script enforces the no-Blender rule.
- Crops (`b51aa930`, from round 1's polish): `HomesteadGrowCrops <days> [tend=1]` and
  `HomesteadCropGrowth <0-1>` grow crops for tests (time skips don't). When it lands, add them to the
  editor skill's "Time and weather for tests" bullet.
- Performance (`0c12d5e9`): `Test-Game.ps1 -RenderScale 0`, `-ExtraExecCmds`, `-ExtraArguments`, and the
  `-UserDir` CSV path. When it lands, add them to the editor skill's perf notes.

## Tooling requests (unassigned)

- `get_play_state` (`st`) should report `namesOpen` and `shopOpen` (carried over from round 1).

## Next

When round 2's lanes are integrated: the save-version bump, a playtest build, and Jenny's answers to the
open questions.
