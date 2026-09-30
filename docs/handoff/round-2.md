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

**Current play/package snapshot (2026-09-29 19:20):** Jenny closed verified `Playtest-0929eve`;
packaged PID 2328 exited naturally, there are no Unreal processes and physical free memory is 9.2 GB.
Integration now has the exclusive editor/UBT slot (with memory watch) to build local aim `798ce775` plus
half-rain `c8765f72` (native 9/9), then PIE behind/front target and rain component behavior. It includes
Props' pending `[F] Pull weeds` hint before corrected Development package/six suites/copied-save smoke,
then Shipping `-ReuseCooked` zero-listener proof. Only after that chain may the Estate shortcut retarget.
Menu field-book hotbar and Water garden outline remain headless/partial and excluded.

### Development firewall prompts / offline Shipping candidate

Development packaged automation can trigger recurring Windows Firewall prompts: UE5.8's in-process
TraceControl listens even with `-notraceserver` / `-traceautostart=0` (for example TCP 1985, with
fallback ports), producing path-specific rules as each Playtest archive moves. This is unresolved for
Development test archives. Do **not** open public Allow rules, and do **not** globally disable
`NotifyOnListen`: it needs elevation and would silently suppress alerts for every Windows app.

After the current package/suites, Integration may build a safe Shipping candidate only through the
supported hash-compatible cooked path in [`docs/offline-startup.md`](../offline-startup.md):
`Build-Game.ps1 -Configuration Shipping -ReuseCooked -Package`, with isolated sandbox
test/game/save/network state. Shipping compiles out TraceControl. Only after its own validation may
the Estate shortcut be reconsidered; the current shortcut stays untouched.

### Urgent forward-aim shipment blocker

**Do not ship or retarget the night package.** Architecture found `main` `5d11ceed` forward-aim bug
`8df23ba3`: `FocusHeldToolTarget` skips the aimed lookup when the nearest current overgrowth matches
the held tool, then `SwingAtOvergrowth` overwrites the aimed ID with `FocusId`. A bramble 70 cm behind
can therefore beat a valid bramble 200 cm ahead; an under-tier thicket behind can block the front
target. Props has a headless targeted fix slot during UAT. Integration may finish current UAT safely,
but the night package remains **provisional, not delivered** until the fix has targeted native/PIE
evidence and a repackage passes. Preserve the prior verified `Playtest-0929eve` build and shortcut.

**Props headless fix `61957b51`:** cleanly based on `main` `144acc7b`, native 9/9, no UE/PIE or
shipping claim. `HeldToolFocus` now chooses aimed forward overgrowth over a nearer matching-tool
target behind while preserving forageables and non-overgrowth plots/drops/store/water. Swing uses
`FindAimedOvergrowth` only, never overwrites with `FocusId`; a behind-only press says `Turn to face it`
without a state change.

Native covers bramble 70 cm behind versus valid front 200 cm prompt/clear/one charge, side under-tier
thicket not blocking, berry prompt retained, no aimed behind clear, turn-around success and silent tier
refusal. Integration cherry-picked it locally as `798ce775` atop merged `main` `144acc7b`; native
Release 9/9 passes and review confirms Swing uses only `FindAimedOvergrowth`, never `FocusId`.
Editor/Game UBT succeeded in 362 seconds before the memory override; no Integration editor/PIE/UAT
process started. Jenny is playing verified Estate PID 2328 untouched, so Integration now wakes/waits
until her game closes and an explicit process/memory-safe grant resumes the chain.

Required remaining evidence is PIE behind/front, side thicket, `Turn to face it` and F5/F9, then
corrected Development repackage/six suites/copied-save smoke and final Shipping no-listener candidate.
The provisional night build remains withheld and
`Playtest-0929eve` remains the verified shortcut.

**Provisional night package evidence:** `Build\Playtest-0929night` at `main` `5d11ceed`
(`JennysHomesteadGame.exe` SHA256
`887EFD0254D2D9A98761CC4B65258878F635DEE5E0BDEB7222085F372EE18EC8`) passed all six packaged
Development suites: Smoke 57.2 fps / p99 16.8 ms, Clearing 52.4 / 21.9, Hotbar 52.3 / 34, NativeMenu
56.2, FullLoop 53.9 and EstateSmoke woods 58.7 / p95 18.9. Copied-save smoke was skipped because the
aim blocker makes the package provisional. After the target fix, Integration reruns native/build/PIE,
corrected Development package/six suites plus copied-save smoke, then stages the supported Shipping
`-ReuseCooked` candidate and proves its PID has no TCP/UDP listener using isolated `-UserDir` save
round-trip/offscreen tests. Only an accepted Shipping candidate may retarget the Estate shortcut.

**Final rain loudness partial `734813ad`:** native 10/10, no UE build or delivery claim. Rain
`RainAudioGain` is multiplied by 0.5 linear (-6.02 dB): full default AmbienceVolume 0.7 becomes
outdoor 0.315 / indoor 0.1225. `FadeIn(2, 1)` remains, `RainAudible` start/stop threshold is not
halved, and indoor LPF/mute stay unchanged. The controlling user slider is **AmbienceVolume**, not
Effects; native tests cover the 0.5 ratio across rain, indoors and slider values.

It integrates only after Jenny exits and the editor is safe, together with the aim fix before the
corrected Development/Shipping package. It does not affect the current game.

**Integration validation batch (main-integrated/package-pending):** Integration's local `c9481e38`
evidence is now included in `main` `5d11ceed`; it is **not shipped** until the current night package
passes. The batch applied Water
applied Water no-pail `20dd9cd1` + `069e493a` and forage freeze `1010aa2e`; Props bramble
`1deb02ab` / `e37288ef` / `4d8bfecc`, sprint `7475b435`, Hoe hint `391f08f7`, Bed-v2 `aa375409`;
and Menu Store `1b2b97eb` + pail gauge `9d5da35d`. Native passes 9/9, forage-id Python 7/7, and
Editor/Game builds plus all exclusive PIE checks pass; editor is closed with zero UE. The accepted
delivery note must disclose that old canes-bed deconstruction refunds four Hay under intentional
current-cost compatibility semantics, not a save migration.

- Water prompt: no-pail/chest/empty/full carried states show the exact EmptyPailText; no-pail-anywhere
  remains native-only.
- Pail gauge: six=72 px, five=60 and two=24; F5 at 3 restores after watering to 2; one pail <=6 hides
  the Water tile, while excess 8 or two pails shows it.
- Store Map: counter/square says already there with no time change; Manor travel persists.
- Bramble `550200` at 283 cm focuses over weed at 125 cm and one worn clear removes three canes for
  1.2 Energy; Sapling `570143` clears on one click for 4 Branch +1 Kindling /1.5 Energy; Iron thicket
  has no animation/sound; F5/F9 cleared state passes.
- Sprint: run/walk changes about 0.03 Energy over 6 s, auto-off/refusal at <=10 and no resume at 50.
- Hoe hint is dropped from this local batch and remains chest-native-only. Bed UI reads 4 Branch +4
  Hay; old canes-bed deconstruct refunding four Hay is native-only intentional current-cost
  compatibility behavior. Forage-ID freeze remains source-only.

Pail footer `4545a8a2` is accepted as `d2c8b4b0`: native 9/9, Editor/Game build and PIE verify
`Pail (Carried) Water 5/6` footer with KBM and gamepad D-pad plus 60 px hotbar five-of-six state.
Menu pickup `b2a49e36` + `cb3f40c7`, weed clip and scythe remain excluded.

## 4 PM playable build

**[playtest] ready:** gameplay `9175e34b`, packaged from `main` `9a409e07` (including the
FullLoop berry-regrowth assertion correction). After Jenny quit and asked, Integration retargeted only
`Homestead Estate.lnk` to
`jennifergalley-literate-eureka\Build\Playtest-0929pm\Windows\SurvivalGame\Binaries\Win64\JennysHomesteadGame.exe`;
`Homestead.lnk` and the morning `Playtest-0929` build remain untouched. The latest Estate save
(`Manual`, 2:01 PM) carried over and is scratch-backed.

