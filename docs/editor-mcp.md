# Driving the Unreal Editor through MCP

Unreal 5.8 ships Epic's experimental **Unreal MCP** plugin (`ModelContextProtocol`) and the
**Toolset Registry** toolsets. Together they expose the running editor to AI agents over the
Model Context Protocol: agents can load levels, inspect and edit actors, assets, Blueprints and
materials, capture the viewport or the whole editor window, start/stop Play-In-Editor, read the
output log, run automation tests, trigger Live Coding and drive Slate UI.

Nothing is added to `SurvivalGame.uproject`. The plugins are enabled only for the editor process
started by `Scripts\Start-EditorMcp.ps1`, so commandlets, cooking and packaged builds are unchanged.

Agents: the working playbook is the repo skill `.github\skills\unreal-editor-mcp\SKILL.md`. Keep it
updated as you learn better recipes; this page records setup and verification.

## Start it

```powershell
.\Scripts\Start-EditorMcp.ps1            # builds SurvivalGameEditor, opens the editor, waits for MCP
.\Scripts\Start-EditorMcp.ps1 -SkipBuild # when the editor module is already built
```

The server listens only on `127.0.0.1:8765/mcp` (`-Port` overrides it) and rejects browser
origins other than localhost. It lives as long as that editor window; closing the editor stops it.
The script also passes the project's usual offline arguments (`Get-UnrealOfflineArguments.ps1`).

Enabled toolset plugins: `EditorToolset`, `AutomationTestToolset`, `ConfigSettingsToolset`,
`LiveCodingToolset`, `SlateInspectorToolset`, `PluginToolset`, `AnimationAssistantToolset` and
`PhysicsToolsets`. Other Epic toolsets (PCG, Niagara, UMG, StateTree, MetaHuman, GAS, ...) are in
`Engine\Plugins\Experimental\Toolsets` and can be added to the list in the script when needed.

## Use it from Copilot

`.github\mcp.json` registers the server as `unreal`. Copilot connects to MCP servers when a
session starts, so start the editor first, then start (or restart) the session. Run `/mcp` to
confirm `unreal` is connected. Project-level MCP config is loaded only for trusted folders.

The server uses tool search, so only three MCP tools appear:

1. `list_toolsets` - names and descriptions of every toolset.
2. `describe_toolset` with `toolset_name` - every tool and its JSON input schema.
3. `call_tool` with `toolset_name`, `tool_name` and `arguments`.

Useful starting points:

| Toolset | Tools |
| --- | --- |
| `EditorToolset.EditorAppToolset` | `CaptureViewport`, `CaptureEditorImage`, `StartPIE`, `StopPIE`, `IsPIERunning`, camera, selection, content browser, CVars |
| `editor_toolset.toolsets.scene.SceneTools` | `get_current_level`, `load_level`, `find_actors`, `add_to_scene_from_asset`, `trace_world`, folders |
| `editor_toolset.toolsets.actor.ActorTools` / `object.ObjectTools` | transforms, components, reflected properties |
| `editor_toolset.toolsets.programmatic.ProgrammaticToolset` | a sandboxed Python script that batches other tool calls |
| `EditorToolset.LogsToolset` | `GetLogEntries`, verbosity |
| `AutomationTestToolset.AutomationTestToolset` | discover, run and read automation tests |
| `LiveCodingToolset.LiveCodingToolset` | compile C++ changes into the running editor |
| `SlateInspectorToolset.SlateInspectorToolset` | snapshot and click/type in editor UI |
| `homestead_agent.toolset.HomesteadPlayTools` | play the game in PIE: real controller/keyboard input, `walk_to`, play-state readout (project toolset) |

## Playing the game

The stock tools can see the running game but can't play it: Slate clicks and key presses don't
reach Homestead, which reads input through its PlayerController. The project adds
`HomesteadPlayTools`:

- `Source\SurvivalGameEditor\HomesteadAgentPlayLibrary.*`: an editor-only C++ library that injects
  simulated input through `APlayerController::InputKey`, the same path as the game's playtest
  harnesses. It also auto-steers `walk_to` and reports player, menu and nearby-resource state.
- `Content\Python\homestead_agent\toolset.py`: exposes the library as an MCP toolset.
  `Content\Python\init_unreal.py` registers it only when the Toolset Registry is loaded, so
  commandlets (including the content bootstrap) are unaffected.

Controls, focus rules, menu quirks and a verified gather/eat/craft/fell loop are in the skill.

## Use it from a shell

`Scripts\editor_mcp.py` is a small standard-library client for sessions that started before the
editor, or for scripting. Images returned by tools are written to `Saved\McpCaptures`.

```powershell
python Scripts\editor_mcp.py tools
python Scripts\editor_mcp.py call list_toolsets
python Scripts\editor_mcp.py call describe_toolset '{"toolset_name":"EditorToolset.EditorAppToolset"}'
python Scripts\editor_mcp.py call call_tool '{"toolset_name":"EditorToolset.EditorAppToolset","tool_name":"CaptureEditorImage","arguments":{}}'
```

PowerShell mangles embedded quotes for native commands; write the arguments to a file and pass
`@path\to\args.json` when that happens.

## Verified behavior and known quirks

Verified on 2026-09-25 against UE 5.8.2: connect, list toolsets, `get_current_level`
(`/Game/SurvivalGame/Maps/Homestead`), `load_level`, `CaptureViewport`, `StartPIE`, `CaptureEditorImage`
of the game running in PIE (Guidebook visible), `GetLogEntries`, `StopPIE`. A fresh headless
Copilot session also loaded `unreal` from `.github\mcp.json` and called its tools natively.
Also on 2026-09-25, an agent played a fresh world in PIE through `HomesteadPlayTools`: it gathered
stones, berries, branches and reeds, ate from the Inventory, toggled a setting and restored it,
crafted a Crude hatchet and felled a tree with it. It also hot-patched the editor module with Live
Coding. A plain `-run=pythonscript` commandlet confirmed the toolset stays unregistered without the
MCP plugins.

- `CaptureViewport` requires `captureTransform` and `annotations` keys; pass `null` for defaults.
  During PIE it captures the editor level viewport, not the game view; use `CaptureEditorImage`
  to see the game.
- The Homestead world is generated at play time, so the editor viewport of the unplayed map is
  empty/black at the origin.
- The first PIE start took about 76 seconds (shader and animation compilation) and `StartPIE`
  reported "Timed out waiting for PIE to start" even though PIE started. Poll `IsPIERunning`.
- The plugins are Experimental; tool names and schemas may change with engine updates.
- MCP-driven PIE is an editor session. It does not replace packaged-build playtests or the
  acceptance evidence described in `visual-playtesting.md`.
