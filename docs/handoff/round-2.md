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
| **D. Seasons Agent (retiring; handoff to Water)** | `fd682909` | `jennifergalley-improved-giggle` | seasonal handoff branch `b19a0ad0`; Water owns completion |
| **E. Crafting Agent** | `ce241dd6` | `jennifergalley-stunning-waddle` | workbench, sawhorse, fences and gate, furniture, dishes, craft categories; **the only lane editing `SHomesteadMenu`** |
| Performance Agent | `a34483d7` | `jennifergalley-refactored-doodle` (branch `jennifergalley-performance-agent`) | frame rate and pacing; holds the perf window while measuring (winter canopies are on its list) |
| Weather Agent | `89914e30` | `jennifergalley-silver-guide` | weather and water, continuing from round 1 |
| Build Speed Agent | `6e131c6a` | `jennifergalley-automatic-spork` | idle |

Lane names in the app may lag behind this table while sessions rename themselves.

## Lean roster transition (pending archive confirmation)

Jenny's requested long-lived child roster is **Documentation, Integration, Architecture, Menu/UI,
Water and Props only**. Future feature work goes to these retained feature sessions, subject to the
three-hands-on-implementer cap (including Integration) and the two-Unreal-process cap.

| Current session / role | Planned disposition | Handoff condition |
| --- | --- | --- |
| Documentation Agent, Integration Agent, Architecture Agent | **Retain** | Long-lived team roles |
| UI Agent / temporary Menu Agent (`5cf73757`) | **Retain** | Menu/UI feature owner |
| Weather Agent / Water (`89914e30`) | **Retain** | Water and terrain owner |
| Props Agent | **Retain role; session TBD** | Receives props and character asset recipes |
| Performance Agent (`a34483d7`) | Retire | After safe handoff/merge |
| Build Speed Agent (`6e131c6a`) | **Archived** (orchestrator confirmed 2026-09-29) | Handoff complete: `f6ed1c42` and `a624c704` are on `main`; Integration owns the build recipe |
| Seasons/Mushroom Agent (`fd682909`) | Retirement requested; archive pending confirmation | Handoff branch `b19a0ad0` is unmerged; Water owns completion |
| Calendar Agent (`f8b77021`) | Retire | `a3c7e04d` is held: Props must first add and test the retain-60 correction; Integration merges both commits atomically |

**No archive is complete until the archive tool confirms it.** Keep each retiring session's code,
commit and handoff links in this page until then; remove stale wake-ups and ownership pointers only
after confirmation.

Gait and Planning are not orchestrator children. **Gait is idle and ready for Jenny to archive, not
archived:** its editor exited, tracked work is pushed, no automation remains, and only 97 disposable
`crop_preview_*.png` files are untracked. Sprint is handed to Menu and character asset recipes to
Props. For the run/hair change, Integration may cherry-pick `1197f02a` + `b1d43919` alone to avoid
the unready Harvest ancestor `ad534632`; Integration is verifying. Planning remains outside this
child-roster decision.

**Build Speed handoff:** the Integration Agent owns `Scripts\Invoke-UnrealBuild.ps1` and the build
recipe. Canonical measurements and the non-adopted UBA cache decision remain in
`docs\research\build-speed\README.md`. The lane's private-PCH rule stays in
`homestead-code-conventions`: after C2027/C2065, include the type's header in the file; add it to
`SurvivalGamePCH.h` only when common across the runtime module, never UnrealEd. The UBT mutex is per
engine installation; compile throughput hinges on physical free memory and UBA's 85% commit threshold.

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

**Current slot / rotation snapshot (2026-09-29 11:09):** Props released its weeds slice slot, then
received the third **headless** slot solely for the retain-60 Calendar correction below. Props must
release it by the 2 PM packaging window. The next editor sequence is **Menu → Water lake → Props weeds**;
the orchestrator continues to enforce the three-hands-on and two-Unreal-process caps.