All packaged 1080p suites passed: Smoke 57.9 fps, Clearing 52.1, Hotbar 53.5, NativeMenu 56.6 and
FullLoop 54.5. `EstateSmoke` passed with zero material compile/errors, all six landscape points
rendering, and the pond default material/usage checks; manor measured 59.2 fps (p99 20.2 ms) and woods
60 fps (p99 16.7 ms). The build includes the north-west lake and map-label removal, rain gain fix,
Gait run, Menu toast/berry A-X Energy/Ctrl+wheel/wait changes, manor rubble and sprint, plus earlier
crop, river, performance and save work. Rain remains audibly unproven; listen outdoors on day 2 from
about 11:00 to 15:30.

## Evening playtest build

**[playtest] ready:** `Build\Playtest-0929eve` packages `main` `ca141b1f` / game code `a2607437`
plus test-only `6556c1fe`, and is on `Homestead Estate.lnk` with a copied 3:46 PM Manual save.
Integration retargeted only that shortcut with its existing icon/arguments; it never touched
`Homestead.lnk` or Jenny's live save. Native passed 9/9; six packaged suites passed: Smoke 56.9 fps,
Clearing 53.1, NativeMenu 55.2 and EstateSmoke woods 58.6 among them.

The fresh-game packaged FullLoop exercises the core loop, so it is now **deliverable**. Food PIE also
verified +5 Branch +1 Kindling, Hearth RoastedRoots at 2 Roots +1 Kindling, low-Energy berries without
a centre toast, and Roots `582129` gathering through F5/F9. The copied-save loop passed only in PIE,
not the packaged executable (desktop keystroke limitation); old-save Hoe and no-pail lake prompt
remain untested. Sprint still drains in this build, the night remains daylight-blue, and the
inside-store Map card still offers a false `23 min` walk.

Input-policy changes must run packaged **both Hotbar and FullLoop** suites, which caught the silent
stale assertions here. The playtest does not blanket-complete the separate partial bramble/sprint/bed/
Hoe-hint, Water prompt/generator, or Menu pickup/pail/store-card work.
## Core-loop priority after 4 PM

Jenny's next priority is a packaged, end-to-end first core loop, in this order: hoe/tool assembly;
manor rubble; early till/sow/water/harvest; easy live foraged food; lake pail fill; then town
buy/sell/return. **Equivalent-time fast travel is part of that core loop.** Integration owns a
packaged sandbox acceptance run for it. Music is postponed until this loop is accepted; unfinished
dye/Appearance and night-light work remain bounded partial work, not substitutes for the loop.

**Packaged core-loop acceptance sequence (pending):**

1. Start a fresh Estate game; open the standing-room chest, take the pail/branches/food, and find the
   billhook then hoe without duplicating rack rewards across F5/F9.
2. Till, sow, fill the carried pail at the lake, water, advance growth and harvest. The lake fill
   check also runs from the copied packaged save with recorded pail/water/Energy/focus/edge telemetry.
3. Gather live food in the estate/road corridor, eat it with immediate bounded Energy feedback, and
   cook roots at a lit hearth with exactly one Kindling debit per successful batch.
4. Use Town/Manor signs or Map travel to advance the equivalent safe walking time, buy/sell at town,
   then return; reject hunger/doze/max-hour unsafe candidates without partial state.

Every step needs the owning slice's native/PIE evidence plus a packaged sandbox pass. A failed step
blocks promotion of the core loop, not unrelated visual experiments.

