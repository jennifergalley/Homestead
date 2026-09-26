---
name: unreal-editor-mcp
description: Control the real Unreal Editor 5.8 for Homestead/SurvivalGame through Epic's built-in MCP server and play the game inside it - launch the editor, load the Homestead map, start Play-In-Editor, walk the heroine around, gather/craft/fell trees, navigate the field book (inventory, craft, settings), capture what is on screen, read logs, inspect/edit actors and assets, run automation tests and Live Coding. Use whenever a task needs the live editor or the game running inside it, rather than headless commandlets or packaged builds.
---

# Unreal Editor MCP

Epic's experimental **Unreal MCP** plugin (`ModelContextProtocol`, shipped with UE 5.8 under
`Engine\Plugins\Experimental`) turns the running editor into an MCP server. The repo launches it
without touching `SurvivalGame.uproject`, and adds a project toolset,
`homestead_agent.toolset.HomesteadPlayTools`, for playing the game with real controller input.
Setup and verification history: `docs\editor-mcp.md`.

**Keep this skill current.** When you discover a working recipe, a schema gotcha, a failure mode or
a better workflow, update the relevant section below (and the Field notes) in the same change.
Record outcomes, not diaries. Remove advice that proves wrong.

## 1. Is the server up?

```powershell
python Scripts\editor_mcp.py call list_toolsets
```

- Toolsets listed (including `homestead_agent.toolset.HomesteadPlayTools`): ready, go to step 3.
- `Cannot reach Unreal MCP`: start it (step 2).
- Another session may already own an editor (`Get-Process UnrealEditor`). Reuse a running MCP
  editor rather than opening a second one on the same project, and don't close an editor another
  session is using.

## 2. Start the editor with MCP

```powershell
pwsh -NoProfile -File .\Scripts\Start-EditorMcp.ps1            # builds SurvivalGameEditor first
pwsh -NoProfile -File .\Scripts\Start-EditorMcp.ps1 -SkipBuild # module already built
```

- Opens the visible editor on the startup map (`/Game/SurvivalGame/Maps/Homestead`), enables the
  MCP and toolset plugins for this process only, disables background CPU throttling (so PIE runs at
  full speed while another window has focus), and returns once `http://127.0.0.1:8765/mcp`
  answers (localhost only). Options: `-Port`, `-Map`, `-TimeoutSeconds`, `-EngineRoot`.