## Lanes and ownership

The design's "Lanes and ownership" table is authoritative. In short:

- **A** owns `Homestead::Calendar`, the `Step()` hunger and energy changes, `WorkCost`, the season
  rollover hook, crop season masks, planting and focus warnings, withering, the HUD calendar text, and
  hunger and season toasts. **It exposes `Calendar` and the rollover hook to B-D first, within a day.**
- **B** owns the five `CropKind` rows, their plant and withered sets (Blender), and withered plot visuals.
- **C** owns `ShopKind::Seedsman`, Sunday closing, the seed stock move, the tin watering can, Tregear's
  building, interior and props, and the `SeedsmanDoor` anchor.
- **D handoff / Water** owns the forage season table, the blackberry item and picking, field mushrooms
  (kind, placements, prop), `MPC_Season` and the seasonal material edits. Water receives the unmerged
  handoff branch `jennifergalley-mvp-woodland-biome @ b19a0ad0`.
- **E** owns the Workbench, Sawhorse, Fence, Gate and furniture pieces and meshes, recipes and dishes, and
  the Craft categories in `SHomesteadMenu`. It's independent of A.

## Rules this round (carried over)

- **Two Unreal processes** machine-wide, **one reserved for the Integration Agent**; every other lane
  shares the second, one at a time (`Start-EditorMcp.ps1` enforces both). Close your editor as soon as a
  verification pass is done (`Scripts\Stop-MyEditor.ps1`).
- **Perf window:** measurements run alone (`Scripts\Start-PerfWindow.ps1`); don't launch Unreal, build
  or run Blender (including a headless batch) while someone holds it. Headless Blender skewed a perf run
  about 2x.
- **Cross-session messages are immediate:** every `send_session_message` uses
  `delivery_mode: "immediate"`, never default/enqueue. Keep it short, self-contained and actionable.
  For an urgent blocker or rule change that must reach a busy session mid-turn, also use `mailbox_send`
  to its worktree. Older queued messages can arrive late: honor the newest timestamp or explicit
  decision, and ignore stale superseded instructions.
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
| 580000-580999 | field mushrooms (Water; handed off from Seasons; about 40, generated by `Scripts\Terrain\mushrooms.py` into `Simulation\HomesteadEstateMushroomPlacements.inc`) |
| 581000-581099 | **reserved exclusively** for Water roadside forage (approval 2026-09-29; `roadside.py` → `HomesteadEstateRoadsidePlacements.inc`) |
| 582000-582099 | clearable manor ruin rubble (Props; `HomesteadEstateRuinRubblePlacements` in `HomesteadEstate.cpp`) |
| *next free: 582100+* | *claim here* |

| Scenery kinds (`EstateSceneryKinds`; `scatter.py` kind bytes must match) | Owner |
| --- | --- |
| 13-18 | trees (oak, beech, sycamore, hawthorn, holly, hazel coppice) |
| 19-41 | MVP woodland biome |
| *next free: 42+* | *claim here* |

## Shared interfaces this round

- **`MPC_Season`** (Water; Seasons handoff): `/Game/SurvivalGame/Environment/Seasons/MPC_Season`, scalars `SeasonBlend` (0-4),
  `Autumn`, `WinterBare` and `Frost` (0-1), written by `AHomesteadWorld`. Materials read seasons from it. When
  you change the MPC, rebuild and commit the materials that use it in the same change (editor skill, table 0.1).
- **`Simulation\HomesteadSeasons.{h,cpp}`** (Water; Seasons handoff): the forage season table and `Seasons::LookAt`.
- **Enum values being appended** (append-only; keep catalogue rows in enum order): Water's handoff adds `Item::Blackberries`,
  `Item::FieldMushrooms` and `ResourceKind::FieldMushrooms`. Lanes list theirs here as they claim them.