**Integration copied-save PIE checkpoint:** `main` `1d1eebb5` (Water food/roots, Menu input and Map
travel) passed a copy of Jenny's 3:46 PM Manual save: four tilled plots; three distinct Turnip sows;
a named no-seed fourth attempt; berry beside soil LMB eat / F weeds-only / E sow; grow/harvest +2;
lake carried-pail LMB/E fill to six; Map Town about 7.5 h grounded; Pascoe sold one Turnip
10.87 → 11.07, bought seed → 10.87; Map return at 11:10 PM; and pail water plus sow/water state
survived F5/F9. This is **main-integrated PIE evidence, not packaged or shipped**. The no-pail prompt
and honest old-save Hoe path remain untested. Integration next adds Props food, then runs copied-save
packaged scratch and six suites; the inside-store map card's `X walk there 23 min`, bright
11 PM–1:30 AM night and long 7.5 h walk are nonblocking follow-ups.

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
| 582100-582299 | extra pickable forage in Estate woods and fields (Water; `Scripts\Terrain\berries_more.py` → `Simulation\HomesteadEstateForagePlacements.inc`) |
| *next free: 582300+* | *claim here* |

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
- **Road geometry:** C++ `EstateLayout` still has landmarks and polygons, not a road polyline. Native
  travel/sign/forage code uses generated `Simulation\HomesteadEstatePublicRoad.inc`, written by
  `Scripts\Terrain\public_road.py` from `estate_layout.json` road/roadProfile plus
  `EstateHeightfield.r16`. Do not invent another C++ path; regenerate the include whenever those
  inputs move.
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
| **Shipped in the 4 PM playable build:** approved lake in the owned geographic north-west (Jenny's map reads north-up, right=east, so her visual "up-and-left" overrides her spoken "NE") at **(-50, -745) m**, about **76 × 44 m**, with roughly 110 m from the farm edge and a ~90 m farm-side access path | Weather Agent (`89914e30`) | `df19d74a` merged as `main` `61c1595c`: Editor build/native 8/8/static-init clean and PIE verified actor load, peaty water/stony landing, carried pail fill/full, knee-deep wade wall, map lake and no `"Dirt road"`. Integration's packaged `EstateSmoke` passed pond M/MI default/material compile and heightfield-versus-Landscape visual checks; above-surface reflections/water visibility had already resolved the architecture winding false alarm. |
| **Approved:** a clearly signed manor-south gate/path with protected switchbacks or stairs to a **12–20 m dry beach** along the owned ~630 m cliff coast | Weather Agent (`89914e30`) | `author-fixed-cornish-estate-map`; `add-regional-landforms-water` |
| **Approved:** the wider walkable beach above (12–20 m dry width) below the owned cliffs | Weather Agent (`89914e30`) | `author-fixed-cornish-estate-map`; `add-shore-and-river-fishing` |
| The river looks as if it stops before reaching the ocean in Jenny's screenshot | Weather Agent (`89914e30`) | **Partly checked, not resolved:** PIE on `27e2e917` confirms the source, banks and water down to the beach, but the ribbon still ends a couple metres short of the ocean, separated by sand/foam. Own a later true estuary connection while widening the beach; do not mark the gap fixed. |
| **Pending:** around 9 PM the Estate visibly brightens and moonlight reads like sunlight | Architecture traces the nighttime directional/skylight/auto-exposure path; Water owns a later measured fix | Integration captures packaged **RT-on** evidence at 19:00, 21:00 and midnight. No fix is shipped or inferred from the trace. |
| **Pending:** town buildings are bunched too tightly | Water, after the final road route | The current 12 blocking blockouts occupy a 40 × 34.5 m four-sided square with 0.2–0.35 m adjacent gaps (about 0.9 m by the store); the main road ends ~72 m short. Make a ~60 × 45 m open square with terraces/cottages, 3–6 m side lanes, and a separate curved 5–6 m `townStreet`; preserve main-road 1.94 km chainage plus StoreDoor/Counter and Shop IDs/saves. |
| Running foot kicks too high toward her butt; lower its swing apex slightly | Harvest Agent / temporary Gait Agent (`65a2408b`) | `polish-locomotion-view-distance-and-time-hud` |

**Water-lane order:** the north-west lake shipped in the 4 PM build; its above-surface/reflection and
packaged material/heightfield gates passed. Verify the river mouth in the parked 4 PM river branch,
then stage **beach → route**. Night lighting and the town-road layout are separate later increments
after the north-west lake; town-entry/store acceptance, coordinate bridge, roadside forage and travel
signs wait for the final road route. If the terrain or water work needs placement ids, the Water Agent
claims them through this page before using them (the registry starts at 581000+).

### Forage placement ID freeze blocker

**Current saves are not corrupt; this blocks future forage/terrain/road rebakes and merges.**
`forage.py` compactly enumerates accepted new-Estate brambles, then Roots `582128+`, then successful
roadside stops `581000+`; its accepted brambles/roots span `582100+`. A future newly rejected
candidate would shift many later IDs, causing old harvested/cleared `ResourceEdit`s to attach to the
wrong kind or position despite `bakeVersion=2`. Water must preserve the committed id -> kind ->
position mapping for every existing `581000+` and `582100+` node, reserve per candidate with holes,
allocate additions only as new IDs, and prove old-save cleared/harvested mapping through a
save-regeneration regression before the machine permits another forage/terrain/road rebake.

**Water headless partial `1010aa2e`:** a pure-Python allocator freezes committed `.inc` rows,
rejects missing/malformed/duplicate/out-of-range entries, separates Estate/roadside reserved IDs
including gaps and retirement, and produces byte-identical current scratch output. Synthetic Python
tests pass 7/7; native `PublicRoad` passes 10/10 with all 59 frozen rows at id/kind/position 0.5 mm
and saved picked `582100` / `582128` / `581005` reload coverage. It changes no save version or current
save. Integration may cherry-pick only after the active UAT package; until then the rebake/merge block
remains in force and this is not shipped.

**Packaged lake pail regression:** the diagnosis is inconclusive; do not make a speculative shore-range
change. The shipped probe accepts lake shore <=120 cm, and PIE filled at landing (-79, -744) using an
emptied carried pail. Current world focus misleadingly offers `[A] Fill carried Pail` even with no
carried pail (the starter pail remains in its chest), then errors `Carry your pail`; a full pail, low
Energy or being >1.2 m off bank are other possibilities.

Water and Integration reproduce the same flow only against a scratch copy, never Jenny's live save.
The unique copied-save packaged run records pail/water/Energy/focus/edge distance/actor tag plus A/E/RT
input and toast output, then becomes a core-loop acceptance test. Its copied-save **PIE** precursor
passed at fill six; no-pail prompt coverage remains untested.

**Pail state core-loop requirement:** the first safe slice keeps positional v12/v13 `Item::Water`
stock internally (pack, chest and drops; max 1200; `TakesSpace=false`) rather than silently migrating
or discarding it. `FillWater` tops pack Water to six and `Water(plot)` spends one; pails are currently
fungible and may be multiple. For an ordinary one-pail case, hide the inventory Water tile and show
`PailCharge` 0..6 with oil-lamp-style hotbar progress. Excess above six and Water in chests/world stays
accessible and visible as legacy reserve; pail transfer/drop preserves charge atomically.

Props owns core compatibility and Menu the UI. Cover v12/v13 plus 0/1/6/7/1200, multiple containers,
no pail and round-trip cases. Stable per-pail IDs/tagged migration is a separate large follow-up.
Prove packaged lake fill before calling the loop accepted.

**Menu pail gauge partial `9d5da35d`:** native Release 9/9 (economy 14 / scenario 573), 1080p PIE
only, not ready or shipped. `Homestead::PresentPail` renders carried-pail hotbar Fill as
`min(Water, 6) / 6`, blue when charged and red when empty. With exactly one pail and pack Water <=6,
it hides the pack Water tile/detail as `Water N/6`; zero/multiple pails or overflow >=7 retain the
tile. Chest/drop Water always remains visible; stowing/dropping the pail reveals the pack tile. v12/v13
positional-stock/save mechanics remain unchanged.

PIE verifies hotbar full blue 6/empty, pack 5 Water hidden and legacy 9 tile/full bar. Missing:
hover `Water N/6` detail, 6→5 watering, chest stow/drop and F5/F9 in PIE (native covers them).
Cherry-pick after the `a4bb831f` travel test hunk; it is independent of the pickup popup. Integration
now owns editor/UAT to cherry-pick this, Store Map and Water's pail-prompt pair for a bounded next
package only after targeted checks. The misleading fill prompt is unchanged here.

**Pail hover / pickup helper splits:** original mixed `8cebcc84` and Menu's main branch are preserved;
no amend/reset occurred. Both new branches are partial, not ready or shipped:

- **Pail footer `4545a8a2`:** based on `main` `8c671873` plus pail `6b46a788`, native 9/9 and
  clean merge tree with Integration local `c9481e38`, no UE compile. It adds
  `PailChargeLabel` / `FHomesteadRow.Status` / `PackHint` footer so mouse and gamepad see
  `Water N/6`; gamepad lacked the grid-only hover tooltip by design. Integration must compile and
  PIE-check it after the current editor closes.
- **Pickup helper `acd012f4`:** based on `main` `adef3a56` plus pickup `ae584b5a` and quiet
  `79789c9a`, native 9/9 (economy 14 / scenario 639) across berry/buy/craft/drop+pickup/Water/chest
  transfer. It moves the gain helper to `Simulation/HomesteadHoldings.h`; revision and pack+owned
  stock logic is not itself buggy. UE compile and 720p/4K visual proof remain required.

**Water pail-prompt partial `20dd9cd1`:** native 10/10, no UE or delivery claim. `FocusActions`
shows carried empty fill / full pail full / chest pail take it / no pail requires one, and
`FillWater` distinguishes in-chest, none, far-from-freshwater and full without changing mechanics,
save or reach. The branch is synced through merge `4dcd3898` (lake trail preserved; `beach_belt`
untracked).

Before Integration can cherry-pick, replace Water's proposed EmptyPailText with Jenny's exact:
`The pail is empty. Fill it at a body of water`. Far refusal may clarify river/lake, but no-pail
focus also needs PIE coverage. This remains a pending text correction, not shipped.

**Corrected pail-prompt pair:** `069e493a` atop `20dd9cd1` now has the exact EmptyPailText and
native 10/10; far refusal remains truthful about river/lake and excludes sea. It is still partial:
no UE/PIE, not in package `a2607437`, and Integration must cherry-pick only this pair after packaging
(not Water's unrelated night/lake-trail work).

**Turnip planting regression:** Menu input is main-integrated in `1d1eebb5` and the copied-save PIE
checkpoint passed three distinct Turnip sows, then a named no-seed fourth attempt, with sow/water state
surviving F5/F9. Keep the copied-save **packaged** run as the remaining core-loop gate.

**Food and Cooking Kindling:** **main-integrated/package-pending in `a2607437`, not shipped.**
Props' branch was native Release 9/9 before merge. `CanEat` now permits a full-Food/low-Energy benefit
while both-full refuses. RoastedRoots uses 2 Roots + 1 Kindling; HerbedRoots uses 2 Roots + 1
MeadowHerb + 1 Kindling, at a lit Hearth/fire through canonical `CraftChange` / requirements /
`AssessRecipe`. One Kindling debits only for a successful cooked batch, never for failed/canceled
recipes, other crafts or fire fuel.

Branches hand-gather as 5 Branch + 1 renewable Kindling (24-hour regrow); source text is `Fallen
branches, saplings and old boughs`. Native coverage includes failures, no stock, fire, Energy, v12
reload and woodland/Estate. Water adds woodland roots because the first existing live patch is about
222 m away. Integration still owns UE compile, suites and PIE hearth/lit-fire exact-debit/woodland
recipe verification. Food success still returns `Ate Berries: Energy +6`; combined testing with Menu
`60c6d6ba` must confirm the centre success toast is suppressed while the bar popup remains.

**Priority:** these playtest items take precedence over the ordinary round-2 feature queue. Each row's
own status is authoritative: shipped work remains historical evidence, while pending and
main-integrated work is not player-shipped until its stated gate passes. The orchestrator assigns an
implementer slot before any owner starts hands-on work; no one edits a busy lane's files or starts a
fourth implementer.

## Open blockers and known bugs

### Final planning OpenSpec decision

The current farming artifacts reflect the final 2026-09-29 decision, superseding `b82eec53` and tiered
`e2159ea2`: flat **3 game hours** of Well fed at ×0.85; Meals +25/+40/+60 Energy; a full-energy
≥1-hour extension guard; and `CorruptSave` for non-finite or `hour + 3`-exceeding expiry. New Estate
games default to 60-minute days, while Settings retain 30/60/120 and existing saves retain their
stored value. Period dishes differ in Energy restoration but all grant the same flat 3-hour Well fed
duration. Calendar must not implement stale `b82`.

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

- **Field book notification overlay** — **Menu `88180744` shipped in the 4 PM playable build.** The
  open-book hoe-craft notice has zero 4K layout-pixel difference and long words wrap.
  It replaces the menu-reflowing banner with the brief floating non-modal overlay: it auto-disappears,
  preserves focus/input, uses a Victorian licensed font with larger centered text and a compact
  content-sized frame.
- **Unified hints and toasts** — **Menu/UI ownership note; pending and not shipped.** Architecture
  traced separate current channels: unconditional top-left controls, `DrawInteractCue` focus text,
  world Canvas toasts and the native-menu parchment NoticeCard. Unify transient world feedback and
  persistent focus actions in a top-centre parchment visual through shared style, not a duplicate UI;
  retain separate error-over-success priority and device-specific glyphs without focus theft.

  Show top-left controls only for the first 60 visible real seconds after boot, new game or
  `ResetActionHints`, freezing that timer in a paused book/shop. Preserve retirement after three
  successes in `GameUserSettings::ActionHints`. Cover Feedback/Prompt/NativeMenu, 720p/4K, controller,
  pause timing and focus behavior.
- **Pickup gain popup** — **Menu acceptance `04959978` is main-integrated in `23aba36a`, but excluded
  from the next aim/rain/Shipping package and not shipped.** Based on common Simulation revision gains
  across pack, owned chests and drops, it suppresses moves/reloads and Water, then presents a
  brass/cream `+N` right of the projected chest for 2.6 visible seconds (maximum four) while
  book/shop hold. It removes `Selected quantity stored/taken`, garment and drag success notices;
  errors remain.

  Native passes economy 16 / scenario 670 and Editor build is green. 1080-class PIE verifies a real
  BerryBush `+5 Berries` beside the heroine from 0.8–2.6 s, gone at 3.5 with no toast; crop harvest
  `+4 Roots` and `+2 Seeds` separately with no toast; two quick `+3 Branch` grants combine `+6`;
  three Water gives none; legacy 9 Water tile remains visible and the pail bar is full. Earlier craft
  `+1 Axe` also has no toast. Drop/chest/shop-buy plus 720p/4K remain unverified; Menu closed its
  editor before Jenny's game startup rather than attempting a 4K capture under memory pressure.
- **Zero-stock hotbar seed/food items** — **Menu plus Props Simulation, pending and not shipped.**
  `HotbarSnapshot` currently preserves a pinned item and icon even after `Sim.Count(pack)==0`, making
  planted/stored turnip seeds and strawberry runners look available. Hide zero-count consumable
  seed/food visuals and actions while retaining optional pin mapping for reacquisition; Props guards
  against any zero-stock implicit fallback. Cover sow, stow, F5/F9, old-save pinned zero,
  reacquisition and no accidental planting.
- **Field-book hotbar editor** — **Menu headless source slice, pending and not shipped.** While the
  Inventory field book is open (the world hotbar is intentionally hidden by
  `Controller::ShouldShowHotbar`), show all 10 saved `HotbarSlots`. Drag a carried tool/item to an
  exact slot and drag slot-to-slot to reorder; reuse the menu's pointer drag grid and virtual
  controller drag, with a pad alternative.

  Assignments are references, never stock moves: replacing an occupied slot swaps; a move clears the
  source; bindings stay unique; cancel changes nothing; chest rows must transfer to pack first. Preserve
  old saves and depleted pins. Require native assignment/save-uniqueness coverage, then PIE 720p/4K
  mouse and controller proof. Menu works headless during Jenny's PID 2328/memory hold; no UE/UBT.
  This remains behind the aim/rain/Shipping path.
- **Human-readable save confirmation time** — **Menu, pending and not shipped.**
  `Controller::MenuSaveStatus` currently shows an ISO-like UTC timestamp. Present it as a localized,
  human-readable local date/time (for example, `September 28, 2026 12:01 PM`) without changing the
  saved UTC timestamp or applying an incorrect time-zone conversion. Cover unknown time and 720p/4K.
- **Minimap/compass HUD trial** — **pending behind the core loop, not shipped.** The current minimap
  is 220 logical px / 120 m crop; far badge radius 8.5 U, near 11 U and glyphs 10 U become about 5.7
  physical px at 720p (`UiScale` 0.667), while 4K caps at 1.5. Enlarge important glyphs with a minimum
  physical-pixel radius, collision spacing and pixel snap, preserving the crop. The current
  `MapPainter` custom verts disable subpixel snapping; Canvas rescales the clock to logical 42, while
  money's native Slate Bold 20 remains crisp.

  Render time through native Slate `SHomesteadHudScale` without duplicate Canvas time, leaving the
  dial. Retain the minimap while trialing a top-centre compass with N/E/S/W and selected Map landmarks
  from `MapComponent` frame/player/camera yaw and `MapGeometry` bearing (+X north/+Y east), with no
  per-paint actor scan. Avoid top-centre parchment toast/calendar and the first-60-second controls;
  collapse at 720p if needed. Validate 720/1080/4K, rotation, map/controller overlap and crisp still
  text.
- **Remove Guidebook UI** — **Menu, pending and not shipped.** Remove the Guidebook surface entirely,
  updating navigation cycles, shortcuts, page indexes, NativeMenu coverage and packaged checks while
  retaining save compatibility.
- **Appearance controls and naming** — **Long bob `61059621` is ready; orbit remains local-only
  partial and unshipped.** `f63ba148` + `b77ada68` implement front framing plus mouse/WASD/right-stick
  orbit without movement, and camera collision restore on close; those passed PIE. The Slate wheel-zoom
  fix only builds: it still needs PIE near-wall and 4K checks before camera work can be reviewed or
  merged.
- **Build menu wording** — **Menu, pending and not shipped.** Trace the current Build-menu `Plan`
  action and relabel/adjust its semantics so the control accurately describes what it does. Keep this
  separate from the active dye/Appearance verification work.
- **Hair groom** — **temporary Gait Agent** (`65a2408b`), after the running-heel and scythe work:
  investigate the intermittent exploding/sticking-out groom.
- **Gait and scythe** — **temporary Gait Agent** (`65a2408b`), after its current run-heel slice:
  lower the running foot swing apex slightly; at rest the scythe must sit in her hand, and its blade
  must not clip the terrain during a sweep.
- **Tool feedback and clearing animation** — **Props, pending and not shipped.** An iron-billhook-tier
  target must be non-actionable with no animation, Energy spend, progress or yield when the wrong tool
  is selected, and give a clear needs-iron hint. Make billhook swings visibly contact their targets.
  Correct the pickaxe's upside-down idle grip; one tap or hold on a rock triggers the complete
  two-swing clearing animation and awards/clears once, without a second click or double reward.
  Architecture traces tool tier, input and reward paths before native/PIE proof.
- **Gather and scythe feedback** — **Props, pending and not shipped.** Remove generic slight-knee-bend
  gather routing: solid pickup uses the existing Stones kneel; bush/plant pickup uses the existing
  Berries/Roots hip-pouch animation; preserve specialized reeds/tree behavior and held-prop contact.
  Replace the scythe cue with an original or verified CC0 airy grass/steel `shhhhnk`, timed to the
  blade pass.

  **Scythe audio partial v2 `4818a3b0` + fallback `3383e29e`:** native 9/9, no import/audition/UE
  build/package or shipped claim; do not use conflicting v1 `9e119415`. Original project-authored
  `Assets/Audio/Effects/ScytheSwish.wav` is deterministic `Scripts/generate_scythe_sound.py` NumPy
  seed 7307 output (0.72 s, 48 kHz/16-bit mono, peak -6 dBFS, RMS -22.2): airy 1.3–3.6 kHz band noise,
  stem clicks, grass rustle and faint damped steel partials, with no third-party audio. Bootstrap
  imports `/Game/SurvivalGame/Audio/Effects/ScytheSwish`; mowing plays it once per successful
  `mown > 0` sweep. Missing cue now logs bootstrap import guidance once and remains silent - it must
  never fall back to CC0 `GrassStepA` footstep audio. PIE still needs cue count at 30/60/120 fps,
  miss/cancel, rain/music mix/headroom and cooked asset proof. Add provenance to `docs/asset-credits`
  only when shipped.
- **Bilateral ground-pull and sapling action count** — **Props, pending and not shipped.** By-hand
  Resource Weeds/Nettles already resolve in one `Sim.Harvest`; replace right-knee-only
  `KneelGather(Pouch)` with a dedicated bilateral kneel: two hand grabs, left/right toss behind,
  rise, and one final-contact commit (cancel free, no double stock). Garden `Sim.Weed` stays instant
  and yieldless.

  **Props bilateral partial:** `887a2dfb` adds Control Rig recipe
  `Content/Python/homestead_agent/kneel_pull_weeds.py` (150 frames at 30 fps; both knees, two low
  bilateral pulls, L/R toss, rise, 3.4 s contact; grip/knee/torso-clearance diagnostic report).
  `f3584fa3` appends PullWeeds routing with skeleton guard and
  `ControllerWeedPull.cpp` preflight/copy: one actual Harvest/Weed at the second root, cancellation
  before beat with no stock/Energy, resource gain versus yieldless garden behavior, and F/X never sow.
  Native 9/9 passes; there is no UE script build, bake, `.uasset`, Character Lab or PIE evidence.

  **Visual follow-up `23289e9c`:** native 9/9 on `887a2dfb` + `f3584fa3`, no UE/UBT or clip bake.
  Two transient no-collision/no-shadow handful props, derived from the clump mesh/garden NettlePatch,
  appear on pulls, follow hands, toss left/right behind, tumble and hide on end/cancel. The first pull
  is ThinResource 55%; cancel restores it; the final second root is the one Sim commit, with no save
  change. This is excluded from the current pail package.

  Editor/Game compile, `kneel_pull_weeds.build()` / report(), MetaHuman Character Lab, and PIE still
  must cover grip/knees/toss, clump cancellation/one stock-Energy, garden F5/F9 and final-contact
  transaction. Tossed props currently vanish abruptly at clip end; inspect that before polish. The
  missing-asset fallback may preserve old behavior only during development and must never ship absent
  the authored asset. It follows `blender-assets`, uses `E:` scratch, and stays out of UE/UBT while
  Integration owns the editor.

  Keep tool-kind rules narrow: Weeds/Nettles are one hand-or-Scythe action and Billhook is wrong tool;
  `BrambleThin` with worn Billhook is one; only the common worn `Sapling` requires two logical swings.
  Its existing `MacheteHack` already has blade strikes at frames 20/37 but commits once at the second
  (1.25 s), causing the extra click. After food work, make worn Sapling one logical clear per clip:
  3–4 Branch + 1 Kindling, 1.5 Energy, tier rules preserved, and update Hotbar tests. Keep a standing
  two-strike hack or dedicated standing hook for tall rigid saplings/billhook work; never reuse the
  soft 40 cm one-knee reeds saw. Iron+ `BrambleThicket` (3/2/1/1) and Steel+ Bank (4/3/2/1) gates stay
  unchanged. For the pictured missing bramble prompt, obtain actual `FocusId`, kind and toast before
  declaring this a root cause.
- **Hoe/pail exact target** — **Water garden-outline partial `14cce349` + `9b69eca6`, not shipped or
  in the current aim/firewall package.** Native 10/10 and Editor/Game builds pass. Side-effect-free
  `CheckTill` / `Weed` / `Water` now feed both `PreviewGarden` and the mutators, with block errors
  matching mutations; 50 repeated previews are byte-identical under Serialize.

  Hoe previews its exact 85 cm GardenCell; Pail previews the focused 60 cm plot. The world visual is
  transient sampled-ground 4x4 thin 2.5 cm-raised segments at 100 cm, no collision/shadow, green/red,
  and hides for book/shop/planning/failed/water focus with no save IDs. PIE still must cover slopes,
  turned soil, adjacent plots, blocked cells, pad/HUD behavior and z-fighting before readiness.
- **Weedy crop-plot prompt** — **Props headless source slice, pending and not shipped.** A weedy
  planted crop plot must offer `[F] Pull weeds` / gamepad `[X] Pull weeds`; F/X must never sow.
  Props is extracting the focus copy/condition from current main and adds native planted/mature/clean/
  tool-state checks. PIE after Jenny's game must verify the hint and action on the actual plot.
- **Long/tousled hair at angles** — **temporary Gait Agent** (`65a2408b`), while testing in its
  editor after the run rebake and before sprint: investigate the screenshot's disappearing hair and
  side patches at some camera angles. Pending; not shipped.
- **Sprint toggle** — **Menu `af7831b1` shipped in the 4 PM playable build.** Commit
  `8e0516a0` toggles sprint with L3 or a released Shift tap: Shift+Q/click does not toggle, work/book/
  shop pause speed while preserving intent, and load/new/retry/teleport reset it. At <=10 Energy it
  gives a notice; exhaustion disables sprint. Native 8/8 plus economy 12 / scenario 521 checks and
  Editor and Game builds/static-init pass. Integration's PIE verified Shift tap 480 cm/s, second tap
  210 cm/s and the corrected hint text. The hint lacks a standalone 4K capture; packaged NativeMenu
  and Hotbar suites passed.
- **Sprint Energy cost** — **final product direction, pending and not shipped.** Sprint has **zero**
sprint-specific Energy cost; this supersedes both current 0.35/real-second behavior and the tentative
0.05/s/regen proposal. Baseline awake time drain remains -0.6/game-hour and ordinary work costs remain.
Refuse the sprint toggle at Energy <=10 and turn it off if other work/time reaches that threshold; do
not auto-resume after recovery.

**Props partial `7475b435`:** native 9/9 on `jennifergalley-sprint-zero`, based on `4b8d6edd` and
cleanly merging `main` `4463086d`, not built/PIE/packaged or shipped. It removes the 0.35/s charge,
uses `Sim.CanSprint(Energy > 10)`, toggles off at <=10 with no auto-resume, and leaves speed/awake
drain unchanged. Native coverage spans 30/60/120 FPS and day lengths, floor/refeeding/work/reload, plus
Hotbar/Creek/Visual routes. Integration's UAT lock on `a2607437` remains ahead of UE validation.

The bramble-on-food `42a63b8f` conflict is separate from sprint; Props rebases it only after the
package and Integration confirmation. Props' next headless slice is a separate `NoHoeMessage`
chest/drop blade-location hint. Starter food and abundant berry requests remain pending, so this does
not claim early Energy is fully solved.
- **Live sound sliders** — **symptom investigation pending, not shipped.** Mouse drag already calls
  `MenuPreviewAudioVolume` live through `SSlider.OnValueChanged`, then release writes INI; d-pad steps
  preview and persist. Jenny's symptom may instead be effects without a continuous audible source,
  music silence, or a broken cancel path: Esc/B/book close during drag does not roll back,
  `OnMouseCaptureEnd` always commits, and keyboard/pad-origin sliders may preview without committing.

  Menu first reproduces an audible component path. The later bounded fix snapshots transactional gains:
  preview while changing, Confirm commits, and Cancel/focus loss/book close restores every old gain
  without saving a drag. Consider a one-shot Effects preview and avoid reintroducing gain-squared
  behavior; verify at 1080p and 4K.
- **Contextual hotbar eating and berry feedback** — **Menu `88180744` shipped in the 4 PM playable
  build.** Menu core-input pair `60c6d6ba` + `a874d260` is main-integrated in `1d1eebb5`, not
  packaged or shipped: native 9/9, Editor build and PIE verify F/X weeds without sowing, A/E seed-selection
  refusal when empty, no Turnip fallback after depletion, selected berry A/E sow, and five rapid berry
  taps consuming five during the chew. Berries remain edible beside tilled soil despite plant focus.
  The first planted plot still holds focus until squarely at the next plot; woodland UE suites remain
  unrun.

  **Next Menu slice:** show the bar popup without a duplicate centre `Ate Berries +Energy +0Food`
  success toast, preserving errors; remove the `Selected quantity stored` copy. No UE while
  Integration owns the slot.
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

  **Post-core-loop work:** a dedicated music implementer auditions the four CC0 candidate recordings,
  selects/imports only the qualified set with credits and catalog entries, and adds the shuffle with
  long ambient-only gaps. Do not use the untracked Kevin MacLeod CC-BY files. Register
  `MusicShuffleBagTests` in CMake and verify packaged multi-track load and run.
- **Context hint** — **Menu `88180744` shipped in the 4 PM playable build.** Plain wheel
  cycles the hotbar; Ctrl+wheel zooms while the book has focus.
- **Weed visibility and grounding** — **Props `[ready]` `8418aa8e` for post-4 PM integration only;
  not in the 4 PM package or shipped.** This extends `889cfde8` with the original imported
  `WeedClump` asset and a pivot-seating correction. Day Estate PIE renders all 157 original clumps
  (dock 57, thistle 54, dandelion 46); every Landscape-trace pivot sits 0–10 cm below ground
  (2.7 cm median, including 36% slopes), and prompts appear only over drawn clumps. Native 8/8 and
  Editor compile pass.

  Dusk PIE (19:30–20:10) shows visible rosettes and their prompt; pulling dock changes the drawn count
  156 → 155. F5/F9 clock rewind confirms the pulled clump stays gone while an untouched neighbour
  remains. This completes the prior dusk and PIE reload debt. At 4–7 m in tall grass the weeds still
  read visually modest. Props closed its editor at 13:10; the orchestrator verified no Unreal
  processes remain.

  **Screenshot diagnosis correction:** the bare, leafless ~69 cm arching canes near a fence/grazed
  125 cm ring are tentatively spring `SM_BrambleThin` overgrowth, not the green tuft/small-leafy
  thimbleberry visual used for ordinary weeds. `WeedClump` replacement `cc5b115d` will not fix this
  look. Fence live nodes in `HomesteadEstateDisrepair.cpp` IDs 550001+ are worn-billhook
  `SM_BrambleThin` canes. The concrete candidate mismatch is controller focus choosing the nearest
  **centre** inside 280 cm without gaze, versus `Overgrowth::Reach=300 cm` and a forward 80 cm /
  200 cm-radius swing probe: a dead-ahead 285 cm cane can be in Simulation reach but show no prompt or
  swing target, while a nearer weed/grass steals focus.

  **Props rebased headless partial:** branch `jennifergalley-bramble-focus-main` replays the three
  commits as `1deb02ab` (forward-biased 300 cm aim focus/swing target), `e37288ef` (worn Sapling one
  logical clear with two physical blows), and `4d8bfecc` (quiet under-tier refusal). Native 9/9 covers an aimed 285 cm
  cane versus nearer weed/grass, behind/301 cm refusal, scythe choosing weed, gated iron thicket,
  save, and one worn-Sapling 3–4 Branch + 1 Kindling / 1.5 Energy yield once through reload.

  Old branches remain preserved; no reset/force occurred. Props' trial merge of current main,
  bramble-main, sprint `7475b435`, Hoe hint `391f08f7` and Bed-v2 is clean/native 9/9. Required
  before `[ready]`: UE/editor, packaged validation, PIE at 285 cm, a two-blow/one-logical sapling,
  silent under-tier refusal and F5/F9. This remains a plausible cause, not proof for Jenny's pictured
  cane.
- **Manor rubble** — **`53fe97d5` → `9ecb08ad` shipped in the 4 PM playable build.** Clearable
  slate heaps and granite/hall cobbles use reserved placement
  IDs `582000–582099`. Integration's PIE cleared slate `582001` with E/A (pack 102 → 104, mesh gone);
  rubble `582008` requires the pickaxe then two swings for Stone/Scrap iron. F5 before clearing
  `582011`, then F9, visually restores only `582011`; `582001`/`582008` remain gone. Native 8/8,
  Editor and Game builds/static-init pass.

  Packaged suites passed. Focus can select nearby bramble/weed and resource reach can pass through a
  wall, matching current nearby-resource behavior. Weeds `8418aa8e` remains post-package work.
- **Loose manor timber piles** — **Props, pending and not shipped.** Make large loose timber piles
  clearable like the shipped slates using appended stable debris IDs only: no existing ID shift, old
  saves preserve their state, and heritage structural support remains untouched. Require PIE F5/F9
  clear-state proof.
- **Rusted hoe wayfinding** — **main-integrated `a785a417`, not packaged or shipped.** The salvage
  order is billhook → hoe → axe → scythe → pickaxe. Tilling without a hoe directs her to search the
  old manor; the journal/guide points to the west rooms by the chimney. Fresh-game PIE verified
  `520001` billhook then `520002` hoe, crafting/tilling, and F5/F9 search flags. Native 9/9 covers
  reward order and old saves. Honest old-save PIE has not separately run; Integration's copied-save
  packaged core-loop test remains the gate.

  **Props hint partial `391f08f7`:** native Release 9/9, clean off `afe57121`, no UE/PIE or delivery
  claim. `NoHoeMessage` now searches owned Hoe/HoeBlade in pack, chest and ground before salvage-rack
  guidance, giving the nearer location, distance and action; old-save chest blade/rack-absent cases are
  native-covered. Till refusal state, save and quantities stay unchanged. It is excluded from package
  `a2607437`; Editor compile and PIE still gate it.
- **Energy and food balance** — **Calendar Agent** (lane A, task 1.3): the chosen direction is one
  visible **Energy** meter later, rather than a visible hunger-plus-energy pair. Keep serialized hunger
  compatibility; revise gentle-hunger penalties into energy/food balance and modest **Well Fed** meals.
  Planning traced the present state: Hunger starts at 85, drains 2/hour awake and fails at 0; Energy
  starts at 100 and drains through work; food restores both. The change is **pending, not in today's
  4 PM build**; Planning updates the OpenSpec spec.
- **Food Energy affordance** — **pending, not 4 PM content.** Before purchase, shops and Inventory
  hover show the canonical nominal `+N Energy` for food. Remove only the redundant stack number from
  hover text; retain tile quantity and controls. **Menu** owns the UI after active dye/Appearance work;
  source values from `ItemInfo` so future Energy-only lane-F values flow through automatically.
  Require native coverage and PIE checks at 1080p and 4K.
- **Whole-number currency** — **agreed design, pending and not 4 PM content.** Preserve the current
  `int64` raw values and save bytes: semantically relabel the smallest stored unit as one whole
  `coin`, with **no numeric x100 migration**. Thus raw 1000 (formerly $10) becomes 1,000 coins and
  raw 40 (formerly $0.40) becomes 40 coins; buying power and saves remain unchanged. The canonical
  `FormatMoney`/`Delta` renders grouped whole integers with correct singular/plural and no `$` or
  decimal anywhere. Reference raw prices remain pasty 80 base / 100 shop, bread 40 / 50 and cabbage
  seed 32 / 40; the v12 fixture remains raw 2234 coins.

  **Props** owns the future simulation/economy semantic field/type rename and `GrantMoney`
  cap-overflow guard after an explicit slot; **Menu** owns the coordinated shop/HUD/toast/UI formatter
  slice. Avoid a half release. Validate Economy, Lamp, legacy v12/v13 saves, raw 0/1/`INT64_MIN` and
  cap behavior; package a 720p/4K purchase such as 1,000 → 900 coins for a pasty.
- **Weather recurrence** — **Water Agent** (retained lane; supersedes the broader Calendar proposal):
  rain every third day is too frequent. The smallest traced change is a stable hash selecting offsets
  **1 or 2** and **6 or 7** in every 10-day block: exactly 20% rain, 4–6-day gaps and day 0 dry. Keep
  the current 09:00–15:00 rain window, overcast, moisture and audio behavior; no seed or new save
  section. Old saves' forecast can change, while accrued plot moisture persists; document that at
  implementation. Tests cover count, gaps and save/reload. **Pending; not shipped.** Calendar retires
  after its lane-A work.
- **Starter chest and wardrobe** — **main-integrated `a785a417`, not packaged or shipped.** Fresh-game
  PIE verified the standing-room chest's pail, four branches, 3 pasties, 2 bread and seven garments;
  the tunic stays worn. It runs only in `NewEstateGame`, never restocks on load and uses normal chest
  capacity. The original 2197-placement table hash is pinned, excluding only appended rack `520006`;
  F5/F9 keeps the rack unsearched with the next axe head and does not duplicate the hoe. Native 9/9,
  Editor/Game builds and static-init check pass. The copied-save packaged core-loop test is still
  pending, so do not call this shipped.
- **Clean bed recipe** — **Props, pending and not shipped.** Jenny rejects thorny bramble canes in a
  bed recipe. The live `Piece::Bed` cost is 4 Branch + 4 BrambleCanes (the crafting progression doc's
  retired Fiber text is also stale). Props replaces it with **4 Branch + 4 Hay**: the same eight units,
  with Hay from TallGrass using a worn scythe at 1–2 per tuft and no iron-tier upgrade. Existing built
  beds plus v12/v13 saves, Piece IDs and Item IDs stay unchanged; other cane recipes stay unchanged.
  **Bed-v2 partial `aa375409`:** native 9/9, one commit off `main` `46daf9dd`, with exact
  requirements and v12 built bed/canes unchanged in saves. `BedrollTakesHay` covers missing-Hay refusal
  and heritage; FullLoop has Hay but UE pack total is unverified. Compatibility tradeoff: deconstructing
  an old canes bed refunds 4 Hay because normal current-cost deconstruct has no per-instance material
  provenance - this is intentional, not a stock migration. UE/editor/package proof remains pending.
  Update the canonical progression doc only after code lands.
- **Starter rack placement save safety** — **Props urgent implementation guard; not shipped.** A new
  rack at placement ID `520006` must append after every existing placement section, not insert into an
  earlier numeric range and renumber later `550xxx` saved resources. Before `[ready]`, require a
  byte-identical old-table regression proving no existing placement IDs move; do not mutate user saves.
- **Owned chest names and original trunk** — **Props simulation/save/world with Menu rename input,
  pending and not shipped.** An owned chest receives a persistent stable-ID custom name, shown in its
  world interaction prompt before opening. Author an original high-fidelity Victorian timber trunk;
  Coral Island is mood reference only, never copied. Architecture traces the name/save path before a
  separate increment and native/PIE persistence coverage.
- **Auto-store matching stacks** — **Menu plus Props Simulation, pending and not shipped.**
  Architecture specifies one atomic storage-only transfer API: move only carried items matching stacks
  already in the **current** chest; exclude equipped and unmatched items, respect capacity, permit a
  partial transfer without loss, and persist it. Menu exposes a Storage-only T/button shortcut (add
  gamepad Y only if it is safe). Cover capacity, partial/no-loss and old-save behavior in native and
  PIE tests.
- **Safe manor construction** — **Props, pending and not shipped.** The estate parcel is already
  owned, but `Manor::BlockedByManor` blanket-rejects nine footprint samples inside
  `ManorFootprint`, except furnishings on the heritage standing-room floor. Define a safe roofless-hall
  subpolygon with full-footprint and capsule margins that excludes heritage walls/masonry, then permit
  chests, beds, fires and own foundations there.

  `ResolvePlacement` already snaps Wall/Doorway to a new foundation edge and Roof to its cell, while
  `CheckSite` requires a foundation and rejects duplicates. Allow a physically clear 3 × 3 m new
  foundation cell only after excluding the ruin cross wall (U1800), rubble (U2150/V1100) and a retained
  walking corridor; then validate wall edges and doorways against heritage mesh even across building
  IDs. Narrow `Manor::BlockedByManor`'s current broad exemption for any non-Foundation on the room
  `buildingId` to an actual heritage foundation inside the standing room; hall extensions use the
  measured safe-zone predicate.

  Later walls/roofs may snap only to a nonheritage new foundation with collision/segments, never
  replace heritage fabric. Controller green preview and `Place` share a core `CheckSite`; validate
  saved nonheritage structures after heritage/parcels deserialize and preserve old saves. Require
  native plus on-foot PIE path, collision and save tests. Architecture is still checking whether the
  wall/roof follow-up is viable; the packaged core loop (including fast travel) remains ahead, with
  music postponed until that acceptance passes.
- **Road-to-town forage** — **Water Agent** (`89914e30`): add pickable berries and herbs along the
  road to town, including the bridge approach, and significantly increase visible pickable
  berries/herbs/non-farm food across the estate distributions. The ID range is reserved; implementation
  remains pending the narrow public-road-corridor proof and bridge coordinate sync.
- **Abundant live berry bushes and non-farm food** — **Water, pending and not shipped.** Populate
  Estate woods, fields and the road with measurably abundant **live pickable** bushes, rather than
  decorative-only foliage. Architecture's baseline is about 104 live `BerryBush` nodes (22 normal
  woodland, 43 MVP and 39 of 42 special; 3 skipped), five berries per harvest and 36 game-hour regrow,
  with no current season gate. The 374,654 scenery instances include many decorative berry-looking
  bushes and do not count.

  Only 13 live nodes sit within 15 m of the 1.94 km road: 11 in its first 400 m, two from 400–800 m
  and none beyond 800 m. Water's first targeted increment adds about 25 live bushes: 14–20 roadside
  stops from chainage 800–1940 at 55–80 m intervals, plus 8–12 woods/field edges beyond the first
  150 m. Preserve existing IDs and player edits, use reserved `581000–581099`, and measure visibility
  and performance. Roots/herbs provide off-season food.

  The old Seasons handoff would gate Blackberries from Summer 15 through Autumn 28; do not silently
  add that gate without spring food and clear player prompts. Acceptance is live-node density,
  seasonal readiness/regeneration, save safety and performance - never decorative instance count.
  Water's `fc758da4` (+28 live BerryBush: 18 woods, 10 hedges) followed by `01bda38d` (+16 Roots
  near the manor, three within 110 m) and 15 roadside bramble/herb/root placements at `581000+` are
  main-integrated in `1d1eebb5`. Old placements stay append-only; native 10/10 covers old save,
  picked state and regrowth. Integration's copied-save PIE grew/harvested +2; package scratch/suites
  remain pending, so it is not player-shipped.
