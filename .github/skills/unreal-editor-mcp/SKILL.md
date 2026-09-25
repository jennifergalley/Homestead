---
name: unreal-editor-mcp
description: Control the real Unreal Editor 5.8 for Homestead/SurvivalGame through Epic's built-in MCP server - launch the editor, load the Homestead map, run the game in Play-In-Editor, capture what is on screen, read logs, inspect/edit actors and assets, run automation tests and Live Coding. Use whenever a task needs the live editor or the game running inside it, rather than headless commandlets or packaged builds.
---

# Unreal Editor MCP

Epic's experimental **Unreal MCP** plugin (`ModelContextProtocol`, shipped with UE 5.8 under
`Engine\Plugins\Experimental`) turns the running editor into an MCP server. The repo launches it
without touching `SurvivalGame.uproject`. Background and verification history: `docs\editor-mcp.md`.

**Keep this skill current.** When you discover a working recipe, a schema gotcha, a failure mode or
a better workflow, update the relevant section below (and the Field notes) in the same change.
Record outcomes, not diaries. Remove advice that proves wrong.

## 1. Is the server up?

```powershell
python Scripts\editor_mcp.py call list_toolsets
```

- Toolsets listed: ready, go to step 3.
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
  MCP and toolset plugins for this process only, and returns once `http://127.0.0.1:8765/mcp`
  answers (localhost only). Options: `-Port`, `-Map`, `-TimeoutSeconds`, `-EngineRoot`.
- The editor keeps running after the script returns.
- A fresh worktree has no `Binaries\`; the build step compiles the editor module (minutes).
- A cold start can take minutes. Quiet is not hung; check `Saved\Logs\SurvivalGame.log`.

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

Shell helper for repeated calls (writes arguments to a file to avoid PowerShell quoting problems):

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
```

## 4. Recipes (verified)

**Load the game map**

```powershell
mcp $S get_current_level
mcp $S load_level '{"level_path":"/Game/SurvivalGame/Maps/Homestead"}'
```

**Play the game in the editor and look at it**

```powershell
mcp $E StartPIE '{"options":{"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":8}}'
mcp $E IsPIERunning        # poll: StartPIE can report a timeout even though PIE started
mcp $E CaptureEditorImage  # the running game as shown in the editor window
```

**Read the log** (`category` "" means all; `pattern` is a required regex)

```powershell
mcp $L GetLogEntries '{"category":"","pattern":"LogHomestead|Error","maxEntries":40}'
```

**Stop playing**

```powershell
mcp $E StopPIE
```

**Editor viewport capture** (the editing camera, not the game view during PIE)

```powershell
mcp $E CaptureViewport '{"captureTransform":null,"annotations":null,"bShowUI":false}'
mcp $E CaptureViewport '{"captureTransform":null,"annotations":{"gridSpacing":500,"maxLabelDistance":8000,"maxLabels":10},"bShowUI":false}'
```

## 5. Toolset map

| Need | Toolset |
| --- | --- |
| PIE, captures, camera, selection, content browser, CVars | `EditorToolset.EditorAppToolset` |
| Load level, find/place/remove actors, traces, outliner folders | `editor_toolset.toolsets.scene.SceneTools` |
| Actor transforms/components; reflected properties and classes | `editor_toolset.toolsets.actor.ActorTools`, `editor_toolset.toolsets.object.ObjectTools` |
| Assets, meshes, textures, materials, Blueprints, data tables | `editor_toolset.toolsets.asset.AssetTools` and siblings (`static_mesh`, `skeletal_mesh`, `texture`, `material`, `material_instance`, `blueprint`, `data_table`) |
| Batch several tool calls in one sandboxed Python script | `editor_toolset.toolsets.programmatic.ProgrammaticToolset` |
| Output log | `EditorToolset.LogsToolset` |
| Automation tests | `AutomationTestToolset.AutomationTestToolset` |
| Compile C++ into the running editor | `LiveCodingToolset.LiveCodingToolset` |
| Click, type and snapshot editor UI | `SlateInspectorToolset.SlateInspectorToolset` |
| Sequencer, Control Rig | `animation_toolset.toolsets.*` |
| Config sections, plugins, physics assets | `ConfigSettingsToolset.*`, `PluginToolset.*`, `PhysicsToolsets.*` |

More Epic toolsets (PCG, Niagara, UMG, StateTree, GAS, MetaHuman, ...) live in
`E:\Program Files\UE_5.8\Engine\Plugins\Experimental\Toolsets`. Add the plugin name to `$plugins` in
`Scripts\Start-EditorMcp.ps1` and restart the editor to use one.

## 6. Rules

- Homestead's world is generated at play time. The unplayed map shows little or nothing in the
  editor viewport; judge the game from PIE captures.
- Don't save level or asset edits made while exploring unless the task calls for it. Generated
  content comes from `Scripts\bootstrap_unreal.py` and related scripts, and editor hand edits can be
  overwritten by the next bootstrap. Put durable changes in the generating scripts or source.
- PIE is an editor session. It supplements, but does not replace, packaged-build playtests and the
  acceptance evidence described in `docs\visual-playtesting.md`.
- Jenny's preview saves are selected by `Preview.cmd`'s profile argument; PIE does not use that
  argument. Don't copy or point PIE at her profiles.
- The server is localhost-only and exists only while the editor runs. Close the editor when you
  finish unless other work is using it; it holds a lot of memory.
- These plugins are Experimental. If a schema differs from this skill, trust `describe_toolset` and
  fix this file.

## 7. Field notes

Dated and short, newest first. Promote anything durable into the sections above.

- 2026-09-25: A fresh headless Copilot session loaded `unreal` from `.github\mcp.json` and called
  `list_toolsets` and `get_current_level` natively.
- 2026-09-25: First PIE start took about 76 s (shader and animation compilation); `StartPIE`
  returned "Timed out waiting for PIE to start" while PIE was running. `IsPIERunning` confirmed it.
- 2026-09-25: `CaptureViewport` rejects calls that omit `captureTransform`/`annotations`; pass `null`.
  It returns the PNG inside JSON text (`returnValue.image`), which `editor_mcp.py` extracts.
- 2026-09-25: During PIE, `CaptureViewport` showed the editor camera at the origin (black), while
  `CaptureEditorImage` showed the running game (Guidebook menu, Spring / Day 1, 06:00).
- 2026-09-25: `find_actors` with `{}` fails; it needs explicit keys (check `describe_toolset`).
- 2026-09-25: The editor shows a "Shader Model 6 required for Virtual Shadow Maps" notification on
  launch. It predates MCP and was left alone.
