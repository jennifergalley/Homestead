# Round 1: "Walk your estate"

Kicked off 2026-09-27 (Jenny's go at 10:54 Arizona time). Kickoff brief:
`openspec\changes\pivot-to-cozy-estate-life-sim\round-1-kickoff.md`. The docs agent keeps this
page current; report changes to it rather than editing lane rows yourself.

## Registry

| Role / lane | Session | Branch | Worktree (`E:\Repos\copilot-worktrees\SurvivalGame\...`) | MCP port | OpenSpec change |
| --- | --- | --- | --- | --- | --- |
| Orchestrator (coordinates only; never builds, merges, packages or verifies) | `92eac339-51a7-4354-bc79-d33d0da1a000` | `jennifergalley-unreal-engine-mcp` | `jennifergalley-cautious-pancake` | 8765 (so the native `unreal` MCP tools reach the orchestrator's editor) | `author-fixed-cornish-estate-map` |
| Integration and builds (only session that packages) | `e251051b-8674-4ef0-a3ed-03830407f8b6` | `jennifergalley-integration-and-builds` | `jennifergalley-literate-eureka` | 8775 | none |
| Docs agent | `d99bb15c-6135-4f9d-b21a-f46b63c3b36f` | `jennifergalley-work-optimizer` | `jennifergalley-stunning-dollop` | none (no editor) | none |
| Dollars and general store | `5cf73757-b7c2-43ce-9332-153a163267f3` | `jennifergalley-dollars-and-general-store` | `jennifergalley-fluffy-broccoli` | 8769 | `add-dollars-and-general-store` |
| Overgrown estate clearing | `ce241dd6-2c0b-47ea-a402-ec9fe5dc3572` | `jennifergalley-overgrown-estate-clearing` | `jennifergalley-stunning-waddle` | 8767 | `add-overgrown-estate-clearing` |
| Ruined manor and arrival | `f8b77021-941d-47e8-8bbd-1e93a632e5e8` | `jennifergalley-ruined-manor-and-arrival` | `jennifergalley-studious-doodle` | 8768 | `add-ruined-manor-and-arrival` |
| Estate boundary and minimap | `6e131c6a-a333-4f65-a10b-a634a2f04117` | `jennifergalley-estate-boundary-and-minimap` | `jennifergalley-automatic-spork` | 8766 | `add-estate-boundary-map-and-minimap` |
| Estate ocean and water | `89914e30-d8b6-4605-8635-5735406c97a2` | `jennifergalley-estate-ocean-and-water` | `jennifergalley-silver-guide` | 8771 | `author-fixed-cornish-estate-map` (task 2.3: ocean, river, pail refill) |
| MVP survival polish (separate product line; never merge with `main`) | `d587d011-6481-4e8d-a465-ecbe80e96bbc` | `mvp-survival` (session branch `jennifergalley-mvp-survival-polish`) | `jennifergalley-probable-barnacle` | 8770 | none |
| Planning (idle) | `57cf6ea4-e358-4d63-b34d-c140448d7ad6` | `jennifergalley-cozy-estate-pivot-plan` | | | `pivot-to-cozy-estate-life-sim` |
| Blender assets (idle) | `65a2408b-f87d-42c7-afdf-c48370465344` | `jennifergalley-blender-asset-pipeline` | | | |

**Current app names** (each session keeps "<one or two words> Agent" and renames itself when its main
task changes; `docs\handoff\README.md`):

| Name | Session | Work |
| --- | --- | --- |
| Orchestrator Agent | `92eac339` | coordinates the lanes |
| Integration Agent | `e251051b` | merges and builds; the only session that packages |
| Clearing Agent | `ce241dd6` | clearing |
| UI Agent, then Lamp Agent | `5cf73757` | UI, then the oil lamp |
| Farm Agent | `f8b77021` | the derelict farm |
| Ground Agent, then Sleep Agent | `89914e30` | ground, then the sleep change |
| Trees Agent | `65a2408b` | trees |
| Woodland Agent (app name still "MVP woodland biome") | `fd682909` | the MVP woodland biome |
| Build Speed Agent (app name still "Estate boundary and minimap") | `6e131c6a` | build speed |
| Documentation Agent | `a9f10974` (project session `d99bb15c`) | docs; named by Jenny, so kept |
| Architecture agent | `a1648ae7` (`jennifergalley-cuddly-invention`) | code steward: `docs\architecture.md`, "Code practices", code-convention skills, safe refactors, batch reviews |

Session IDs are the app's project-session IDs: use them with `send_session_message`. The worktree
folder name is the session's **mailbox address** for urgent `mailbox_send` messages
(`docs\handoff\README.md`).

## Lane status

The checkboxes on each lane's branch are the source of truth (`openspec\changes\<change>\tasks.md`).

**Integrated round-1 build: `main` at `3b8172ba` (2026-09-27, packaged by the orchestrator).** It
contains store `d679b64f`, clearing `96b8fb84` (with the pail fix), manor `dc2c86f9`, map `0bdec5fe`
and ocean v2 `8a407908`, plus world changes: every landscape proxy stays loaded, the pasture is
green, and `M_PropTextured` has the ISM and Nanite usage flags. All 7 native test suites pass. The
package boots the Estate in 3.5 s with no material errors. Jenny launches it from the **"Homestead
Estate"** desktop shortcut: the orchestrator worktree's `Build\Windows` with the argument
`/Game/SurvivalGame/Maps/Estate`. The first integration build was `f2e504c5`.

**Lanes: close your editor and *merge* `origin/main` into your branch** (merge, not rebase, if you've
already pushed), then continue and send `[ready]` with a SHA.

## Round-1 polish (after Jenny's playtest)

Lanes work from Jenny's playtest notes. Each delivers through "Delivering lane work"; the
integration session merges and packages.

| Lane | Session | Tasks |
| --- | --- | --- |
| Clearing | `ce241dd6` | salvage uses the stone-gather animation; rocks stuck to her arm; stick pickup sometimes falls back to the knee animation; meadow herbs show a prompt but no visible plant; "Requires a <tool>" prompts; more berry bushes (ids 540000+) |
| Store/UI | `5cf73757` | bigger vitals under the calendar; Slate HUD scaling at 4K; the craft slot fills with white as progress; "Haft" renamed "Craft"; a craft animation |
| Manor | `f8b77021` | the derelict farm (X -222..-162 m, Y -705..-645 m; clearable overgrowth ids 550000+); the estate more trashed and overgrown |
| Ground | `89914e30` (`jennifergalley-silver-guide`) | landscape textures; a camera-ring grass-blade field (`HomesteadGrassField.*`, `bake_ground.py` → `EstateGround.bin`); softer grass footsteps; woodland floor under the trees |
| Trees | `65a2408b` (Blender game assets) | new oak, beech, sycamore/ash, hawthorn and holly/hazel meshes under `Content/SurvivalGame/Environment/Trees`, for denser, lusher woods |

**Orchestrator, done at `b5a82e6b`:**
- The hearth plays a real CC0 recording (`Scripts\prepare_hearth_crackle.py`) at 20% volume,
  silenced by a sight-line trace whenever a wall is between her and the fire.
- The default surname is Cavendish.
- The woods are about 5x denser (`scatter.py`).
- Decorative scenery is hidden around every `State.resources` node.
- The "Homestead Estate" shortcut uses `Homestead.ico`.

**Registry: estate placement ids and scenery kinds.** Claim a range here (through the docs agent or
the orchestrator) before using it, and keep the comment at `Simulation\HomesteadEstate.h` ~87 in step.
**Section order:** later sections yield to earlier ones (they keep clear of what's already placed), so
`ProvisionalEstatePlacements` adds them in id order: berry brambles (540000+), then the derelict farm
(550000+), then the clear-out (570000+). Adding the farm first skipped 10 of 42 brambles and failed
`HomesteadSimulationTests` (~line 3259, `berries >= 36 && byDoor >= 2`).
**Append only:** placements added with `grow()`/`along()` in `HomesteadEstate.cpp` take sequential ids
(`next++`), so inserting a line mid-run renumbers every later placement, and saves then clear the
wrong nodes. Moving or removing placements needs `table.bakeVersion` raised (the orchestrator's call,
like the save version).

| Placement ids | Owner |
| --- | --- |
| 500000+ | world lane |
| 510000+ | overgrowth (clearing lane) |
| 520000+ | salvage piles |
| 530000+ | town (reserved) |
| 540000-540043 | berry brambles (clearing lane) |
| 550000+ | derelict-farm clearables (manor lane) |
| 560000-569999 | MVP woodland biome interactables (`fd682909`) |
| 570000-579999 | Coral Island-style clear-out near the manor (clearing lane) |

| Scenery kinds (`EstateSceneryKinds` in `HomesteadWorld.cpp`; `scatter.py` kind bytes must match) | Owner |
| --- | --- |
| 13-15: oak, beech, sycamore; 16-18: hawthorn, holly, hazel coppice | trees lane |
| 19-41 | MVP woodland biome (`Scripts\Terrain\mvp_woodland.py`) |

The MVP woodland polygon lives in `Scripts\Terrain\mvp_woodland.json` (not in `estate_layout.json`,
which `reshape.py` regenerates), shared by `scatter.py` and `bake_ground.py`. On its lane branch; not on
`main` yet.

**Re-bake order after `scatter.py` regenerates the scenery:** `bake_ground.py` and
`build_ground.py` (ground lane), then the estate map (`docs\setup.md`, "Estate map").

## Shared interfaces and rules this round

- Estate anchors and placements: `Source\SurvivalGame\Simulation\HomesteadEstate.cpp`
  (`ProvisionalEstateLayout`) mirrors `Scripts\Terrain\estate_layout.json`; update both together.
- Ground height for snapping level actors: `HomesteadEstateTerrain::Height` works outside the
  controller once the terrain is `Activate()`d. In the editor,
  `HomesteadEstateAuthoringLibrary.editor_ground_height` returns -1e9 where World Partition cells
  aren't loaded.
- Blender props face -Y in their recipes and import facing +Y, so C++ placement applies
  `LocalYaw - 90` (see the store's `Prop()`).
- Estate stand-in visuals (`AHomesteadWorld`, `HomesteadWorld.cpp` ~3140-3185): `SalvagePile` shows
  granite cobbles until the manor lane dresses the piles, and the spring flowers (`Primroses`,
  `Bluebells`, `WildDaffodils`, `WildGarlic`) show meadow-herb flowers in a grass tuft until each is
  authored.
- The standing room's door opens **west** into the ruin's south range. The comment at
  `HomesteadEstate.cpp` ~99 that says it "faces east onto the forecourt" is stale (clearing lane to fix).
- Fixed-estate resource persistence: cleared or edited placements are saved as `ResourceEdit`
  entries with chunk (0, 0) and `localId` = the placement id. Placement id ranges per lane are in
  `Simulation\HomesteadEstate.h` (world 500000+, overgrowth 510000+, ...).
- The item catalogue: `Simulation\HomesteadItems.{h,cpp}`. Append only; rows must match enum order.
  Serialize edits between lanes.
- Saves: lanes never change `SimulationSaveVersion`; the orchestrator bumps it once per integration.
  It's **13** since `c729d4e6` (v12 saves migrate; 11 is refused with a reset notice; 7-10 still migrate).
  If your branch adds anything to the save format, tell the orchestrator before your `[ready]`. New
  save data goes in tagged trailing sections (parcels, economy, `tools`, `manor`), and an unknown tag
  invalidates the save. Estate saves go to `<SaveGames>\Estate\`.
  **Items (save v13+, `c729d4e6`):** append to `enum class Item` (`Simulation\HomesteadItems.h`)
  freely; stocks now carry their width and v12 saves migrate. Never reorder or remove items. New save
  sections write any list or per-enum array with its count first. The save-safety hold is lifted.
- `SHomesteadMenu` edits are serialized through the orchestrator.
- **Integration session: packaging and batching.** Merge `[ready]`s in batches (one editor build, one
  game-target build, one native test run, one PIE pass per batch), and package only at the end of a round
  or when the orchestrator asks for a playtest build (Jenny, 2026-09-28). Lanes build only the editor
  module, and only when their C++ changed. Build acceleration (UBA cache, mutex, unity/PCH, faster
  cooking) is being investigated by the map lane (`6e131c6a`); findings go to the docs agent.
- **Packaging is centralized with the integration session** (`e251051b`). Lanes deliver through "Delivering lane work"
  in `docs\handoff\README.md`: verify, native tests, editor compile-check, commit/push, then message
  the orchestrator with branch, SHA, what changed, what was verified and what to try.
- Jenny's playable builds: see the editor skill, section 0.

## Open blockers and known bugs

- **Packaged test suites fail on retired woodland content.** Smoke and FullLoop (Reeds), Clearing
  (hatchet vs billhook), NativeMenu and Hotbar (Knife). Owner: clearing lane, retargeting them to
  the estate.
- **Strike and mow animations** aren't imported yet. Owner: clearing lane.

Resolved in `3b8172ba`: ruin kit and scenery granite drawing the Default Material in packages
(`M_PropTextured` now has ISM and Nanite usage flags), and distant landscape vanishing (every
landscape proxy now stays loaded; there's still no HLOD).

Resolved in `f2e504c5`: estate saves (the `ReadSave` Z bound), the cookfire/Hearth
`FocusLegacySubject` bug, and the estate spawn yaw. The orchestrator confirmed the yaw by eye in the
packaged build (she starts facing the lit doorway); there's no `controlYaw` assertion yet.

- **Packaging with several worktrees** (Zen `Failed to launch ZenServer` / `Failed to read oplog`,
  UBT `ConflictingInstance`): resolved. `Build-Game.ps1` builds the game target with `-WaitMutex`,
  cooks with `-SkipZenStore`, and waits for UAT (`cdbd249f`). Verified 2026-09-27 17:30 by the MVP
  lane: `-PackageOnly` packaged successfully (BuildCookRun 200 s, UAT about 6 min) and the packaged
  smoke, NativeMenu, Hotbar and FullLoop suites passed. Only one session packages now (the
  integration session since 2026-09-28).
- **Blender prop import** needs a free editor slot (2-process limit). Props are imported in a
  running editor via `run_python`, not the headless script.

## Decisions during the round

- **Playtest builds on a schedule** (Jenny, 2026-09-28): on the "Homestead Estate" shortcut by 7:30 AM
  daily and 4:00 PM on weekdays (plus weekend builds when noticeable features land). The orchestrator
  triggers the integration session at about 5:30 AM and 2:00 PM; lanes close their editors while it
  packages; `main` stays playable. Details are in `docs\handoff\README.md`, "Playtest builds".
- **Waiting means ending your turn** (Jenny, 2026-09-28, all sessions): no sleep/poll loops while waiting
  for a slot, the UBT queue, a `[ready]` or a perf window. Schedule a wake-up with
  `save_session_automation`, end the turn, and clear it afterwards (`docs\handoff\README.md`). The
  orchestrator checks in every 30 minutes the same way.
- **Perf measurements get the machine to themselves** (Jenny, 2026-09-28): one Unreal process, no
  builds. Claim it with `Scripts\Start-PerfWindow.ps1` and release it with `Stop-PerfWindow.ps1`; other
  worktrees' `Start-EditorMcp.ps1` waits while the lock is fresh.
- **At most 2 Unreal processes machine-wide** (Jenny, 2026-09-28; it was 3). Each editor commits
  15-17 GB; with three open, RAM ran out and the pagefile grew to 81.5 GB, filling C:.
  `Start-EditorMcp.ps1` enforces it. Close your editor as soon as a verification pass is done.
  **One slot is reserved for the Integration Agent** (`jennifergalley-literate-eureka`); all other lanes
  share the second, one at a time (also enforced).
- Agent editors start with Live Coding and ray tracing off (`bedbb9b8`, `50f9c64c`).
- Agent editors skip the new-game setup (`homestead.SkipNewGameSetup`, passed by `Start-EditorMcp.ps1`):
  new Estate games use the default names (Eleanor Cavendish, Trevennor). Set it to 0 in the console
  to test the Appearance and "Who comes home?" steps; packaged builds and normal play are unaffected.
- The Estate is the default editor startup and game map (`b07d4a82`). Packaged woodland test
  scripts pass `/Game/SurvivalGame/Maps/Homestead` explicitly until each suite is retargeted.
- HUD layout (Jenny): minimap bottom-right, calendar top-right, key hints top-left, and an icon-only
  vitals stack under the calendar (top-right) for food, energy and purse (`UI/SHomesteadVitals`).
- Each worktree's editor uses its own MCP port; 8765 and the native `unreal` tools aren't safe to
  assume.
- Shared-doc findings go through the docs agent (`docs\handoff\README.md`).
- Packaging is centralized (Jenny, 2026-09-27): only one session runs UAT and packaged tests;
  lanes implement, push and notify. `mvp-survival` packages its own deliverables to
  `E:\Repos\HomesteadMVP\Windows` after telling the orchestrator.
- **The orchestrator only coordinates** (Jenny, 2026-09-28). Hands-on integration moved to a dedicated
  session, `e251051b` ("Integration Agent", worktree `jennifergalley-literate-eureka`): it merges
  forwarded `[ready]`s, builds, runs native, packaged, PIE and perf checks, packages, and reports
  `[integrated] <what> @ <sha>`. Reason: while the orchestrator was busy with hands-on work its turn
  stayed open, so lanes' messages never reached it. Flow: lane `[ready]` → orchestrator → integration
  session → `[integrated]` → orchestrator → Jenny. **Packaging target:** the integration session packages into its own worktree
  (`jennifergalley-literate-eureka\Build\Windows`). At the end of the round, when the orchestrator
  says so, it retargets the "Homestead Estate" shortcut to that build's `JennysHomesteadGame.exe`, keeping
  the Homestead icon. Until then the shortcut stays on `jennifergalley-cautious-pancake`, and
  `Homestead.lnk` is never touched.
- Overgrown clearing lane: removes the knife, machete, warmth and fibre paths. Its "Estate tool route"
  is now in skill section 4; the manor lane added the on-foot routes to all five salvage piles.

## Pending doc updates on merge

- Lamp lane (on its branch, not on `main` yet): the new commands `LabHold Lamp`, `LabAction LampDown|LampUp`,
  `HomesteadLampOil <hours>`, and `HomesteadTeleport X Y [Z]` (stands her on the ground once collision has
  streamed in; McpHelpers `tp` switches to it); lamp clips from `homestead_agent.lamp_pose`
  (`build_raised` / `build_set_down` / `report`) and glass/flame materials from
  `homestead_agent.lamp_materials.build()`. When it lands, add these to the character-lab and
  console-command notes in the editor skill and check `tp`'s help in `McpHelpers.ps1`.

- Lamp lane (uncommitted in `jennifergalley-fluffy-broccoli` as of 2026-09-28): `build_prop.py` gains a
  recipe-level `BAKE_MESHES = {"SM_Name"}` (or a dict of per-mesh overrides) to bake only chosen meshes,
  so a prop can bake its opaque body while leaving separate glass or flame meshes unbaked
  (`Recipes/oil_lamp.py`: `SM_OilLamp` baked, `SM_OilLampGlass` and `SM_OilLampFlame` not). With only
  `BAKE`, every mesh still bakes. When it lands, add it to the Blender skill's "Bake and review
  settings" step and `docs\blender-assets.md`.

- Architecture agent (not on `main` yet):
  - **Legacy probes removed** (`26b367e9`): `Test-AuthoringSettings.ps1`, `Tests\HomesteadMenuSourceTests.py`,
    FernSpike and the `*Policy` scripts. Drop `Test-AuthoringSettings.ps1` from editor skill section 0's
    "Never stop shared processes" bullet, and delete the table 0.1 row about `HomesteadMenuSourceTests.py`
    failures.

## Tooling requests (unassigned)

- `get_play_state` (`st`) should report `namesOpen` and `shopOpen`. Both make the controller swallow
  gameplay keys, but `st` can't show them today, so a tap that does nothing looks like broken input
  (the Sleep/Ground lane, 2026-09-28). Asked for by that lane.

- The `HomesteadPlayTools` additions asked for by the clearing lane:
  - `bootstrap_estate_tools`: search every salvage pile, gather branches, haft all five tools and
    slot the hotbar in one call.
  - `swing` (or an action tool generally): press the action and return the resulting toast or
    notification synchronously, so a refused action isn't mistaken for broken input.

- A launch argument for the packaged Development build to set the start hour and take a screenshot
  without typing into the console (for example `-HomesteadStartHour=`), for night-lighting QA
  in the packaged build, since PIE runs without ray tracing. Asked for by the MVP lane.

## Next

- Integrate the remaining lanes, bump the save version once, package the estate build to its own
  "Homestead Estate" shortcut.
- Then flesh out round 2 (`rework-farming-calendar-and-period-crafting`) in OpenSpec before
  implementing it; salvage night-lighting and wake-up work from `wip/foliage-night-dawn-wake`.