- **Seasons handoff contents (`b19a0ad0`, unmerged):** `HomesteadSeasons.{h,cpp}` (forage table:
  blackberries Summer 15–Autumn 28, field mushrooms Autumn, spring flowers Spring, meadow herb
  Spring–Autumn), blackberries/mushrooms and icons, `ResourceKind::FieldMushrooms` plus the 580000+
  placements, the unimported Blender `FieldMushrooms` prop, `HomesteadWorldSeasons.cpp` (MPC writer
  and `homestead.SeasonDay` / `homestead.Frost` overrides), `build_season_materials.py` and a native
  test. Native tests pass **9/9** on that branch. Still required before `[ready]`: import the mesh,
  run the material script (the MPC asset does not yet exist), and PIE season captures.
- **Road geometry:** C++ `EstateLayout` has landmarks and polygons, not a road polyline. Native code
  needing road geometry derives it from `Scripts\Terrain\estate_layout.json`; don't invent a separate
  C++ path.
- **Field-book map destination names reserved:** **Town** and **Manor**. The future travel action and
  UI use these exact user-facing names; other map work must not reuse them.
- **Seedsman anchors claimed (C; branch `4f21a2d8`, not on `main` yet):** `Anchor::SeedsmanDoor`
  `{-52050, 116000}`, yaw 0; `Anchor::SeedsmanCounter` `{-51450, 116000}`, yaw 180. The lane adds them
  to the provisional layout, `reshape.py` and `estate_layout.json` together.

## Open questions for Jenny