- **Terrain-following road grade** — **Water Agent, pending and not shipped.** Eliminate artificial
  raised/lowered road segments. The road is Landscape paint/ruts, not a raised mesh: `reshape.py`
  grades a 2.8 m flat half-width plus 12 m falloff at ±11%, and its weightmap/rut SDF share the route.
  The recorded profile is p95 1.3 cm deviation, but reaches 1.107 m at ford (-12.53, -40.3) because a
  river cut after road grading leaves the painted route deep in water.

  Water first surveys rendered Landscape-versus-r16 1 m cross-sections at centre/±2.8/±15 m, then
  patches only mismatched edit-layer tiles <=5 cm or makes a local ford embankment with a new wooden
  bridge. Synchronize PNG, r16, roadProfile, weightmap, ruts, ground, material and map while retaining
  anchors/chainage. Also rerun `Scripts\Terrain\public_road.py` to regenerate compiled
  `Simulation\HomesteadEstatePublicRoad.inc` from `estate_layout` road/roadProfile and
  `EstateHeightfield.r16`; verify generated fast-travel stops, signs and terrain heights. Never broadly
  reshape from `game_raw_4033` or erase lake/river work.
- **Field-book road label** — **Water Agent** (`89914e30`): the redundant runtime `"Dirt road"` label
  is removed with lake `df19d74a` in `main` `61c1595c`
  (`HomesteadMapComponent::RefreshModel`; the road remains drawn), with a book-map lake/path
  screenshot. It **shipped with the lake** in the 4 PM playable build.
