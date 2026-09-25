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
  step), relaunch. The editor locks its module DLLs, so build with it closed.

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

- Every PIE start generates a **new woodland seed**; positions and node ids differ each run.
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

## 8. Field notes

Dated and short, newest first. Promote anything durable into the sections above.

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