- **Day length:** resolved 2026-09-29 — new Estate games default to **60-minute days**. Settings
  continue to offer 30 and 120 minutes, and existing saves retain their stored value. The source
  OpenSpec is updated only after Props implements and verifies the corrective commit held below.
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
| **Integration pending; not shipped:** approved lake in the owned geographic north-west (Jenny's map reads north-up, right=east, so her visual "up-and-left" overrides her spoken "NE") at **(-50, -745) m**, about **76 × 44 m**, with roughly 110 m from the farm edge and a ~90 m farm-side access path | Weather Agent (`89914e30`) | `[ready]` `2bc18c46` (lake `df19d74a` plus main merge): Editor build/native 8/8, two saved proxies, six landscape tiles / 4,279 vertices with a subsequent zero diff, on-foot farm-to-landing path, pail kneel/fill 6, knee-deep wading barrier, peaty water, and book-map lake/path evidence. Integration still needs packaged smoke of actor/material, landscape-vs-r16, visual and pail behavior; lake is small on the map and lacks dawn/dusk and packaged-cook evidence. |
| **Approved:** a clearly signed manor-south gate/path with protected switchbacks or stairs to a **12–20 m dry beach** along the owned ~630 m cliff coast | Weather Agent (`89914e30`) | `author-fixed-cornish-estate-map`; `add-regional-landforms-water` |
| **Approved:** the wider walkable beach above (12–20 m dry width) below the owned cliffs | Weather Agent (`89914e30`) | `author-fixed-cornish-estate-map`; `add-shore-and-river-fishing` |
| The river looks as if it stops before reaching the ocean in Jenny's screenshot | Weather Agent (`89914e30`) | **Partly checked, not resolved:** PIE on `27e2e917` confirms the source, banks and water down to the beach, but the ribbon still ends a couple metres short of the ocean, separated by sand/foam. Own a later true estuary connection while widening the beach; do not mark the gap fixed. |
| **Pending:** around 9 PM the Estate visibly brightens and moonlight reads like sunlight | Architecture traces the nighttime directional/skylight/auto-exposure path; Water owns a later measured fix | Integration captures packaged **RT-on** evidence at 19:00, 21:00 and midnight. No fix is shipped or inferred from the trace. |
| **Pending:** town buildings are bunched too tightly | Water, after the final road route | The current 12 blocking blockouts occupy a 40 × 34.5 m four-sided square with 0.2–0.35 m adjacent gaps (about 0.9 m by the store); the main road ends ~72 m short. Make a ~60 × 45 m open square with terraces/cottages, 3–6 m side lanes, and a separate curved 5–6 m `townStreet`; preserve main-road 1.94 km chainage plus StoreDoor/Counter and Shop IDs/saves. |
| Running foot kicks too high toward her butt; lower its swing apex slightly | Harvest Agent / temporary Gait Agent (`65a2408b`) | `polish-locomotion-view-distance-and-time-hud` |

**Water-lane order:** the north-west lake is `[ready]` but **Integration pending, not shipped**; it
is eligible only if the listed packaged smoke gates pass. Verify the river mouth in the parked 4 PM
river branch, then stage **beach → route**. Night lighting and the town-road layout are separate later
increments after the north-west lake; town-entry/store acceptance, coordinate bridge, roadside forage
and travel signs wait for the final road route. If the terrain or water work needs placement ids, the
Water Agent claims them through this page before using them (the registry starts at 581000+).

**Priority:** these playtest items take precedence over the ordinary round-2 feature queue. They are
all **pending, not shipped**. The orchestrator assigns an implementer slot before any owner starts
hands-on work; no one edits a busy lane's files or starts a fourth implementer.

## Open blockers and known bugs

### Held planning OpenSpec correction

Planning's `b82eec53` OpenSpec docs briefly landed on `main` in `514c48c5`, with **no gameplay code
changed**. **Final planning decision (2026-09-29):** Integration merges Planning `16f4ed5b`, not the
tiered `e2159ea2`: flat **3 game hours** of Well Fed at ×0.85, Meals +25/+40/+60 energy, the
full-energy ≥1-hour extension guard and explicit `CorruptSave` validation. Treat current `main` wording
as pending correction until Integration reports the final merged SHA. Calendar must not implement stale
`b82`; this handoff does not duplicate or revert the shared OpenSpec artifacts.

### Calendar day-length / town-arrival blocker

Calendar lane A's `a3c7e04d` changes the default `dayMinutes` **60 → 30**. On the ~1.94 km
manor-to-town road, that doubles the in-game walk to roughly 12.3–14.4 hours: an 8 AM departure reaches
town about 8:19–10:22 PM, after the 6 PM General Store close. **Product decision:** new Estate games
keep the 60-minute default; the 30/120 Settings options remain, and old saves retain their stored
`dayMinutes` value.

**Integration hold:** Props is adding a focused, safely tested restore-60 correction on top of
`a3c7e04d`; Integration merges that pair atomically. Do **not** merge raw A, and exclude both commits
from the 4 PM package. After Props reports the implementation SHA and verification, update the
source-of-truth OpenSpec and then replace this blocker with Integration's final `main` SHA.

### OpenSpec strict-validation baseline (fix pending merge)

`openspec validate --changes --strict` still fails on current `main`: **56 passed, 3 failed (59
items)**. Every `ADDED` requirement needs at least one `#### Scenario:` with WHEN/THEN:

| Change | Spec requirement missing a scenario | Owner / fix |
| --- | --- | --- |
| `enrich-estate-ground-and-meadow` | `estate-ground-presentation`: “Soft steps on grass” | Water's `9d587d6a` adds the scenario; pending merge |
| `fix-estate-river-source` | `estate-river`: “The river is always present” | Water's `9d587d6a` adds the scenario; pending merge |
| `flexible-sleep` | `flexible-sleep`: “Recovery by hours slept” | Water's `9d587d6a` adds the scenario; pending merge |

Water validated **59/59** locally with `9d587d6a`; until it lands, this blocks only strict whole-repo
validation, not feature implementation. The OpenSpec house rules require a scenario for every `ADDED`
requirement.

## Pending playtest feedback

- **Field book notification overlay** — **Menu `[ready]` `88180744`, Integration pending; not
  shipped.** The open-book hoe-craft notice has zero 4K layout-pixel difference and long words wrap.
  It replaces the menu-reflowing banner with the brief floating non-modal overlay: it auto-disappears,
  preserves focus/input, uses a Victorian licensed font with larger centered text and a compact
  content-sized frame. Integration inspects it for `main`/package inclusion.
- **Appearance controls and naming** — **UI / Menu Agent** (`5cf73757`), when granted a slot:
  click-drag rotates the preview, WASD orbits it, wheel zooms it, and the default heroine faces the menu
  regardless of the wall or world yaw. Rename the user-facing **Curly Bob** option to **Long bob**.
- **Hair groom** — **temporary Gait Agent** (`65a2408b`), after the running-heel and scythe work:
  investigate the intermittent exploding/sticking-out groom.
- **Gait and scythe** — **temporary Gait Agent** (`65a2408b`), after its current run-heel slice:
  lower the running foot swing apex slightly; at rest the scythe must sit in her hand, and its blade
  must not clip the terrain during a sweep.
- **Long/tousled hair at angles** — **temporary Gait Agent** (`65a2408b`), while testing in its
  editor after the run rebake and before sprint: investigate the screenshot's disappearing hair and
  side patches at some camera angles. Pending; not shipped.
- **Sprint toggle** — **temporary Gait Agent** (`65a2408b`), after the run-heel work: sprint becomes
  a toggle on controller L3 and PC Shift. Update the controls and hints; pending PIE verification.
- **Contextual hotbar eating and berry feedback** — **Menu `[ready]` `88180744`, Integration
  pending; not shipped.** Controller A/X eats one selected berry per tap only when Talk has no
  precedence, and displays `+` the actual bounded Energy delta. Integration reviews the evidence before
  package inclusion.
- **Music variety** — **pending, not fixed.** **Architecture Agent** (`a1648ae7`) traced the root
  cause: the shipped catalog loads only one track, `EveningHarp`, despite five named entries. The
  shuffle bag anti-repeats correctly when it has more than one track, but the existing 55–110 s gap
  (and first 18 s) are too short. Four other Kevin MacLeod CC-BY imported `.uasset`s exist only
  untracked in the orchestrator worktree; **do not commit them** under the music eligibility rule
  (new project-authored or verified CC0 only). Architecture owns read-only license sourcing. Its
  verified **CC0 recording** audition shortlist (links and license labels checked; no file imported
  or listened to yet) is:
  - Maarten Schellekens, *Whispers of the Glen* (2:41) and *Medieval Theme* (2:36), Free Music
    Archive: <https://freemusicarchive.org/music/maarten-schellekens/public-domain-1/whispers-of-the-glen/>
    and <https://freemusicarchive.org/music/maarten-schellekens/public-domain-1/medieval-theme/>.
  - cynicmusic, *A New Town (RPG Theme)* and *Town Theme RPG*, OpenGameArt:
    <https://opengameart.org/content/a-new-town-rpg-theme> and
    <https://opengameart.org/content/town-theme-rpg> (durations not yet recorded).
  - Optional *Celtic Loop* (<https://opengameart.org/content/celtic-loop>) may be too repetitive.

  A later dedicated music implementer auditions texture, period fit and clip silence before selecting
  anything; then imports only the selected CC0/original recordings with credits and catalog entries,
  uses 3–8 minute ambient-only gaps and a randomized 45–120 s first gap, registers
  `MusicShuffleBagTests` in CMake, and verifies packaged multi-track load and run.
- **Context hint** — **Menu `[ready]` `88180744`, Integration pending; not shipped.** Plain wheel
  cycles the hotbar; Ctrl+wheel zooms while the book has focus. Integration reviews it with the Menu
  batch.
- **Weed visibility and grounding** — **Props weeds-only branch
  `jennifergalley-weed-visuals @ 889cfde8`, partial; not shipped.** It has a new main-mesh fallback
  (no thimbleberry), Landscape-versus-r16 grounding/reseat behavior, and an unimported Cornish
  `WeedClump` Blender asset. Native tests pass 8/8 and the Editor compile passed. The branch still
  owes an eye-height PIE check, sweep of all **115** placements, day-and-dusk captures, and a
  new-game reload. Only after that evidence may it establish that a visible weed asset exists wherever
  the prompt says `"Weeds  E  Pull"` and that none float above slopes or patches.
- **Manor debris** — **Clearing / Props lane** (owner **TBD** when a slot opens): slate and shingle
  piles that look clearable should become suitable saved clearables, rather than static scenery.
- **Rusted hoe wayfinding** — **UI Agent / Docs review**: add in-game guide or wayfinding to the rusted
  hoe blade; a documentation-only answer is insufficient.
- **Energy and food balance** — **Calendar Agent** (lane A, task 1.3): the chosen direction is one
  visible **Energy** meter later, rather than a visible hunger-plus-energy pair. Keep serialized hunger
  compatibility; revise gentle-hunger penalties into energy/food balance and modest **Well Fed** meals.
  Planning traced the present state: Hunger starts at 85, drains 2/hour awake and fails at 0; Energy
  starts at 100 and drains through work; food restores both. The change is **pending, not in today's
  4 PM build**; Planning updates the OpenSpec spec.
- **Weather recurrence** — **Water Agent** (retained lane; supersedes the broader Calendar proposal):
  rain every third day is too frequent. The smallest traced change is a stable hash selecting offsets
  **1 or 2** and **6 or 7** in every 10-day block: exactly 20% rain, 4–6-day gaps and day 0 dry. Keep
  the current 09:00–15:00 rain window, overcast, moisture and audio behavior; no seed or new save
  section. Old saves' forecast can change, while accrued plot moisture persists; document that at
  implementation. Tests cover count, gaps and save/reload. **Pending; not shipped.** Calendar retires
  after its lane-A work.
- **Starter food and hoe wayfinding** — **Clearing / Props lane** (after weed/rubble work): put **3
  pasties and 2 loaves** in the starter chest. Make the **hoe head** the second salvage reward after the
  billhook, with a contextual refusal and west-chimney journal hints to find it. Pending; no code or
  content is finished yet.
- **Starter wardrobe** — **Props lane** (with starter food): put completed wearable clothes in the
  starter chest so Jenny can change outfit; verify pack/chest capacity and saving. Pending; not shipped.
- **Road-to-town forage** — **Water Agent** (`89914e30`): add pickable berries and herbs along the
  road to town, including the bridge approach. The ID range is reserved; implementation remains
  pending the narrow public-road-corridor proof and bridge coordinate sync.
- **Field-book road label** — **Water Agent** (`89914e30`): the redundant runtime `"Dirt road"` label
  is removed in lake `[ready]` `2bc18c46` (`HomesteadMapComponent::RefreshModel`; the road remains
  drawn), with a book-map lake/path screenshot. It is **Integration pending, not shipped** with the
  lake; no separate road-label merge occurred.
- **River road bridge** — **Water Agent** (`89914e30`), after the lake slice; a safe, walkable
  period wooden bridge where the road crosses the river. A Props mesh may be needed. Pending; not
  shipped.
- **Nighttime brightness** — **pending, not shipped.** At about 9 PM, the Estate visibly brightens:
  moonlight reads like sunlight. Architecture's read-only trace of current `HomesteadWorld.cpp`
  (`59–64`, `1715–1784`, `4268–4337`) found no 21:00 trigger: sun reaches zero around 18:23,
  moon intensity is 2 lux and rises from roughly 7 to 46 degrees by 21:00, real-time sky capture
  intensity is 0.6 (day 1), and night auto-exposure has a -2 EV100 floor (day 0). The combined
  moon/sky/adaptation cause is plausible, not visually proven.

  **Water** later runs a controlled existing-CVar trial — moon 0.1–0.3 lux, sky 0.1–0.2, and an
  exposure floor toward -1/0 — while preserving smooth dusk and lamp readability. Log and use a
  fixed camera at 18:00, 19:00, 21:00 and midnight in clear and rain. **Integration** judges
  packaged RT-on Lumen hardware-ray-tracing plus VSM evidence, because the normal agent editor launch
  disables RT/VSM; neither the trace nor screenshots alone establish a fix.
- **Town-road layout** — **Water Agent**, after the north-west lake and final road route: the 12
  blocking `town_massing.py` blockouts occupy a 40 × 34.5 m four-sided square with adjacent building
  gaps of only 0.2–0.35 m (about 0.9 m beside the General Store); the main road ends ~72 m short of
  TownSquare. Make a ~60 × 45 m open square, with irregular terraces/cottages, 3–6 m side lanes and
  a separate curved 5–6 m `townStreet` from the main-road end to the square. Keep the existing
  1.94 km main-road chainage stable, plus GeneralStoreDoor/Counter access and persisted `Shop.id`.

  Update `town_massing.py` external actors, the graded heightfield and `estate_layout.json`, road
  SDF/material/ruts, scenery clear mask, ground and map bake/import together. The controller
  currently uses `ProvisionalEstateLayout()` directly: mirror moved anchors into that C++ table;
  do not rely on the stale `DA_EstateLandmarks` claim. Verify walking from RoadTownEnd through the
  town entrance to the store counter without teleporting in day, night and rain. Only after the
  route is final can the coordinate bridge, roadside forage and travel signs be aligned. This is a
  later, separate increment and is **not** 4 PM package content.
- **Change Dye** — **UI / temporary Menu Agent** (`5cf73757`), after contextual berries: the current
  action is a no-op. It opens the selected colour or swatch choice, supports preview, confirm and
  cancel, then persists the selection. Pending PIE verification.
- **Leather backpack upgrade** — **pending, not shipped.** A tentative one-time **$15** purchase at
  the open General Store doubles inventory capacity **120 → 240 items**. (`ShopGoods` normally repeats,
  so this needs a special upgrade row.) $15 is intentionally above the $10 start—roughly ten cabbage
  harvests net $0.50—and is tunable later. The worn rucksack appears on her back and its Appearance
  show/hide is independent of capacity and saving.

  **Props** owns the core `bRucksackOwned` save state, `PackCapacity(state)` (120/240), validated
  optional trailing save section (old defaults false), and original leather back-socket prop; save
  loading reads structural inventory up to 240 **before** the entitlement tag, then runs post-tag
  `ValidateInventory`. **Menu** owns the shop upgrade row, `bRucksackVisible`, the Appearance toggle
  and the 120-cap UI helpers. Tests cover malformed/duplicate entitlement sections, rebuy refusal,
  insufficient funds and capacity/save behavior.
- **Town travel** — **pending, not shipped.** A wooden `Walk to town` sign outside the estate and a
  return sign by town; clickable **Town** and **Manor** destinations on the Map invoke the same travel
  action. Architecture traced the road polyline in `estate_layout.json` (486 points / 1.94 km; runtime
  has landmarks only). The MetaHuman walks 210 cm/s (legacy 180); at a 60-minute day, road-only travel
  is 6.16 game hours / 15.4 real minutes (12.32 game hours at a 30-minute day), plus connectors.

  Travel must first preflight a candidate advance for hunger failure, unexpected 6-hour doze and
  `MaxHour`, then atomically commit time plus a safe position through `PrepareWorldAt` /
  `SettleOnGround`; no unsafe fallback. On the Map, a single click selects and double-click/A zooms,
  so travel needs a separate explicit confirmation. The signs and map invoke the same action.
  **Water** owns the generated runtime route and endpoints, **Architecture** the read-only trace,
  **Props** the original signs and **Menu** the shared travel/map UI; a future travel implementer owns
  the action.
- **Wait for opening** — **Menu `[ready]` `88180744`, Integration pending; not shipped.** At a
  closed 19:00 store, B cancels with no time change; then A+A advances to the next 08:00 and returns
  Pascoe's Talk interaction. The safe candidate preflight/commit path remains separate from future
  equivalent-time Town/Manor travel signs and map actions. Integration reviews it with the Menu batch.
- **Hearth, ambience and standing-room door** — **pending, not shipped.** Architecture's read-only
  trace found hearth gain 0.2 (NaturalSound spatial 150+550 cm) with occlusion. A later **audio/door
  implementer** modestly raises it to ~0.3–0.35 and adds standing-room-specific containment, so the
  hearth stays quiet outdoors even with the door open. `ForestAmbience` is a non-spatial loop at default
  0.70 and never mixes indoors: expose a cheap `GetIndoorMix` from the existing
  `Weather::Indoors` roof/shelter easing (0–1), and apply indoor gain/low-pass to birds and creek while
  retaining the user's slider multiplier; the roof-overhead check must also run in sun. Architecture is
  separately checking rain audio. The asset itself is **not missing**: tracked/cooked `RainLoop.wav`
  (CC0 Ylmir, *Rain (loopable)*; credited) and its `.uasset` load non-spatial through
  `UHomesteadWeather`. Water's headless trace found the cause: `FadeIn(2, Gain)` followed by
  `SetVolumeMultiplier(Gain)` applies rain gain twice, leaving roughly 0.40 for a default shower and
  0.07 for drizzle, while source RMS is a healthy -24 dBFS. Its narrow branch correction uses
  `FadeIn(2, 1)` and leaves gain solely to the multiplier. Rain is intentionally silent on dry
  days/times (currently only day 2/3, 09:00–15:00).

  **The isolated code fix shipped** as `65726628` → `545e057b` on `main` `76b316a3` and is included
  in the 4 PM package. Editor and game builds, native tests (8/8), and the static-init check pass.
  Water's PIE evidence verifies dry off; rainy-noon indoor `IsPlaying` gain 0.245 / outdoor 0.63;
  drizzle indoor 0.105; and the two-second fade. Those values establish the single-gain formula
  instead of the prior `Gain²`; source RMS remains -24 dBFS. Lake and weather-schedule work were
  excluded.

  **Audible resolution remains unproven.** Low RAM and the Menu editor prevented Water's ears-on
  capture, and packaged `AudioProof` covers only legacy audio. Jenny can listen outdoors during rain
  on day 2, roughly 11:00–15:30, in the 4 PM package. Do not describe the subjective rain sound as
  conclusively fixed without ears-on or recorded Estate-rain evidence.

  The heritage-stone west doorway is a 130 × 220 cm gap with no leaf. **Props** queues an original
  oak-plank mesh and frame after the cove stairs. The later audio/door implementer makes the leaf
  world-owned and smoothly auto-hinged, non-trapping in physics, and PIE-verifies opening and closing.
  No save migration is needed. Architecture continues verified CC0 music sourcing separately.

## Decisions during the round

- **4 PM playtest package:** only independently verified `[ready]` slices are eligible. Pending
  feedback above is not included merely because it has an owner.
- **Calendar A package hold:** raw `a3c7e04d` is excluded. Its 60-minute-default correction must be
  implemented and verified by Props, then atomically merged with A by Integration.

## Pending doc updates on merge

- **Seasons roadside forage (581000–581099; range reserved, implementation not approved):** about 30
  BerryBush, Flowers, Primroses, WildDaffodils and Roots along the public manor-to-town road
  (`Scripts\Terrain\roadside.py` → `Simulation\HomesteadEstateRoadsidePlacements.inc`). Most lie
  outside `EstateBoundary`. Architecture's trace says spawn/gather/save/regeneration are
  position-independent, but PIE/native proof on the public road is still pending. Implementation needs
  a **narrow road-corridor predicate and positive/negative tests**, not a blanket outside-boundary
  exception, and must stay in sync with Water's road bridge.
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
