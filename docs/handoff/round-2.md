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
| **B. Harvest Agent** | `65a2408b` | `jennifergalley-vigilant-fishstick` | peas, wheat, barley, leeks, winter broccoli; withered silhouettes |
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

**Current implementer slots (2026-09-29):** Integration Agent (time-critical batch), Calendar Agent
and Performance Agent. Harvest Agent was paused to give Integration the slot; Seedsman Agent, Seasons Agent
and Crafting Agent are also paused until the orchestrator grants a slot.

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

## Open questions for Jenny

- **Day length:** 30-minute days (the design's default), or 60? Settings keep 60 and 120 either way.
- **Sunday closing:** should shops close on Sundays? (A per-shop data flag, so easy to turn off.)
- **Names:** Tregear's and its keeper are placeholders.

## Lane status

Not started. Lanes send `[ready]` to the orchestrator; the docs agent records what's integrated here.

## Open blockers and known bugs

None yet.

## Decisions during the round

None yet.

## Pending doc updates on merge

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