- **Farm-to-lake trail** — **Water, pending and not shipped.** The dashed lake path is absent on the
  ground; its current route is hidden below canopy litter. Cut a clear, actual woods trail from farm to
  landing through ground-material wear, then verify it visually and on foot. Water's separate
  `1d5b90a9` trail PNG/bin is committed but requires `build_ground.py`, importing `T_EstateGround` /
  `T_EstateCanopy`, `ImportEstateMap`, and a visual check before any delivery claim.
- **River road bridge** — **Water Agent** (`89914e30`), after the lake slice; a safe, walkable
  period wooden bridge where the road crosses the river. A Props mesh may be needed. Pending; not
  shipped.
- **Oil-lamp reach** — **pending behind the core loop, not shipped.** Held and placed lamps share
  `LightIntensity=1400` and `radius=1000 cm`, use inverse-square point lights with shadows, and flicker
  at 0.9–1.05. A literal +300% radius reaches 4000 cm but is only 1/16 as bright at 40 m versus 10 m
  and can expand shadow-caster volume about 64x; do not assume radius alone produces a useful throw.

  Props/Integration first trial measured low-gain broad fill or bounded falloff that avoids near glare
  and wall leak. Compare held and placed lamps in packaged RT-on 4K manor/woods fixed cameras at
  5/10/20/40 m: lux/median brightness, p95/p99, GPU and shadow cost, indoor wall leak, night warmth
  and unchanged oil use.
