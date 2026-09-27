# Round 1: "Walk your estate"

Kicked off 2026-09-27 (Jenny's go at 10:54 Arizona time). Kickoff brief:
`openspec\changes\pivot-to-cozy-estate-life-sim\round-1-kickoff.md`. The docs agent keeps this
page current; report changes to it rather than editing lane rows yourself.

## Registry

| Role / lane | Session | Branch | Worktree (`E:\Repos\copilot-worktrees\SurvivalGame\...`) | MCP port | OpenSpec change |
| --- | --- | --- | --- | --- | --- |
| Orchestrator + world/terrain lane | `92eac339-51a7-4354-bc79-d33d0da1a000` | `jennifergalley-unreal-engine-mcp` | ask the orchestrator | ask the orchestrator | `author-fixed-cornish-estate-map` |
| Docs agent | `d99bb15c-6135-4f9d-b21a-f46b63c3b36f` | `jennifergalley-work-optimizer` | `jennifergalley-stunning-dollop` | none (no editor) | none |
| Dollars and general store | `5cf73757-b7c2-43ce-9332-153a163267f3` | `jennifergalley-dollars-and-general-store` | `jennifergalley-fluffy-broccoli` | 8769 | `add-dollars-and-general-store` |
| Overgrown estate clearing | `ce241dd6-2c0b-47ea-a402-ec9fe5dc3572` | `jennifergalley-overgrown-estate-clearing` | `jennifergalley-stunning-waddle` | 8767 | `add-overgrown-estate-clearing` |
| Ruined manor and arrival | `f8b77021-941d-47e8-8bbd-1e93a632e5e8` | `jennifergalley-ruined-manor-and-arrival` | `jennifergalley-studious-doodle` | 8768 | `add-ruined-manor-and-arrival` |
| Estate boundary and minimap | `6e131c6a-a333-4f65-a10b-a634a2f04117` | `jennifergalley-estate-boundary-and-minimap` | `jennifergalley-automatic-spork` | 8766 | `add-estate-boundary-map-and-minimap` |
| MVP survival polish (separate product line; never merge with `main`) | `d587d011-6481-4e8d-a465-ecbe80e96bbc` | `mvp-survival` (session branch `jennifergalley-mvp-survival-polish`) | `jennifergalley-probable-barnacle` | 8770 | none |
| Planning (idle) | `57cf6ea4-e358-4d63-b34d-c140448d7ad6` | `jennifergalley-cozy-estate-pivot-plan` | | | `pivot-to-cozy-estate-life-sim` |
| Blender assets (idle) | `65a2408b-f87d-42c7-afdf-c48370465344` | `jennifergalley-blender-asset-pipeline` | | | |

Session IDs are the app's project-session IDs: use them with `send_session_message`.

## Lane status

The checkboxes on each lane's branch are the source of truth (`openspec\changes\<change>\tasks.md`).
On `main` as of 2026-09-27: the Estate World Partition level and fixed-world mode (`d32b58a0`),
estate water, baked scenery and woodland placements (`cf90fc3e`), the item catalogue (`5ae9509d`),
money and the general store (`8ecb84a2`, `d5d1f51a`), and parcels (`a94d4f61`). The clearing,
manor and boundary lanes haven't merged yet.

## Shared interfaces and rules this round

- Estate anchors and placements: `Source\SurvivalGame\Simulation\HomesteadEstate.cpp`
  (`ProvisionalEstateLayout`) mirrors `Scripts\Terrain\estate_layout.json`; update both together.
- Ground height for snapping level actors: `HomesteadEstateTerrain::Height` works outside the
  controller once the terrain is `Activate()`d. In the editor,
  `HomesteadEstateAuthoringLibrary.editor_ground_height` returns -1e9 where World Partition cells
  aren't loaded.
- Blender props face -Y in their recipes and import facing +Y, so C++ placement applies
  `LocalYaw - 90` (see the store's `Prop()`).
- The item catalogue: `Simulation\HomesteadItems.{h,cpp}`. Append only; rows must match enum order.
  Serialize edits between lanes.
- Saves: lanes never bump `SimulationSaveVersion`; the orchestrator bumps once at integration. New
  save data goes in tagged trailing sections (parcels, economy, `tools`, `manor`), and an unknown tag
  invalidates the save. Estate saves go to `<SaveGames>\Estate\`.
- `SHomesteadMenu` edits and engine packaging are serialized through the orchestrator.
- Jenny's playable builds: see the editor skill, section 0.

## Open blockers and known bugs

- **Estate saves fail on `main`:** `ReadSave` rejects `abs(PlayerLocation.Z) > 5000`, and estate ground
  sits at Z ≈ 8700-9500. Fixed on the manor lane's branch; lands when it merges.
- **Cookfire/Hearth cook action broken on `main`:** it calls `FocusLegacySubject`, but craft rows
  are now `EHomesteadMenuSubject::Recipe`, so the toast "The cookfire recipe could not be
  selected." appears (`HomesteadController.cpp` ~2111). Owner to be assigned by the orchestrator.
- **Estate spawn yaw** is overwritten by `ChooseStartingView` and by `SetAppearancePreview(false)`
  restoring an earlier `SavedViewRotation`. Being fixed on the manor lane.

- **Packaging with several worktrees:** `LogIoStore: Error: Failed to launch ZenServer` when another
  worktree's zenserver holds port 8558. A per-worktree `[Zen.AutoLaunch] DesiredPort` is being
  tested by the orchestrator; not yet confirmed.
- **Blender prop import** needs a free editor slot (3-process limit). Props are imported in a
  running editor via `run_python`, not the headless script.

## Decisions during the round

- Agent editors start with Live Coding and ray tracing off (`bedbb9b8`, `50f9c64c`).
- Each worktree's editor uses its own MCP port; 8765 and the native `unreal` tools aren't safe to
  assume.
- Shared-doc findings go through the docs agent (`docs\handoff\README.md`).

## Pending doc updates on merge

- Estate boundary lane: the field book gains a Map tab (page 7, `M` opens it directly). Once it's on
  `main`, update the `FieldBookPages` cycle in skill section 4 "Field book" to 0 Inventory, 1 Craft,
  2 Build, 7 Map, 3 Guidebook, 6 Appearance (Settings stays outside the cycle). The docs agent does this.

## Tooling requests (unassigned)

- A launch argument for the packaged Development build to set the start hour and take a screenshot
  without typing into the console (for example `-HomesteadStartHour=`), for night-lighting QA
  in the packaged build, since PIE runs without ray tracing. Asked for by the MVP lane.

## Next

- Integrate the remaining lanes, bump the save version once, package the estate build to its own
  "Homestead Estate" shortcut.
- Then flesh out round 2 (`rework-farming-calendar-and-period-crafting`) in OpenSpec before
  implementing it; salvage night-lighting and wake-up work from `wip/foliage-night-dawn-wake`.