- The editor keeps running after the script returns.
- A fresh worktree has no `Binaries\`; the build step compiles the editor module (minutes).
- A cold start can take minutes. Quiet is not hung; check `Saved\Logs\SurvivalGame.log`.
- C++ changes: function-body edits can be hot-patched with Live Coding (section 5). New
  classes/UFUNCTIONs or `.Build.cs` changes need: stop the editor, run `Build.bat` (the script's build
  step), relaunch. The editor locks its module DLLs, so build with it closed. Live Coding patches
  live only in memory: relaunch without `-SkipBuild` afterwards so the DLL on disk matches source.
- `-ExtraPlugins A,B` enables more engine plugins for the session (for example
  `MetaHumanCharacter,MetaHumanSDK,MetaHumanCoreTech,MetaHumanGenerator`).
- `-AllowPython` registers `homestead_agent.toolset.HomesteadEditorPython.run_python`, which runs
  arbitrary editor Python (full `unreal` API; print output and an optional `result` value are
  returned). Opt-in because it is unrestricted code execution on the localhost server; use it for
  editor automation that Epic's toolsets don't cover (for example the MetaHuman Creator subsystem).

## 3. Choose how to call tools

- **Native MCP tools** (`unreal` server from `.github\mcp.json`): available only if the editor was
  serving when the Copilot session started. Check with `/mcp`. Prefer these when present.
- **Shell client** (always works): `Scripts\editor_mcp.py`. Images in results are saved to
  `Saved\McpCaptures\*.png` and replaced with `<saved to PATH>`; open them with the `view` tool.

The server exposes three meta-tools (tool search mode):

| Tool | Arguments |
| --- | --- |
| `list_toolsets` | none |
| `describe_toolset` | `toolset_name`; returns every tool with its JSON input schema |
| `call_tool` | `toolset_name`, `tool_name` (short name), `arguments` object |

Run `describe_toolset` before first use of a toolset; required arguments are enforced strictly.

Shell helpers (write arguments to a file to avoid PowerShell quoting problems). Put them in a
session scratch `.ps1` and dot-source it in each command:

```powershell
function mcp($ts, $tool, $a = '{}') {
  $j = @{ toolset_name = $ts; tool_name = $tool; arguments = (ConvertFrom-Json $a -AsHashtable) } |
    ConvertTo-Json -Depth 20 -Compress
  $j | Set-Content "$env:TEMP\mcpargs.json"
  python Scripts\editor_mcp.py call call_tool "@$env:TEMP\mcpargs.json"
}
$E = 'EditorToolset.EditorAppToolset'
$S = 'editor_toolset.toolsets.scene.SceneTools'
$L = 'EditorToolset.LogsToolset'
$SL = 'SlateInspectorToolset.SlateInspectorToolset'
$H = 'homestead_agent.toolset.HomesteadPlayTools'
function hk($tool, $a = '{}') { (mcp $H $tool $a | Out-String | ConvertFrom-Json).content[0].text }
function st([int]$n = 6) {   # parsed play state
  $o = mcp $H get_play_state "{`"nearby_count`": $n, `"radius_cm`": 4000}" | Out-String
  (($o | ConvertFrom-Json).content[0].text | ConvertFrom-Json).returnValue | ConvertFrom-Json
}
function shot() {            # capture the editor window (shows the game during PIE); returns PNG path
  $o = mcp $E CaptureEditorImage | Out-String
  if ($o -match 'saved to ([^>]+?\.png)') { $Matches[1].Replace('\\\\', '\') } else { $o }
}
```

## 4. Play the game

### Start and stop

```powershell
mcp $E StartPIE '{"options":{"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":5}}'
do { Start-Sleep 3; $s = st } until ($s.worldReady)   # StartPIE may report a timeout; poll instead
...
hk release_all; mcp $E StopPIE
```

- Every PIE start with no save in this checkout generates a **new woodland seed**. Once an
  autosave exists, PIE resumes it (same position/time), so positions and node ids persist.
- **LB/RB outside the book change the hotbar slot**, not book pages. Open the book first:
  `I` opens Inventory (page 0), Menu opens Settings (page 4).
- The field book opens on the Guidebook at start. Close it with B (`Gamepad_FaceButton_Right`).
- An editor "Missing Project Settings / Shader Model 6" notification covers the view after launch.
  Dismiss it: `mcp $SL Snapshot '{"ref":"","maxDepth":12,"bIncludeSourceLocations":false}'`, find
  `button "Dismiss" [ref=bN]`, then `mcp $SL Click '{"ref":"bN"}'`.

### HomesteadPlayTools

Input goes through `APlayerController::InputKey` as simulated events (the same path as the game's
own playtest harnesses), so it reaches gameplay and the native menu. All input tools return
immediately; observe with `get_play_state` and `shot`.

| Tool | Use |
| --- | --- |
| `get_play_state(nearby_count, radius_cm)` | JSON: `worldReady`, `bookOpen`, `bookPage`, `selectedRow`, `focusTitle`, `focusActions`, `toast`, `inventory` (units), `hunger/energy/warmth`, `hour`, `location`, `controlYaw`, `speedCmPerSec`, `hotbarSlot`, `walk`, `nearbyResources[]` (`id, kind, x, y, distanceCm, bearingDeg, cleared, ready, focused`) |
| `tap_key(key)` | Press and release one FKey (`E`, `Tab`, `Escape`, `Two`, `Gamepad_FaceButton_Bottom`, ...) |
| `hold_key(key, seconds)` | Hold a key/button (crafting, sprint) |
| `set_sticks(move_x, move_y, look_x, look_y, seconds)` | Hold sticks; move is camera-relative (Y forward), look X turns right |
| `walk_to(x, y, stop_distance_cm, timeout_seconds)` | Auto-steer with the left stick (and turn the camera) to a world XY; poll `walk` for `arrived` / `stuck` / `timeout` / `cancelled` |
| `release_all()` | Center sticks, release keys, cancel walking |

Controller map (preferred; the game is controller-first): A `Gamepad_FaceButton_Bottom`,
B `Gamepad_FaceButton_Right`, X `Gamepad_FaceButton_Left`, Y `Gamepad_FaceButton_Top`,
Menu `Gamepad_Special_Right`, LB/RB `Gamepad_LeftShoulder`/`Gamepad_RightShoulder`,
LT/RT `Gamepad_LeftTrigger`/`Gamepad_RightTrigger`, D-pad `Gamepad_DPad_Up/Down/Left/Right`,
hotbar slots `One`..`Nine`/`Zero`. Keyboard equivalents are in `README.md` Controls.

### Interacting with the world

- **Focus is the nearest interactable within 2.8 m, regardless of facing** (`UpdateFocus`). To target
  a node, get closer to it than to anything else: `walk_to` with `stop_distance_cm` about 45, then
  confirm `nearbyResources[].focused` on your target before pressing the action.
- Trees block movement, so `walk_to` a tree usually ends `stuck` at the trunk. That's fine if the
  tree is `focused`.
- Depleted nodes stay focusable with no actions (`cleared: false`, `ready: false`) and can steal focus.
  Filter targets by `ready`.
- `nearbyResources` covers only the currently streamed chunks. After walking far, nodes elsewhere
  (for example creek reeds) drop out of the list; walk back toward them to reload.
- `focusActions` shows the prompt, for example `[A] Gather   [RT] Clear with Knife`,
  `[RT] Fell with Hatchet`. Use the named button.

Verified loop (one fresh world): stones and berries with A, eat, branches ×3,
stream reeds, craft Crude hatchet, select hotbar `Two`, fell a tree with RT.

```powershell
$s = st 40; $t = $s.nearbyResources | Where-Object { $_.kind -eq 'BerryBush' -and $_.ready } | Select-Object -First 1
hk walk_to ("{`"x`": $($t.x), `"y`": $($t.y), `"stop_distance_cm`": 45, `"timeout_seconds`": 30}")
do { Start-Sleep 1; $s = st 4 } while ($s.walk -eq 'walking')
if (($s.nearbyResources | Where-Object focused).id -eq $t.id) { hk tap_key '{"key":"Gamepad_FaceButton_Bottom"}' }
```

Resource kinds: `Branches`, `Stones`, `BerryBush`, `Roots`, `Flowers`, `Reeds`, `Sapling`, `ForestTree`.
Measured speeds: walking about 180 cm/s at full stick, `walk_to` slows to about 80 cm/s inside 4 m,
sprinting (hold `LeftShift` while moving) about 300 cm/s.

### Field book (native menu)

- Menu opens the book on **Settings** (page 4). LB/RB change pages in the order 0 Inventory,
  1 Craft, 2 Build, 3 Guidebook, 6 (not inspected; likely Appearance), 4 Settings, wrapping.
  Loop LB until `st().bookPage` is the page you want. B closes/backs out.
- **Settings Up/Down moves two rows per press** in the ~1000 px PIE viewport, so only even rows
  (0 Save, 2 Game speed, 4 Invert camera Y, 6 Ambience, 8 Start a new woodland) are reachable by
  D-pad. `SHomesteadMenu::Columns()` returns 2 for page 4 although the rows were drawn stacked;
  not checked at full-screen sizes. Left/Right on a setting changes its value; A toggles.
  Verify the selected row visually before activating.
- "Start a new woodland" opens a Cancel-default confirmation; B backs out safely.
- **Inventory**: A on an item starts a *move*, not use. RT cycles regions (content, equipped slots,
  details). In details, D-pad Down reaches the action buttons (`Eat 1`, `Drop...`), then A.
- **Craft**: D-pad selects a recipe; details list requirements. Crafting is **hold A**
  (`hold_key Gamepad_FaceButton_Bottom 3` crafted once).
- Feedback messages ("Ate Berries.", "Made Crude hatchet.") appear as a banner on the book and in
  `toast` briefly. World-side hints (for example "Craft a crude hatchet before felling trees") may be
  visible in captures without appearing in `toast`, so capture after actions.

Settings changes persist ("Choice saved in game settings"). Where PIE writes them hasn't been
checked, so restore anything you change.

## 5. Other recipes (verified)

```powershell
mcp $S get_current_level
mcp $S load_level '{"level_path":"/Game/SurvivalGame/Maps/Homestead"}'
mcp $L GetLogEntries '{"category":"","pattern":"LogHomestead|Error","maxEntries":40}'   # pattern is a required regex
mcp $E CaptureViewport '{"captureTransform":null,"annotations":null,"bShowUI":false}'   # editor camera, not the game
mcp 'LiveCodingToolset.LiveCodingToolset' CompileLiveCoding    # hot-patch C++ function bodies into the running editor/PIE
```

### Character lab (model and animation iteration)

An endless flat grid floor with the real heroine: the same character, camera, input, anim graph and
foot placement, but no simulation, woodland, menus or saves. It doesn't touch Jenny's saves.

- PIE: `py "unreal.SystemLibrary.execute_console_command(None, 'homestead.CharacterLab 1')"`,
  then `StartPIE`. Set it back to 0 for the woodland. Packaged: `CharacterLab.cmd`, or
  `-HomesteadCharacterLab`.
- `get_play_state` reports `"characterLab": true`; sticks, keys and `walk_to` work as usual.
- Console (use `execute_console_command(world, ...)`): `LabAction Gather|Sticks|Water|Chop|Knife|Till`,
  `LabSun <hour>`, `LabCourse` (10/20/30° ramps and 10/20 cm steps at x = 2500),
  `LabTeleport <x> <y>`, `slomo 0.25`, `homestead.FootPlacement 0|1`.
- The HUD shows speed, gait/action weights, foot placement state, frame time and sun hour.

### Author an animation with the MetaHuman Control Rig

`homestead_agent.rig_authoring.Session` builds a level sequence with the heroine body and her
`MetaHuman_ControlRig`, keys controls, and bakes an AnimSequence. Details that cost time to find:

- `ControlRigSequencerLibrary.set_local_control_rig_*` doesn't key from Python here. Write the
  section channels directly (`control.Location.X`, `control.Rotation.X` = roll/Y = pitch/Z = yaw);
  the Session helpers do this.
- Evaluate the sequence once before reading offsets
  (`LevelSequenceEditorBlueprintLibrary.set_current_time`). Until then the hierarchy reports the
  rig's default skeleton, and `get_control_rig_world_transform` can return zeros.
- Arms need `arm_*_fk_ik_switch` = True for IK; legs are IK with the switch False.
- Spine controls: `roll` bends forward, `pitch` bends sideways, `yaw` twists.
- Bake with `SequencerTools.export_anim_sequence`; the AnimSequence factory needs
  `target_skeleton` set, or creation fails.
- Reading bones from the rig hierarchy after `set_current_time` does **not** give the keyed pose;
  it keeps returning the rest pose. To key something in a moving bone's frame (for example, the
  stick bundle cradled against `spine_05` in `kneel_gather.py`), bake once, read the bone from the
  baked clip with `bone_positions`, then key and bake again.
- `kneel_gather.clearance(anim)` reports per-frame forearm-to-spine and forearm-to-thigh
  distances. Use it to catch an arm passing through her body before you look at captures:
  under about 16 cm to the spine, or about 11 cm to the thigh, means intersection.
- Lab `LabAction Sticks` plays the kneeling stick gather with the stick props.

### Record a playtest video and measure motion

`CaptureEditorImage` is one still. To see motion (gait, wobble, shadow crawl), record the PIE
window with ffmpeg's Desktop Duplication grabber, which gives true 60 FPS where `gdigrab` gives
only about 30. `pip install imageio-ffmpeg` provides the ffmpeg binary.

1. Start PIE with `"playMode":"PlayMode_InEditorFloating"`.
2. Make the window topmost at a known rectangle, or the capture records whatever window is in
   front. Call `SetProcessDPIAware`, then call `SetWindowPos(hwnd, HWND_TOPMOST=-1, 0, 0, 1936,
   1119, 0x40)` on the editor process's `MainWindowHandle`.
3. Run `ffmpeg -filter_complex "ddagrab=output_idx=0:framerate=60:offset_x=0:offset_y=0:video_size=1920x1110:draw_mouse=0,hwdownload,format=bgra" -t 15 -c:v libx264 -preset ultrafast -crf 14 -pix_fmt yuv420p out.mp4`
   in a background process, and drive the heroine with `set_sticks`/`hold_key` meanwhile.
4. Review with `-vf "fps=2,scale=480:-1,tile=6x5"` contact sheets and crops of full-rate frames.

For numbers rather than pictures, register a per-frame Python callback in PIE with
`unreal.register_slate_post_tick_callback`. It records `get_socket_transform(bone, RTS_COMPONENT)`
and ground traces into a list; unregister it and dump the list to JSON afterwards. This is how
the root-motion wobble (head sway 35.6 cm peak to peak) and heel dips were found and verified.
Console variables can be flipped live for A/B captures with
`unreal.SystemLibrary.execute_console_command(world, 'homestead.FootPlacement 0')`.

## 6. Toolset map

| Need | Toolset |
| --- | --- |
| Play the game: input, walking, play state | `homestead_agent.toolset.HomesteadPlayTools` |
| PIE, captures, camera, selection, content browser, CVars | `EditorToolset.EditorAppToolset` |
| Load level, find/place/remove actors, traces, outliner folders | `editor_toolset.toolsets.scene.SceneTools` |
| Actor transforms/components; reflected properties and classes | `editor_toolset.toolsets.actor.ActorTools`, `editor_toolset.toolsets.object.ObjectTools` |
| Assets, meshes, textures, materials, Blueprints, data tables | `editor_toolset.toolsets.asset.AssetTools` and siblings (`static_mesh`, `skeletal_mesh`, `texture`, `material`, `material_instance`, `blueprint`, `data_table`) |
| Batch several tool calls in one sandboxed Python script | `editor_toolset.toolsets.programmatic.ProgrammaticToolset` |
| Output log | `EditorToolset.LogsToolset` |
| Automation tests | `AutomationTestToolset.AutomationTestToolset` |
| Compile C++ into the running editor | `LiveCodingToolset.LiveCodingToolset` |
| Editor UI: snapshot, click, type (editor chrome only; see Field notes) | `SlateInspectorToolset.SlateInspectorToolset` |
| Sequencer, Control Rig | `animation_toolset.toolsets.*` |
| Config sections, plugins, physics assets | `ConfigSettingsToolset.*`, `PluginToolset.*`, `PhysicsToolsets.*` |

More Epic toolsets (PCG, Niagara, UMG, StateTree, GAS, MetaHuman, ...) live in
`E:\Program Files\UE_5.8\Engine\Plugins\Experimental\Toolsets`. Add the plugin name to `$plugins` in
`Scripts\Start-EditorMcp.ps1` and restart the editor to use one.

The project toolset lives in `Content\Python\homestead_agent\toolset.py` (MCP wrapper) and
`Source\SurvivalGameEditor\HomesteadAgentPlayLibrary.*` (editor-only C++). `Content\Python\init_unreal.py`
registers it only when the Toolset Registry is loaded, so commandlets and bootstrap are unaffected.
Extend it there when play needs a capability; prefer real input over state edits.

## 7. Rules

- Jenny's rule: if you're unsure whether to make a change, make it when it would enhance realism
  and verisimilitude (also in `.github/copilot-instructions.md`).
- Jenny's rule: **whenever you spawn into the woodland to test something, make it morning** so the
  captures show something. After `worldReady`, run
  `py "w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()\nunreal.SystemLibrary.execute_console_command(w,'HomesteadMorning')"`
  (optional hour argument, default 8). It jumps the clock to the next morning without simulating
  the skipped hours; the same console command works in the packaged Development build. The
  character lab has its own sun (`LabSun <hour>`, default 10).
- Jenny likes to watch you work. Prefer `PlayMode_InEditorFloating` with the PIE window brought to
  the front (section 5) over hidden in-viewport PIE.
- If Jenny has the packaged game open from `Build\Windows` when you need to repackage, close it
  (`Stop-Process -Id <pid>` on the `SurvivalGame` processes) and build in place. She's only messing
  around in it for now and would rather get the newest build. Don't build to a side folder.
- Homestead's world is generated at play time. The unplayed map shows little or nothing in the
  editor viewport; judge the game from PIE captures.
- Don't save level or asset edits made while exploring unless the task calls for it. Generated
  content comes from `Scripts\bootstrap_unreal.py` and related scripts, and editor hand edits can be
  overwritten by the next bootstrap. Put durable changes in the generating scripts or source.
- PIE is an editor session with editor-only overlays and a small viewport. It supplements, but does
  not replace, packaged-build playtests and the acceptance evidence described in
  `docs\visual-playtesting.md`.
- `walk_to` and `get_play_state` are agent conveniences. Report what you observed through captures,
  and say when a result came from auto-steering or state readout rather than visual confirmation.
- Jenny's real mouse/keyboard also reach the editor. If she is using the PC, captures can show her
  hover tooltips and her input can interfere with play.
- The server is localhost-only and exists only while the editor runs. Close the editor when you
  finish unless other work is using it; it holds a lot of memory.
- These plugins are Experimental. If a schema differs from this skill, trust `describe_toolset` and
  fix this file.
- Record game bugs you find while playing as OpenSpec changes in `openspec/changes` (repo
  convention; not GitHub issues). The 2026-09-25 findings live in `fix-editor-playtest-findings`.
- Jenny's performance bar: the framerate must be **smooth**, not just high. Never report a
  performance result from average FPS alone. Check frame pacing on the `Playtest-Visual.ps1
  -PresentationDiagnostics` timing passes (median, p95, p99, max, frames over 20 ms and over
  33.3 ms), look for periodic spikes (for example, the paired 30-39 ms Virtual Shadow Map stalls
  that continuous sun rotation caused), and watch real walk/turn/camera sweeps in play for visible
  hitches. Test both capped (60 FPS) and uncapped. PIE timing is indicative only; smoothness
  sign-off needs the packaged build.

## 8. Field notes

Dated and short, newest first. Promote anything durable into the sections above.

- 2026-09-25: Original props are authored in Blender by the `blender-assets` skill
  (`docs\blender-assets.md`, `Assets\Props\*`). Its `Import-Props.ps1` opens the project in its own
  editor, so quit an MCP editor session first; never run both at once. The heroine's MetaHuman
  pipeline doesn't use Blender.

- 2026-09-25: The sun uses ray-traced shadows (`homestead.RayTracedSun`, default 1) and moves every
  refresh. `homestead.RayTracedSun 0` restores VSM with 0.5° steps. Don't reintroduce continuous
  rotation on the VSM path. See `docs/research/rendering-baseline/README.md`. The `-Presentation`
  smoke route fails at its first case for a pre-existing reason: `ApplyAppearance` rejects
  "Owned wardrobe appearance requires PrepareEquipment".
- 2026-09-25: MetaHuman body shape, GASP locomotion and tests:
  - **Body shape via Python works.** `try_add_object_to_edit`, then `get_body_constraints`; build a
    *new list* (changing the Array's structs in place doesn't stick), set `is_active` and
    `target_measurement`, then `set_body_constraints` and `commit_body_state`. The model shifts
    other measurements (Hip down pushed Waist up), so pin those too.
  - A rigged character must have its face rig removed first (`remove_face_rig`), then be re-rigged
    (`request_auto_rigging`, ~25 s), reassembled, and re-retargeted.
  - `metahuman_look.set_body` wraps this. Always `remove_object_to_edit` afterwards, or Creator
    later fails to open the asset ("already added for editing... may be corrupted").
  - **Migrating from another project headless:** run `UnrealEditor-Cmd <other>.uproject
    -run=pythonscript` with `AssetTools.migrate_packages(pkgs, <our Content dir>, MigrationOptions(prompt=False))`.
    It follows every dependency. The GASP clips dragged in 272 foley sounds and about 40 extra
    clips via notifies, which had to be pruned.
  - **Retargeting UEFN → `IK_MH_IKRig`:** keep the Root Motion op **off**. Even with target root
    `root` and `COPY_FROM_SOURCE_ROOT`, it wrote a root that snaked and yawed up to 16° per step.
    Locking that root in game made the heroine's head and shoulders wobble. `gasp_locomotion`
    retargets with the op off, then `straighten_root` puts the travel on a straight root before
    locking the clips in place (`set_root_motion_enabled`). Check a new clip with
    `get_bone_pose_for_time(anim, 'root', t, False)`: the root should have no yaw and no sideways
    drift.
  - `Test-Game.ps1` smoke routes run a plain `-game` process (not UE automation) with
    `-HomesteadSmokeTest`, which selects the legacy heroine.
  - The Hotbar route's "Held mapped Shift again reaches active grounded sprint" step is flaky
    after `NewGame`; it passed on rerun.

- 2026-09-25: Assembling and playing the MetaHuman heroine:
  - `build_meta_human` can raise RuntimeError (Control Rig "Cannot break link") even when the log
    says the assembly succeeded, **and it doesn't save**. Call
    `EditorAssetLibrary.save_directory(path, False, True)` on the Assembled and Common folders.
  - Texture resolution: set all eight `skin_settings.desired_texture_sources_resolutions` fields to
    `RES4K` before `request_texture_sources`. `has_high_resolution_textures` is a property.
  - Retargeting to `IK_MH_IKRig`: the "Run IK Rig" op blew poses out to about -9000 cm, and "Root
    Motion" sank the pelvis. Disable both by index (`set_retarget_op_enabled(i, False)`). After
    fuzzy auto-mapping, clear the bad maps (Root from a foot, metacarpals from fingers).
  - Python can't spawn actors into the PIE world. Test through the character's own component stack
    instead.
  - Live Coding (`LiveCodingToolset` `CompileLiveCoding`) patches function bodies while PIE runs.
    Standalone `-game` runs load the DLL from disk, so do a real build first.
  - Tools like the hatchet are hidden except during their action, so capture a burst about
    0.5-1.3 s after the action key. Prompts switch to keyboard after `tap_key` of a keyboard key
    (`[LMB] Fell`); a `Gamepad_RightTriggerAxis` tap then doesn't act. Use `LeftMouseButton`.
  - For a face close-up in PIE, set the `CameraArm` `target_arm_length` (about 90),
    `socket_offset` 0 and `target_offset` (0,0,70) via `run_python`. Restore afterwards (330,
    (0,45,55)).
  - Frame-cost breakdown: `-ExecCmds "t.MaxFPS 0, r.GPUCsvStatsEnabled 1, csvprofile start"` on
    `Playtest-Visual.ps1`. The CSV lands in `Saved\Profiling\CSV\`. Before timing, check for other
    heavy processes (a Blender render from another session skewed runs).

- 2026-09-25: `homestead_agent.metahuman_look` wraps the Creator scripting: `set_hair`,
  `apply_colours` (groom Melanin/Redness, eye presets) and `sculpt_face` (81 mapped landmarks).
  Gotchas:
  - Groom and outfit instance parameters live on `sub.get_preview_collection(c)`, not
    `c.internal_collection`. They only appear after `assemble_for_preview`, and after the editor
    has ticked once past a hair change, so change hair in a separate `run_python` call.
  - Parameter `name`s are `unreal.Name`; key dicts by `str(name)`.
  - Creator's eye presets aren't Python-visible. Export `EyePresets` to T3D and `import_text` the
    settings (the module does this).
  - `translate_face_landmarks` moves points only part of the way (about 40%), so scale the deltas.

- 2026-09-25: Killing the editor leaves `Saved\Autosaves\PackageRestoreData.json`. The next launch
  opens a modal "Restore Packages" dialog that blocks startup and the MCP server, and synthetic
  clicks on Skip Restore don't dismiss it. After a kill, delete that file (and scratch autosaves)
  before relaunching. Quitting via `run_python` `unreal.SystemLibrary.quit_editor()` (after
  `LevelEditorSubsystem.editor_request_end_play()`) exits cleanly in seconds and leaves restore
  disabled.
- 2026-09-25: Rotating a directional light invalidates every cached Virtual Shadow Map page. The
  world refresh (every 0.25 s) used to rotate the sun continuously, causing paired 30-39 ms stalls
  at 4K. On the VSM path `UpdateLighting` steps rotation by 0.5°; the default ray-traced sun
  moves continuously.
- 2026-09-25: Console commands in PIE: `run_python` with
  `unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world(), '<cmd>')`.
  To A/B lighting live, call `set_actor_tick_enabled(False)` on `HomesteadController` so its
  Refresh stops overwriting the sun. Then set the sun's rotation, intensity and colour, and the
  `MeadowExposure` post-process settings (set `override_<name>` too), and capture. Turn the tick
  back on afterwards. A new game starts at hour 6 (dawn), and one game hour is 2.5 real minutes.
- 2026-09-25: Creator scripting recipes (via `run_python`). Duplicate a preset from
  `/MetaHumanCharacter/Optional/Presets/<Name>` to start a character. Swap hair with
  `c.internal_collection.try_add_item_from_wardrobe_item('Hair', wi)` plus
  `default_instance.set_single_slot_selection('Hair', key)`, using wardrobe items under
  `/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/`. **Scripted edits don't appear in an open
  Creator window, even after Refresh Preview.** Close the editor (`close_all_editors_for_asset`),
  release edit mode, save, then reopen. For review shots, set `viewport_settings.camera_frame`
  (FACE/BODY) and `show_viewport_overlays = False` before opening, then use Slate `Screenshot` on
  the `MHC_<name>` window. `set_body_constraints` + `commit_body_state` rebuilt the body mesh but
  `get_body_constraints` read back unchanged values, even on a blank MetaHuman. Body shaping
  through Python is unverified; use Creator's Body sliders until proven. `CaptureAssetImage`
  doesn't support MetaHuman characters.
- 2026-09-25: The Epic Launcher queues engine-option installs indefinitely while an editor is open.
  Exit the launcher from the tray and reopen it.
- 2026-09-25: Rendering presets one at a time opens and closes a Creator window every ~40 s.
  Warn the human before starting, because it looks like the editor is restarting and it steals
  any window they're clicking in. Creator's "Missing Project Settings → Enable Missing" writes
  the three `r.GPUSkin`/`r.SkinCache` lines to `DefaultEngine.ini`. They only take effect after
  the next editor restart.

- 2026-09-25: There is no general Epic sign-in in the editor. MetaHuman cloud sign-in lives in the
  person icon on the MetaHuman Creator toolbar ("No user signed in, please autorig to trigger
  log-in flow"). Only **Create Full Rig** starts the login flow. **Download Texture Sources**
  just fails with "User not logged in, please autorig before downloading..." toasts. The login
  needs the human, so ask Jenny to do it. The launcher's project list only
  shows projects it created or opened itself. Open this worktree's `SurvivalGame.uproject` with
  `Start-EditorMcp.ps1` rather than from the launcher.
- 2026-09-25: Installing engine options from the Epic Launcher while an editor is open can leave
  the install queued at "initializing" forever. Fix it by exiting the launcher from the tray and
  reopening it.
- 2026-09-25: `CaptureAssetImage` doesn't support MetaHumanCharacter assets. Capture the Creator
  window instead; it's titled with the asset name. Creator tabs are checkbox refs (Presets,
  Head & Body, Materials, Hair & Clothing, Export, Assembly).
- 2026-09-25: MetaHuman Creator (enabled via `-ExtraPlugins`) opens, but logs "MetaHuman Optional
  Content folder not found ... limited features" until the engine's
  `Plugins/MetaHuman/MetaHumanCharacter/Content/Optional` (TextureSynthesis, BodyTextures) is
  installed from the Epic Launcher. Opening a MetaHuman also reports missing project settings:
  `r.GPUSkin.Support16BitBoneIndex`, `r.GPUSkin.UnlimitedBoneInfluences`,
  `r.SkinCache.CompileShaders`, and D3D12 SM6 (for Virtual Shadow Maps). The log also warns that
  Lumen has no ray-tracing data (no distance fields or hardware RT enabled).
- 2026-09-25: `MetaHumanCharacterEditorSubsystem` (via `run_python`) exposes creation, body/face
  commits, `request_auto_rigging`, `request_texture_sources`, `build_meta_human` and
  `spawn_meta_human_actor`. The stock `MetaHumanGenerator` toolset didn't appear in
  `list_toolsets` even with its plugin enabled.
- 2026-09-25: Slate `Click`/`PressKey` on game menu widgets only moved hover/focus; the game reads
  input through the PlayerController, not Slate focus. That is why `HomesteadPlayTools` exists.
- 2026-09-25: PIE shows an on-screen editor warning, "Multiple directional lights are competing to
  be the single one used for forward shading...". It comes from the game's lighting setup, not MCP.
- 2026-09-25: Felled trees vanished within 0.8 s with no visible fall or stump in captures.
- 2026-09-25: The creek renders as a flat, saturated blue ribbon in PIE captures.
- 2026-09-25: The book feedback banner ("Ate Berries.") pushes the Inventory layout down about 60 px.
- 2026-09-25: Live Coding through `CompileLiveCoding` patched `HomesteadAgentPlayLibrary` while PIE
  was running (about 5 s).
- 2026-09-25: A fresh headless Copilot session loaded `unreal` from `.github\mcp.json` and called
  `list_toolsets` and `get_current_level` natively.
- 2026-09-25: First PIE start took about 76 s (shader and animation compilation); `StartPIE`
  returned "Timed out waiting for PIE to start" while PIE was running.
- 2026-09-25: `CaptureViewport` rejects calls that omit `captureTransform`/`annotations`; pass `null`.
  It returns the PNG inside JSON text (`returnValue.image`), which `editor_mcp.py` extracts.
- 2026-09-25: `find_actors` with `{}` fails; it needs explicit keys (check `describe_toolset`).
- 2026-09-25: `IKRetargetBatchOperation.run_batch_retarget` returns nothing while PIE runs; stop
  PIE first.
- 2026-09-25: After `AnimationDataController.set_bone_track_keys`,
  `AnimationLibrary.get_bone_pose_for_frame` reports the pelvis about 10.7 cm lower than the
  written keys (a skeleton-vs-retarget-source offset). The game doesn't apply that offset. Write,
  read back, and compensate (see `gasp_locomotion.straighten_root`).
- 2026-09-25: Trees are components of the `HomesteadWorld` actor. When searching for terrain with
  traces, require the `ProceduralMeshComponent` hit and a single hit per column; otherwise a
  teleport lands the heroine on a tree canopy.
- 2026-09-25: The view tool sometimes reports a freshly written PNG as missing; view it again.