- **Reduced foliage shadow motion** — **Water, pending and not shipped.** EstateScenery HISM
  brambles/hedge/brush/thimbleberry are shadowed with WPO disabled beyond 60 m and
  `ShadowCacheInvalidationBehavior::Rigid`; individual resource bushes are movable/shadowed, while
  landscape grass has WPO but no shadow. The camera-safe foliage script has masked-opacity dithering
  but no explicit source WPO, so the cause may be shadow cache, dither or RT denoising rather than
  wind alone.

  Water pinpoints the shrub/material, then makes independent packaged RT-on fixed-camera A/B captures:
  wind off, reduced 25–50% tip-only motion, shrub shadows off and dither off. Compare pixel flicker
  and frame cost while retaining natural sway and tree shadows.
- **Nighttime brightness** — **pending, not shipped.** At about 9 PM, the Estate visibly brightens:
  moonlight reads like sunlight. Architecture's read-only trace of current `HomesteadWorld.cpp`
  (`59–64`, `1715–1784`, `4268–4337`) found no 21:00 trigger: sun reaches zero around 18:23,
  moon intensity is 2 lux and rises from roughly 7 to 46 degrees by 21:00, real-time sky capture
  intensity is 0.6 (day 1), and night auto-exposure has a -2 EV100 floor (day 0). The combined
  moon/sky/adaptation cause is plausible, not visually proven.

  **Water wired partial `71cffeaa` is unshipped and outside the 4 PM build.** It wires
  `HomesteadNightLight` into `UpdateLighting`; CVar defaults `NightMoonLux=0.2`, `NightSky=0.3` and
  `NightMinExposure=-1` match `NightLightTuning`. The schedule holds 0.2 lux moonlit ground after
  dusk (altitude compensation capped at 1 lux low), uses night sky 0.3 rather than 0.6 and leaves noon
  unchanged. Native 9/9 and the Editor module compile pass: tests pin a monotonic noon 1→night 0.3
  sky transition, dusk steps <=0.02, no 18:50→midnight brightening, no pops/moon glare and
  21:00/00:00/03:00 at -1.5 to -3.5 stops.

  It still needs PIE after the Menu/Props editor turns, then Integration's fixed-camera packaged RT-on
  Lumen hardware-ray-tracing plus VSM clear/rain captures at 18:00, 19:00, 21:00 and midnight to
  calibrate smooth dusk and lamp/hearth readability. Neither the trace nor the wired schedule
  establishes a visual fix; do not update the editor skill's default row before this sign-off.
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
- **Change Dye** — **Menu 1080p `[ready]` branch `4a3a895e`, integration review pending; not
  shipped or 4 PM content.** Isolated commits `3a664e4b` + `a38b57af` add
  `M_HomespunDyeable` / `MI_PrimitiveTankTop+Shorts` tint assets (three `.uasset`s). Four swatches
  preview the actual MetaHuman body and portrait; Cancel returns exact white at no cost; Apply Wine
  persists through F5 into a fresh PIE. It still needs 4K and carried-garment coverage. Integration
  reviews only the isolated dye commits; do not bundle the partial camera work.
- **Leather backpack upgrade** — **pending, not shipped.** A tentative one-time **1,500-coin**
  purchase at the open General Store doubles inventory capacity **120 → 240 items**. (`ShopGoods`
  normally repeats, so this needs a special upgrade row.) This preserves the old raw-value intent:
  above the 1,000-coin start, with cabbage harvests netting about 50 coins, and remains tunable later.
  The worn rucksack appears on her back and its Appearance show/hide is independent of capacity and
  saving.

  **Props** owns the core `bRucksackOwned` save state, `PackCapacity(state)` (120/240), validated
  optional trailing save section (old defaults false), and original leather back-socket prop; save
  loading reads structural inventory up to 240 **before** the entitlement tag, then runs post-tag
  `ValidateInventory`. **Menu** owns the shop upgrade row, `bRucksackVisible`, the Appearance toggle
  and the 120-cap UI helpers. Tests cover malformed/duplicate entitlement sections, rebuy refusal,
  insufficient funds and capacity/save behavior.
- **Town travel** — **core-loop priority; Menu Map travel `a4bb831f` is main-integrated in
  `1d1eebb5`, not packaged or shipped.** Its PIE evidence shows cancel remains atomic; Town→Manor advances 7 h 27
  while grounded/awake with hunger loss; Manor→Town preview warns the next-day store will be closed.
  Signs are not wired. A wooden `Walk to town` sign outside the estate and a return sign by town still
  invoke the same action as clickable **Town** and **Manor** map destinations. Architecture traced the
  road polyline in `estate_layout.json` (486 points / 1.94 km; runtime has landmarks only). The
  MetaHuman walks 210 cm/s (legacy 180); at a 60-minute day, road-only travel is 6.16 game hours /
  15.4 real minutes (12.32 game hours at a 30-minute day), plus connectors.

  Travel must first preflight a candidate advance for hunger failure, unexpected 6-hour doze and
  `MaxHour`, then atomically commit time plus a safe position through `PrepareWorldAt` /
  `SettleOnGround`; no unsafe fallback. On the Map, a single click selects and double-click/A zooms,
  so travel needs a separate explicit confirmation. The signs and map invoke the same action.
  **Water** owns the generated runtime route and endpoints, **Architecture** the read-only trace,
  **Props** the original signs and **Menu** the shared travel/map UI; signs are still not wired.

  **Store Map card `1b2b97eb` is `[ready]`, not merged or shipped:** native 9/9 (economy 15 /
  scenario 588) and PIE verify counter/square/gateway plus Manor-from-store clock no-change. Within
  15 m of counter/step and 45 m of town square, Town/Store cards say already
  there/in town rather than offering fake `X walk there 23 min`; T/X/click changes neither clock nor
  state. Manor travel from store remains available; street/gateway Town travel remains. Cherry-pick
  only this commit - not Menu's whole synced branch `d171526d` with unrelated partial dye/Appearance,
  pickup and pail work. It is excluded from package `a2607437`, but is included in Integration's
  bounded next-package validation set.
- **Wait for opening** — **Menu `88180744` shipped in the 4 PM playable build.** At a
  closed 19:00 store, B cancels with no time change; then A+A advances to the next 08:00 and returns
  Pascoe's Talk interaction. Follow-on `643a857a` rejects a candidate that would doze during
  preflight. It shares Menu's current integration/package review; the safe candidate preflight/commit
  path remains separate from future equivalent-time Town/Manor travel signs and map actions.
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
- Menu planting prompts (`a874d260`, `[ready]`, pending main): after it lands, update the editor skill's
  tilled-square controls: A/E sows the hotbar-selected seed; no seed selected says `Choose seeds on
  the hotbar to sow`; selected Wild Roots sow roots; F/X weeds only; selected zero stock gives the
  seed-specific refusal. The Menu README changes with the commit already describe the behavior.

## Tooling requests (unassigned)

- `get_play_state` (`st`) should report `namesOpen` and `shopOpen` (carried over from round 1).

## Next

When round 2's lanes are integrated: the save-version bump, a playtest build, and Jenny's answers to the
open questions.
