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

**Keep this skill current.** Several sessions share this machine and this file. Report what you
discover (a working recipe, a schema gotcha, a failure and its fix, a better workflow) to the
round's docs agent (`docs\handoff\README.md`); it records each finding once, here or in the right
doc. With no docs agent running, update the relevant section yourself in the same change, add
failures to table 0.1, and commit to `main` promptly; the orchestrator reconciles conflicts. Record
outcomes, not diaries. Fix or remove advice that proves wrong instead of adding a contradicting note.

## 0. Shared-machine rules (read first)

Several agent sessions (one worktree each, under `E:\Repos\copilot-worktrees\SurvivalGame\`)
build, run editors and package on one PC with one RTX 5080 at the same time.

- **Only the integration session packages.** UAT (`Build-Game.ps1 -Package`/`-PackageOnly`, `RunUAT
  BuildCookRun`) and packaged-game tests run only in the integration session's worktree (registry in
  `docs\handoff\round-<n>.md`). Lanes implement, verify in the editor, run native tests, compile-check
  with `Build.bat SurvivalGameEditor ... -WaitMutex`, commit and push, then send `[ready]` to the
  orchestrator ("Delivering lane work" in `docs\handoff\README.md`). The orchestrator only
  coordinates and never builds. The separate `mvp-survival` line packages its own deliverables to
  `E:\Repos\HomesteadMVP\Windows`, after telling the orchestrator.
- **At most 2 Unreal processes on the machine** (Jenny, 2026-09-28; it was 3), counting editors,
  packaged games and commandlets (`UnrealEditor-Cmd` imports and bootstraps too). Each editor commits
  15-17 GB of memory: with three open, the 32 GB machine ran out of RAM and the pagefile on C: grew to
  81.5 GB and filled the drive. **Close your editor as soon as a verification pass is done**
  (`Stop-MyEditor.ps1`). `Start-EditorMcp.ps1` refuses to launch at 2 and
  lists who owns them (`-Force` overrides). For other launches (a standalone game, a commandlet), check
  in the same command, right before starting:
  `Get-Process UnrealEditor*,SurvivalGame*,JennysHomestead* -ErrorAction SilentlyContinue`.
  More processes than that have reset the GPU driver and exhausted VRAM, which takes every
  session's editor down.
- **One editor per worktree, on its own MCP port.** Pick a free port in 8766-8799:
  `8766..8799 | ? { -not (Get-NetTCPConnection -LocalPort $_ -State Listen -EA 0) } | select -First 1`.
  Pass it as `Start-EditorMcp.ps1 -Port <p>`, then dot-source `Scripts\McpHelpers.ps1 -Port <p>`
  (section 3). The script refuses a port another worktree's editor is serving. Never drive or close
  another session's editor.
- **The native `unreal` MCP tools are hard-wired to port 8765** (`.github\mcp.json`). In round 1
  that's the orchestrator's editor, so in any other session they drive the orchestrator's editor.
  Lanes use the shell helpers instead.
  If you do use native tools, confirm the worktree first:
  `print(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))`.
- **Live Coding and ray tracing are off in agent editors** (`Start-EditorMcp.ps1` defaults). One
  active Live Coding session anywhere blocks **every** worktree's editor build: UBT checks a mutex
  named after the shared `UnrealEditor.exe` path, not the project (`HotReload.cs`). The script turns
  it off two ways (an `-ini:` override on the command line, and `bEnabled=False` written into this
  worktree's `Saved\Config\WindowsEditor\EditorPerProjectUserSettings.ini` before launch, which
  also covers hand-launched editors and `-game` runs), and it no longer loads `LiveCodingToolset`,
  whose `CompileLiveCoding` switches Live Coding on for the session whatever the setting says. For C++
  changes, quit the editor, rebuild and relaunch. `-RayTracing` turns RT back on; use it only when
  you must judge RT lighting and no other Unreal process is running. PIE lighting
  therefore differs from the packaged build, which has RT on.
- **Close your editor** before `git pull`/`rebase` (it locks `.uasset` files), before building
  `SurvivalGameEditor` (it locks the DLLs), and when you finish. Quit cleanly with
  `quit` (McpHelpers) or `.\Scripts\Stop-MyEditor.ps1 -Port <p>`, which closes only this worktree's
  editor: a clean quit through MCP first, then by PID if it hangs. Never pick an editor by name or
  window: several worktrees have them open.
- **The editor opens the Estate by default** (`EditorStartupMap` and `GameDefaultMap` are
  `/Game/SurvivalGame/Maps/Estate` since `b07d4a82`), and that startup is clean, so don't pass `-Map`.
  (An explicit `-Map /Game/SurvivalGame/Maps/Estate` once hung startup for 20 minutes.) For the old
  woodland, `load_level('/Game/SurvivalGame/Maps/Homestead')` after MCP answers.
- **Never stop shared processes.** `zenserver.exe` (the DDC/Zen server on port 8558),
  `UnrealTraceServer.exe`, `ShaderCompileWorker.exe` and other worktrees' `UnrealEditor*`/UBT/UAT
  processes may be serving another session's build or cook. Stop only processes you started, by PID.
  Scripts that refuse to run while any Unreal process exists (`Invoke-ShippingQA.ps1`,
  `Test-AuthoringSettings.ps1`) need an idle machine; coordinate through the orchestrator.
- **Perf and frame-rate measurements need the machine to yourself** (Jenny, 2026-09-28): only ONE
  Unreal process (the one you measure) and no UBT/`cl.exe` builds. With several editors and builds
  running, readings swung about 5x (render thread 20 ms vs 97-118 ms). Before measuring, run
  `Scripts\Start-PerfWindow.ps1 -Purpose '<what>'` (add `-ProcessId <pid>` for a standalone game you
  launched). It refuses, naming every other Unreal process and build, unless yours is the only one,
  and then writes `E:\CopilotScratch\homestead-perf.lock`. While that lock is under 20 minutes old,
  `Start-EditorMcp.ps1` in other worktrees refuses to launch. Ask owners with `mailbox_send` to close or
  pause, and run `Scripts\Stop-PerfWindow.ps1` as soon as you're done. Don't build while someone
  else holds the window.
- **Nothing that pops up on Jenny's desktop.** Don't use `startfpschart`/`stopfpschart`: every dump
  opens an Explorer window on `Saved\Profiling\FPSChartStats\<timestamp>`, and she asked us to stop.
  For frame times, use `stat unit` / `ProfileGPU` output from the log, or `Playtest-Visual.ps1
  -PresentationDiagnostics` and csvprofile (section 9). If you really need an FPS chart, set
  `t.FPSChart.OpenFolderOnDump 0` first (`ChartCreation.cpp`).
- **Scratch and helper files** go in the worktree's `Saved\` (git-ignored) or
  `E:\CopilotScratch\<session-id>\`, never `%TEMP%` (on C:, and shared between sessions) or a shared
  fixed filename. See the disk rules in `~\.copilot\copilot-instructions.md`.
- **Jenny's playable builds.** Never retarget or overwrite `Desktop\Homestead.lnk` or anything
  under `E:\Repos\HomesteadMVP\`. The estate build gets its own "Homestead Estate" shortcut. Never
  merge the `mvp-survival` branch with `main`, in either direction.
- **Saves.** Lanes never change `SimulationSaveVersion`; the orchestrator bumps it once per
  integration. It's **12** since `0e08e717`: version 11 saves are refused with a reset notice, and
  versions 7-10 still migrate. If your branch adds anything to the save format, tell the orchestrator
  before your `[ready]`. PIE saves live in the worktree's `Saved\SaveGames` (Estate in
  `SaveGames\Estate\`). Packaged saves live inside the package at `SurvivalGame\Saved\SaveGames`,
  not in `%LOCALAPPDATA%`.
- **Git with sub-agents.** Don't `git stash -u` while a sub-agent may be writing files; use
  `git pull --rebase --autostash`. Never commit `Content/Trials/Probe/` or `.uasset`/`.umap`
  changes that aren't yours. The Estate level uses one file per actor: commit only your own
  `__ExternalActors__` files plus the `__ExternalObjects__` file for any new Outliner folder, and
  `git checkout` the re-saved `Estate.umap`.

**Lane quick-start** (one worktree, port `$p` from the round's registry in `docs\handoff\`):

```powershell
git status --short | Measure-Object; Test-Path .\SurvivalGame.uproject   # new worktree complete? (0 and True; else see 0.1)
$p = 8768                                                     # your registered port
Get-Process UnrealEditor*,SurvivalGame*,JennysHomestead* -EA 0 # fewer than 2? (Start-EditorMcp checks too)
pwsh -NoProfile -File .\Scripts\Start-EditorMcp.ps1 -Port $p -AllowPython -TimeoutSeconds 1200   # -Port picks the MCP port
. .\Scripts\McpHelpers.ps1 -Port $p                             # every later command; sets UNREAL_MCP_URL for editor_mcp.py
# ...work, StartPIE, hk/st/hshot...
.\Scripts\Stop-MyEditor.ps1 -Port $p                              # before building, rebasing, or when done (only your editor)
& 'E:\Program Files\UE_5.8\Engine\Build\BatchFiles\Build.bat' SurvivalGameEditor Win64 Development "-Project=$PWD\SurvivalGame.uproject" -WaitMutex -NoHotReloadFromIDE   # compile-check
.\Scripts\Test-Native.ps1 -Configuration Release               # ~3 min; Debug is 16-19 min
git status --short                                               # commit only your files, rebase, push
# then send_session_message the orchestrator: branch + SHA, what changed, what you verified, what to try.
# Don't package: only the integration session runs Build-Game.ps1 -Package.
```

### 0.1 Known failures → fixes

Search this table for the error text before debugging. Add a row when you solve a new one.

| Symptom (exact text where known) | Cause | Fix |
| --- | --- | --- |
| `Unable to build while Live Coding is active` | Some editor on the machine has an active Live Coding session. UBT checks a mutex named after the shared `UnrealEditor.exe` path, so it's any editor, any worktree (a hand-launched editor, one started before the opt-out, or one where `CompileLiveCoding` ran). An editor log shows `LogLiveCoding: Display: Starting LiveCoding` when it starts | Pass `-NoHotReloadFromIDE` to `Build.bat` (the scripts do), which skips the check. To find the culprit, search editors' `Saved\Logs\*.log` for `Starting LiveCoding`. Agent editors now start with Live Coding off (command line plus ini) and without `LiveCodingToolset`. |
| `Result: Failed (ConflictingInstance)` from UBT | Two worktrees building at the same moment | Retry after a minute. `-WaitMutex` (used by the scripts) queues instead. A "Build.bat already running" wait is the same queue. |
| An editor build (`Build.bat SurvivalGameEditor ... -WaitMutex`) sits silent for many minutes | It's queued behind other worktrees' UBT builds (the mutex is machine-wide). With 4 lanes building at once, one build waited about 50 minutes (3265 s in total) | Expect it and don't kill the waiting UBT: a killed build rejoins the back of the queue. Check who's building with `Get-CimInstance Win32_Process -Filter "Name='dotnet.exe'"` (UnrealBuildTool in the command line) before assuming a hang. |
| `Build-Game.ps1 -Package` fails within seconds: `A conflicting instance of Global\UnrealBuildTool_Mutex_... is already running. Result: Failed (ConflictingInstance)`, then `AutomationTool exiting with ExitCode=10 (Error_SDKNotFound)` (the exit code is misleading) | UAT's build step can't wait for another worktree's UBT. With several targets it puts `-UbtArgs` inside each `-Target="..."` string, where `-WaitMutex` is ignored | Fixed: `Build-Game.ps1` builds `SurvivalGame` itself with `Build.bat ... -WaitMutex`, then runs `BuildCookRun -skipbuild`. Do the same for a hand-run BuildCookRun. |
| Editor never shows a window, or startup holds for 10+ minutes; the log stops at `Build.bat -Mode=ValidatePlatforms` (the Turnkey platform check) | Every editor start runs that UBT check (no AutoSDK is set up, so it always runs). It's single-instance, so it waits on another worktree's UBT build | `Start-EditorMcp.ps1` now stops its own editor's ValidatePlatforms child tree after 2 minutes. By hand: `Get-CimInstance Win32_Process -Filter "ParentProcessId=<editor pid>"`, then `Stop-Process -Id` that `cmd.exe` and its children. The editor continues. Don't set `UE_SKIP_UBT_SDK_SETUP=1` instead: the editor then treats every platform SDK as invalid. |
| MCP never answers (20 min, log frozen) after `-Map /Game/SurvivalGame/Maps/Estate` | Opening the 4 km World Partition map through `-Map` during startup | Don't pass `-Map`: the Estate is now the default startup map and opens cleanly without it. |
| Every MCP call times out for minutes, but the editor process is still responding | The shared zenserver (`%LOCALAPPDATA%\UnrealEngine\Common\Zen`) stopped answering and the game thread is blocked on it; it recovers by itself (one stall lasted 1009 s: `post recovery finished in 1009.384 seconds`) | Search `Saved\Logs\SurvivalGame.log` for `LogZenServiceInstance` and wait. Don't kill the editor, and never stop `zenserver.exe`. |
| MCP takes more than 10 min to answer on the first launch after a build | Shader and DDC warm-up | Normal. Use `-TimeoutSeconds 1200`; watch `Saved\Logs\SurvivalGame.log`. Quiet isn't hung. |
| `DXGI_ERROR_DEVICE_REMOVED` / `DRIVER_INTERNAL_ERROR`; every Unreal process dies | Several editors creating ray-tracing pipelines (RTPSO) at once on one GPU | Ray tracing is off by default in agent editors. Keep to 2 Unreal processes. |
| `Video memory has been exhausted` (for example 2 GB over budget); captures take 3-20 s and slow-mo bursts miss clips | 3 editors plus the 4 km Estate landscape, or 3 editors plus a packaged game | Keep to the 2-process limit, and close idle editors. |
| C: fills up, and the machine is sluggish with 3 editors open | Each editor commits 15-17 GB of private memory. With three open, only 0.7 GB of RAM was free and the system-managed `C:\pagefile.sys` grew to 81.5 GB. It doesn't shrink until a reboot | At most 2 Unreal processes (`Start-EditorMcp.ps1` enforces it). Close editors as soon as you're done. If C: is already full, ask Jenny to reboot to shrink the pagefile. |
| MCP answers, but with another worktree's map, actors or code | Two editors on 8765, or native `unreal` tools pointing at 8765 | Use your own `-Port` and `McpHelpers.ps1`; check `unreal.Paths.project_dir()`. |
| Modal "Restore Packages" at startup blocks MCP; `Start-EditorMcp.ps1` times out with "MCP did not answer"; Escape doesn't dismiss it | The editor was killed; `Saved\Autosaves\PackageRestoreData.json` remains | `Start-EditorMcp.ps1` now deletes a stale restore file before launching (when this worktree has no editor running). If a dialog is already up: kill that editor, delete `Saved\Autosaves`, relaunch. Quit with `quit_editor()` next time. |
| Editor startup hangs with no log output after `Waiting for ZenServer to be ready`; a native "Wait for ZenServer?" Yes/No dialog is up | The log shows `Found existing instance running on port 8558 with different data directory, will attempt shutdown`: this worktree's `DerivedDataCache\Zen` differs from the running zenserver, so the editor restarts zenserver on its own data dir. That can also pull Zen out from under another worktree's editor | `Start-EditorMcp.ps1` now answers Yes automatically while it waits (it sends the dialog's `IDC_YES` command). By hand: find the window titled "Wait for ZenServer?" for the editor PID with `EnumWindows` and post `WM_COMMAND` 1003 to it (UIA Invoke isn't available). Warn other lanes if you see the shutdown line. |
| Every MCP call hangs after a reimport or bake | A hidden modal ("Overwrite Existing Object") behind PIE | Stop PIE before reimports and Sequencer bakes. To recover, find the modal with user32 `EnumWindows` on the editor PID and click it, or kill and restart the editor. |
| `CaptureEditorImage`: `Failed to capture any editor windows` | Floating or minimised PIE window, or a different monitor | `hshot` (`HighResShot` through `execute_console_command` with the player controller) writes `Saved\Screenshots\WindowsEditor\*.png`, but without Slate UI. For UI, bring PIE in-viewport and retry `shot`, or capture a standalone `-game` window. |
| The hotbar, vitals or field book are missing from a screenshot | `HighResShot` (`hshot`) renders the scene and Canvas HUD only; Slate viewport widgets aren't drawn into it | Use `shot` (`CaptureEditorImage`) or `[GameWin]::Capture` of a standalone `-game` window. |
| `save_asset` returns False | PIE is running | Stop PIE, then `save_loaded_asset(obj, False)`. |
| PIE crashes after a Live Coding patch; `Binaries\Win64\*patch*` locked | Live Coding patch state | Quit, wait about 60 s, delete `Binaries\Win64\*patch*`, rebuild. |
| A material renders as the default grid; the log has `Failed to compile` | A Masks-compressed texture on a sampler that isn't `SAMPLERTYPE_MASKS`, or sRGB engine defaults on a Masks sampler | Match the sampler type; use `T_PropDefault{White,Black}`. Python material builds report success even when the result fails to compile (the mesh renders flat grey in PIE): our `*_ao`/`*_roughness` textures are `TC_MASKS` non-sRGB and need `SAMPLERTYPE_MASKS`. |
| A mesh draws the Default Material only in the packaged build (for example the ruin kit, scenery granite) | The base material lacks a usage flag for how the mesh is drawn; the editor log says `missing usage flag InstancedStaticMeshes/Nanite! Default Material will be used in game` | Set the usage flag on the base material (`bUsedWithInstancedStaticMeshes`, `bUsedWithNanite`), not the instance. `M_PropTextured` now has both, and `Scripts\Blender\import_props.py` keeps them on every bootstrap; `M_Fern02` and `M_Shrub04` got ISM. Search the editor log for that line before packaging new materials. |
| Landscape grass (`LandscapeGrassOutput` + `LandscapeGrassType`) produces empty `GrassInstancedStaticMeshComponent`s in PIE on the Estate | Unknown (UE 5.8); also with `grass.GrassMap.UseRuntimeGeneration=1`, which needs an editor restart because the value is cached per shader platform | Gated off. Don't spend time on it without a new idea; scatter grass another way. |
| Editor RHI-thread crash when deleting a material | Deleting an asset the renderer is using | Reuse the asset and clear its graph. `MaterialEditingLibrary.delete_all_material_expressions` alone leaves nodes behind on rebuilds (71 → 35; a second `LandscapeGrassOutput` survived). Symptoms: "only one Single Layer Water Material node", "The material can contain only one Landscape Grass node", missing-input errors, `get_statistics` vs/ps 0, and a silent fallback to the default grid. After `delete_all`, loop over `get_material_expressions` calling `delete_material_expression`, then assert `get_num_material_expressions(m) == 0` (`Scripts\Terrain\build_landscape_material.py` does this). |
| `unreal.CustomInput(input_name=...)` fails in the constructor; or a Custom node won't compile | Constructor kwargs aren't supported; input/output names that clash with HLSL identifiers | Create it empty, then `set_editor_property('input_name', ...)`, and use unique names. In vertex-shader code sample with `Texture2DSampleLevel(Tex, TexSampler, uv, mip)`; the sampler is `<InputName>Sampler`. |
| A `VolumeTexture` built from Python has the wrong tile size, or property errors | Setting `source2d_texture` resets the tile size to a default (102 for a 1024² atlas); `VolumeTexture` has a single address mode (no `address_x`) and no `blueprint_get_size_x` | Set `source2d_texture` first, then `source2d_tile_size_x/y`. In a Custom node sample it with `Texture3DSample(Vol, VolSampler, uvw)`. |
| An Estate actor edited from Python looks unchanged in PIE | Spatially loaded World Partition actors stream into PIE from their **saved** external-actor packages (non-spatially-loaded ones such as `EstateSea` show edits live), and Python setters such as `AHomesteadWaterRibbon.set_course()` don't dirty the package | Call `actor.modify()` before editing, then `unreal.EditorLoadingAndSavingUtils.save_packages([actor.get_outermost()], False)` before PIE. |
| An OBJ imported through Interchange comes in mirrored and invisible from outside | Interchange maps OBJ (x, y, z) to Unreal (x, −y, z), which flips winding | Write y negated and swap face winding (a, c, b). |
| Distant Estate land missing from elevated or far PIE views | PIE streams about 8 landscape proxies (252 m each, roughly ±400 m) around the **pawn**, with no HLOD, and streaming follows the pawn, not the view target | Move or park the pawn (`MOVE_FLYING`) near the camera for distant captures. |
| `Import-Props.ps1` imports meshes without LODs or collision | `StaticMeshEditorSubsystem` is missing under `-run=pythonscript` | Import inside your running editor with `run_python` (section 8; Props in section 9). |
| Edits to `import_props.py` don't take effect | `import` returns the cached module | Load with `importlib.util.spec_from_file_location` + `exec_module`. |
| C++ duplicate-symbol or redefinition errors (`C2084 function already has a body`) between unrelated `.cpp` files, often only when packaging | Unreal unity builds merge translation units, anonymous namespaces included. The editor build compiles git-modified files outside unity (adaptive unity), so clashes first appear in the packaged game build | Give file-local helpers unique names or prefixes (for example `Og*` in `HomesteadOvergrowth.cpp`), and never put `using namespace` at file scope in a `.cpp`. An anonymous namespace inside a shared named one (`namespace HomesteadMenus { namespace { Gold } }`) still collides across UI files; use a named inner namespace. Compile-check the game target (`Build.bat SurvivalGame Win64 Development ...`) as well as the editor before sending `[ready]`. |
| `error C2487` on a `static constexpr` line in a UCLASS | Several declarators in one line of an `_API`-exported class | Declare one per line. |
| `C4459: declaration of '<Name>' hides global declaration` inside engine headers (for example Chaos) | A file-scope name in your `.cpp` (such as `constexpr ... Face`) leaks into the unity blob; anonymous namespaces don't help | Rename it to something project-specific. |
| New lane's worktree is incomplete: `SurvivalGame.uproject` missing, thousands of staged deletions in `git status` | The app's `create_session` worktree step hit `git command timed out after 300 seconds` (a full checkout with LFS is slow under load), but the session started anyway | `git -C <worktree> reset --hard HEAD` (about 3 min), then confirm `git status` is clean and `SurvivalGame.uproject` exists before building. |
| Estate "Save failed... check disk space and permissions" (misleading text) / saves rejected on load | `ReadSave` in `HomesteadController.cpp` rejected `abs(PlayerLocation.Z) > 5000`; estate ground is Z ≈ 8700-9500 | Fixed on `main` in `f2e504c5` (bound by `MaxWorldCoordinate`). Rebase if you still see it. |
| Spawn yaw ignored on Estate | `ChooseStartingView` (fresh terrain) and `SetAppearancePreview(false)` restoring a `SavedViewRotation` captured before spawn both overwrote it | The manor lane's fix is in `f2e504c5`; if it recurs, check `controlYaw` and a capture after the book closes. |
| "The cookfire recipe could not be selected." | The Fire/Hearth cook action called `FocusLegacySubject`, but craft rows are now `EHomesteadMenuSubject::Recipe` | Fixed on `main` in `f2e504c5`. Rebase if you still see it. |
| UBT crashes during the game-target build with exit code -532462766 (0xE0434352, an unhandled .NET exception) | A stale `.git\worktrees\<name>\index.lock` left by an interrupted git command (all worktrees share one `.git`) | Make sure no git command is running in that worktree, delete the lock file, retry. |
| Packaged smoke/full-loop test steps land on the wrong field-book page | The Map tab added a page to the LB cycle | Fixed in the tests (four LB taps). Count pages from `FieldBookPages` when writing new routes. |
| `Test-Game.ps1` LoadLatest reports an unreadable save | Stale saves left in the `Saved\Automation\SmokeSave` sandbox | Fixed: `Test-Game.ps1` empties the sandbox at the start of each run (`49605e37`). |
| Save strings with non-ASCII characters break the save | The save payload rejects bytes above 127 | Hex-encode free text (the manor lane does this for names). |
| An editor toast "source content changed, import?" covers captures | The editor watches `Assets\` and Blender writes there | `Start-EditorMcp.ps1` now starts with `bMonitorContentDirectories=False`; import explicitly with `import_props.py`. |
| Default argument errors with `Homestead::` enums in `HomesteadCharacter.h` | The header only forward-declares the enum | Use overloads or `{}`, not enum default arguments. |
| `System.Exception: A conflicting instance of AutomationTool is already running` (in `%LOCALAPPDATA%\UnrealEngine\Programs\AutomationTool\Saved\Logs\ErrorLog.txt`); the script only says "Game packaging failed (1)" | UAT is single-instance machine-wide and another worktree is packaging | `Build-Game.ps1` now passes `-WaitForUATMutex` and waits. For a hand-run `RunUAT.bat`, add it yourself. |
| `git status` shows dozens of modified `.uasset`s (Audio, heroine animations and materials) after `Build-Game.ps1` | The content bootstrap re-saves generated assets | Restore the ones your change didn't intend (`git checkout -- <paths>`) before committing. `-PackageOnly` skips the bootstrap when content is current. |
| PIE woodland forest floor near-black at noon; terrain half streamed | Agent editors run with ray tracing off; the game's lighting is tuned for RT | Don't judge brightness, night lighting or shadows in PIE. Use the packaged build (RT on), or `-RayTracing` when process limits allow. |
| A red on-screen warning in Development builds: "Cached lighting in Lumen ... going to be clipped ... r.EyeAdaptation.CachedLightingPreExposure", in full sun (about EV 13.5) | The cached-lighting pre-exposure range didn't cover bright sun | `DefaultEngine.ini` `[SystemSettings]` sets `r.EyeAdaptation.CachedLightingPreExposure=8` (about EV -4 to 16). If it returns, adjust that value, not the exposure. |
| `'HomesteadLabController' object has no attribute 'get_pawn'` | Not exposed to Python | `unreal.GameplayStatics.get_player_pawn(world, 0)`. |
| `NameError: name '__file__' is not defined` in `run_python` | `run_python` executes a code string, not a file | `pyfile <path>` (McpHelpers), or `exec(compile(open(p).read(), p, 'exec'), {'__file__': p, '__name__': '__main__'})`. |
| Saved actors, but the level still shows a dirty package or a teammate's checkout lacks your new Outliner folder | A new Outliner folder lives in its own `__ExternalObjects__/.../<Map>/...` package | Also save `EditorLoadingAndSavingUtils.get_dirty_map_packages()` and commit that package. |
| `LogIoStore: Error: Failed to launch ZenServer`, `Failed to add sponsor process IDs to launched ZenServer`, or `Failed to read oplog from Zen at [::1]:8558 ... HTTP NotFound` while staging (see `Saved\Logs\UnrealPak.log`) | The cook used the machine-shared Zen server (port 8558), and another worktree's zenserver held the port, restarted or was stopped mid-run | `Build-Game.ps1` now cooks with `-AdditionalCookerOptions=-SkipZenStore` and deletes a stale `Saved\Cooked\Windows\ue.projectstore` first (staging reads Zen whenever that marker exists). Never stop `zenserver.exe`: other worktrees may be cooking through it. |
| UAT log shows another worktree's build | `%APPDATA%\Unreal Engine\AutomationTool\Logs\E+Program+Files+UE_5.8\` is shared and overwritten | Redirect `Build-Game.ps1` output to a log in your worktree (`*> Build\Logs\package.log`). |
| `git pull`/`rebase`: `unable to unlink ... Invalid argument` on `.uasset` | Your editor holds the file | Close the editor, then `git status` and finish the rebase. |
| `git merge`/`rebase`: `Your local changes to the following files would be overwritten` on `.uasset`s you didn't mean to change | The editor or a bootstrap re-saved them incidentally | Close the editor, then `git checkout -- Content` (or the specific paths) for the incidental files, and merge. |
| PowerShell ``.Replace("a`nb", ...)`` silently changes nothing | Working copies are CRLF (`core.autocrlf=true`); agent-written files may be LF | Detect the newline first (``$t.Contains("`r`n")``), or use the edit tool. |
| ctest reports `HomesteadSimulationTests` failed or timed out | The Debug build takes about 10 min | `Scripts\Test-Native.ps1 -Configuration Release` (about 3 min). Redirect a single test exe's output to a file; stdout is buffered. |
| `HomesteadEstateAuthoringLibrary.editor_ground_height` returns -1e9 | That World Partition cell isn't loaded in the editor | Load the region first. In game, `GroundHeight()` uses the runtime heightfield everywhere. |
| `Test-Game.ps1` runs only the default smoke test, or errors "Generated resume requires..." | Switches passed as an array or as empty strings | Use a hashtable splat: `$p=@{Packaged=$true; Hotbar=$true}; .\Scripts\Test-Game.ps1 @p`. |
| `tap_key` letters type nothing into a text box (for example the Names card); BackSpace works | `HomesteadPlayTools` sends key events through `InputKey`, which produces no character events for Slate text boxes | Use real Win32 keys: bring the editor's main window forward (the Alt `keybd_event` trick, as in `click`), then send `keybd_event` VK codes, with Shift for capitals. |
| Slate Inspector `Snapshot` returns nothing useful, non-JSON, or refs that don't match last run | It needs `{"ref":"","maxDepth":60}` and a prior `Observe`; output is occasionally malformed; widget refs (`b30`, `b97`...) change between runs | Observe first, retry on non-JSON, and look refs up by name each run rather than hard-coding them. |
| Clicking a button in a custom Slate panel breaks Tab or typing (Tab runs Slate navigation, keys go to the old row) | `SButton`s take keyboard focus on click | Give such buttons `.IsFocusable(false)`. |
| UI is double-scaled at 4K but fine in PIE | The engine DPI curve (`bAllowHighDPIInGameMode`) already scales viewport widgets, so an extra height/1080 `SScaleBox` doubles them | Don't add your own resolution scaling. Check real resolutions in a standalone window (section 8), because PIE at editor size hides it. |
| A kit mesh placed from Python is 100 times too big, or rotated wrongly | `StaticMeshComponent` locations are centimetres at scale 1; `unreal.Rotator(a, b, c)` positional order is (roll, pitch, yaw) | Use cm, and pass rotators by keyword: `unreal.Rotator(roll=..., pitch=..., yaw=...)`. |
| `LineTraceComponent` never hits a mesh (returns false), even in PIE, though world traces do | `UPrimitiveComponent::LineTraceComponent` doesn't hit **Nanite** static mesh components | Trace the world (`World->LineTraceSingleByChannel`, or `line_trace_single` in Python) with other components ignored, and check `Hit.GetComponent()`. The terrain `ProceduralMeshComponent`s aren't Nanite, so component traces still work on them. |
| Estate traces return None or captures show missing land right after PIE starts | World Partition is still streaming (about 55 s on the Estate) | Wait about a minute after `worldReady` before tracing or capturing, or take Z from the heightmap (row below). |
| `FMath::Max(SomeTArray)` doesn't compile | There's no TArray overload | Loop, or use `Algo::MaxElement`. |
| `FVector2D` has no `Rotation()` | Only `FVector` does | Use `GetRotated(Degrees)`, or build the vector yourself. |
| Every mesh of a runtime-built ISM actor shows twice in PIE | PIE duplicates the editor instance's instance components, but not a transient list of them (`AHomesteadManorRuin` / `AHomesteadDerelictFarm` pattern: a transient `Parts` array plus `AddInstanceComponent`) | In `Rebuild`, destroy every `UInstancedStaticMeshComponent` from `GetComponents<>()`, not only the tracked ones. |
| `import_props` fails: the FBX `changed after the Blender build` | It checks each FBX's sha256 against the recipe's `report.json`, and a Blender agent was still rebuilding | Import only after the Blender build has finished. |
| `Start-EditorMcp.ps1` sits in its `Build.bat` step for a long time | The build waits on another worktree's UBT (the queue can take about 50 min) | Expected. If the module is already built (for example you ran `Build.bat SurvivalGameEditor ... -WaitMutex` yourself), launch with `-SkipBuild`. |
| A "Profile Data Visualizer" window pops over PIE and spoils captures | An editor hotkey (unidentified) opened it mid-run | Close it with `WM_CLOSE` to its window (find it with `EnumWindows` on the editor PID). |
| A 4K screen grab of the packaged game captured another session's editor | Matching the window by size; other sessions' maximised editors are also about 3840 wide | Match the window by process image (`QueryFullProcessImageNameW` contains `JennysHomesteadGame`). For 4K launch `-ResX=3840 -ResY=2160 -fullscreen`; a 4K `-windowed` window doesn't fit the 175%-scaled desktop. A 3840x2160 PNG is about 11 MB, over the `view` tool's 10 MB limit: downscale or crop it with PIL before viewing. |
| A teleport lands in the air or underground; traces return None | That World Partition cell isn't streamed, so there's nothing to trace | Take Z from the heightmap: `(v - 32768) / 128` m, where `v = a[y_m + 2016, x_m + 2016]` of `Scripts\Terrain\Estate_Heightmap_4033.png` (row = +Y, column = +X, metres from the map centre). This matches the estate anchors exactly. |
| After `BugItGo` she walks through walls and counters, sinks knee-deep into floors, or, holding W, flies level and goes under the terrain where the road climbs, so "Recovered the character above the generated terrain" fires over and over (looks like missing landscape collision) | `UCheatManager::BugItWorker` calls `Ghost()` (`CheatManager.cpp:1074`): flying, no collision | Send `Walk` straight after every `BugItGo`, including in scripted walk drivers. |
| Keys posted to the packaged game's console don't type | `WM_CHAR` isn't picked up there | Send a `WM_KEYDOWN` per character: VK = the uppercase letter, `-` 0xBD, `.` 0xBE, space 0x20; backtick (0xC0) opens the console. (`[GameWin]::Key` in `Scripts\GameWindow.ps1`.) |
| An Explorer window pops up on Jenny's desktop during a perf run | `startfpschart`/`stopfpschart` opens the `Saved\Profiling\FPSChartStats\<timestamp>` folder on every dump (`t.FPSChart.OpenFolderOnDump`, default on) | Don't use FPS charts; use `stat unit` / `ProfileGPU` log output or csvprofile. If you must, set `t.FPSChart.OpenFolderOnDump 0` first. |
| Frame times swing wildly between runs (for example render thread 20 ms vs 97-118 ms) | Other editors or UBT/`cl.exe` builds were running | Measure only inside a perf window: `Start-PerfWindow.ps1` checks one Unreal process and no builds, and holds off other launches. |
| `Start-EditorMcp.ps1`: "Perf window held by <worktree>" | Another session is measuring performance (`E:\CopilotScratch\homestead-perf.lock`, under 20 min old) | Wait for its `Stop-PerfWindow.ps1` or for the lock to go stale (20 min). `-Force` overrides; don't use it just to skip the wait. |
| `UnicodeEncodeError: 'charmap' codec can't encode` from Python output | The console is cp1252 | `$env:PYTHONIOENCODING='utf-8'`, or write to a file. |
| `Tests\HomesteadMenuSourceTests.py`: 9 failures, 1 error | Pre-existing on `main` (2026-09-27) | Compare against `main` before assuming you broke it. |

## 1. Is the server up?

```powershell
. .\Scripts\McpHelpers.ps1 -Port <your port>   # sets $env:UNREAL_MCP_URL for this process
python Scripts\editor_mcp.py call list_toolsets
```

- Toolsets listed (including `homestead_agent.toolset.HomesteadPlayTools`): ready, go to step 3.
- `Cannot reach Unreal MCP`: start it (step 2).
- Your worktree may already have an editor running (`Get-CimInstance Win32_Process -Filter
  "Name='UnrealEditor.exe'" | select ProcessId,CommandLine`). The command line shows the project
  path and `-ModelContextProtocolPort`. Reuse your own editor; don't touch other worktrees' editors.

## 2. Start the editor with MCP

```powershell
pwsh -NoProfile -File .\Scripts\Start-EditorMcp.ps1 -Port 8768 -AllowPython            # builds SurvivalGameEditor first
pwsh -NoProfile -File .\Scripts\Start-EditorMcp.ps1 -Port 8768 -AllowPython -SkipBuild # module already built
```

- Opens the visible editor on the startup map (`/Game/SurvivalGame/Maps/Estate`), enables the
  MCP and toolset plugins for this process only, disables background CPU throttling (so PIE runs at
  full speed while another window has focus), turns Live Coding and ray tracing off, and returns
  once `http://127.0.0.1:<port>/mcp` answers (localhost only). Options: `-Port`, `-Map` (not needed;
  see section 0), `-TimeoutSeconds` (default 600; use 1200 after a build), `-RayTracing`,
  `-ExtraPlugins`, `-AllowPython`, `-EngineRoot`.
- The editor keeps running after the script returns.
- A fresh worktree has no `Binaries\`; the build step compiles the editor module (2-5 min, longer if
  another worktree is building).
- A cold start can take more than 10 minutes. Quiet is not hung; check `Saved\Logs\SurvivalGame.log`
  and table 0.1 (the `ValidatePlatforms` hang).
- C++ changes: quit the editor (it locks its module DLLs), run the build (the script's build step,
  or `Build.bat SurvivalGameEditor Win64 Development "-Project=<worktree>\SurvivalGame.uproject"
  -WaitMutex -NoHotReloadFromIDE`), and relaunch with `-SkipBuild`. Live Coding is off in agent
  editors because it blocks other worktrees' builds.
- `-ExtraPlugins A,B` enables more engine plugins for the session (for example
  `MetaHumanCharacter,MetaHumanSDK,MetaHumanCoreTech,MetaHumanGenerator`).
- `-AllowPython` registers `homestead_agent.toolset.HomesteadEditorPython.run_python`, which runs
  arbitrary editor Python (full `unreal` API; print output and an optional `result` value are
  returned). Opt-in because it is unrestricted code execution on the localhost server; use it for
  editor automation that Epic's toolsets don't cover (for example the MetaHuman Creator subsystem).

## 3. Choose how to call tools

- **Shell helpers (default on this shared machine):** dot-source `Scripts\McpHelpers.ps1 -Port <p>`
  at the start of each command (shell state doesn't persist between calls). It uses
  `Scripts\editor_mcp.py`, which saves images in results to `Saved\McpCaptures\*.png` and replaces
  them with `<saved to PATH>`; open them with the `view` tool.
- **Native MCP tools** (`unreal` server from `.github\mcp.json`): fixed to port 8765 and available
  only if an editor was serving there when the Copilot session started (check with `/mcp`). Use
  them only when your own editor is the one on 8765 (section 0).

The server exposes three meta-tools (tool search mode):

| Tool | Arguments |
| --- | --- |
| `list_toolsets` | none |
| `describe_toolset` | `toolset_name`; returns every tool with its JSON input schema |
| `call_tool` | `toolset_name`, `tool_name` (short name), `arguments` object |

Run `describe_toolset` before first use of a toolset; required arguments are enforced strictly.

`Scripts\McpHelpers.ps1` defines the helpers used throughout this skill. It writes each call's
arguments to its own file under `Saved\McpArgs`, which avoids PowerShell quoting problems and
clashes between parallel callers.

| Helper | Does |
| --- | --- |
| `mcp <toolset> <tool> [json] [timeout]` | `call_tool`, raw JSON result |
| `hk <tool> [json]` | a `HomesteadPlayTools` **tool** call by tool name (`hk walk_to '{...}'`, `hk tap_key '{"key":"E"}'`), not a hotkey; returns its text |
| `st [nearby]` | parsed `get_play_state` |
| `py <code>` | `run_python` (needs `-AllowPython`), returns the output text |
| `con <command>` | console command in PIE with the player controller (editor world outside PIE) |
| `shot` | `CaptureEditorImage`, returns the PNG path |
| `hshot [WxH]` | `HighResShot` in PIE, returns the new `Saved\Screenshots\WindowsEditor` PNG. Reliable for the world and the Canvas HUD, but **Slate widgets (hotbar, `SHomesteadVitals`, the field book) aren't in it**: check UI with `shot` or a standalone `[GameWin]::Capture` |
| `pie` / `unpie` | start PIE in the viewport / stop it; poll `st` for `worldReady` |
| `quit` | stop PIE and quit the editor cleanly (releases DLL and `.uasset` locks) |
| `pyfile <path>` | run a Python file in the editor with `__file__` set |
| `tp <x> <y> [z]` | move the player pawn (z 200 drops her to the ground) |
| `click <x> <y>` | real Win32 left click at editor-window pixels; Slate clicks don't reach game widgets. Slate `Snapshot` positions are relative to the client area, so add the window chrome (about 12 px). To click something seen in a `shot` capture, scale capture pixels by client width / capture width and add the window-rect origin: with a 3840-wide client, a 1280x692 capture and the rect at (-12, -12), `x = cap_x * 3 + 12`, `y = cap_y * 3 + 12` |

Toolset variables: `$McpEditor` (EditorAppToolset), `$McpScene` (SceneTools), `$McpLogs`,
`$McpSlate` (SlateInspector), `$McpPlay` (HomesteadPlayTools), `$McpPython` (HomesteadEditorPython).
The short aliases `$E`, `$S`, `$L`, `$SL`, `$H`, `$PY` used below are set only when you haven't
already defined those names. PowerShell names are case-insensitive, and dot-sourcing never overwrites
your own `$s` or `$e`; if you have one, use the `$Mcp*` names instead. Keep session-specific helpers (probes,
callbacks) in your own files and load them from `py` with `sys.path.insert`.

## 4. Play the game

### Start and stop

```powershell
mcp $E StartPIE '{"options":{"bSimulate":false,"playMode":"PlayMode_InViewPort","warmupSeconds":5}}'
do { Start-Sleep 3; $s = st } until ($s.worldReady)   # StartPIE may report a timeout; poll instead
...
hk release_all; mcp $E StopPIE
```

- On the **Homestead** (woodland) map, every PIE start with no save in this checkout generates a
  **new woodland seed**. Once an autosave exists, PIE resumes it (same position/time), so positions
  and node ids persist. The **Estate** map is a fixed world: same layout every time.
- **HUD layout (Estate, per Jenny):** minimap bottom-right (`MinimapBox(ViewWidth, ViewHeight)`),
  calendar top-right, key hints top-left, and the vitals stack bottom-left (`UI/SHomesteadVitals`):
  bread, bed and coin icons with bars for food and energy and the $ amount for the purse, with no text
  labels. Check captures against this; the field book covers it when open.
- **Estate PIE recipe:** the editor opens the Estate at startup (if you've switched away, `load_level`
  it back). For a fresh start move `Saved\SaveGames\Estate\*` into a dated backup folder; `pie`; poll `st`
  until `worldReady`; close the Appearance/Names book (B or Escape; check `bookOpen`) before
  captures. Estate saves go to `Saved\SaveGames\Estate\`, and a leftover `*.tmp` there means a
  failed save. Text in a `.sav` (such as the save label) is UTF-16, so search it with
  `[Text.Encoding]::Unicode`, not as ASCII. **Packaged Estate runs resume too:** after the first run the build loads its own
  estate save (inside the package's `SurvivalGame\Saved\SaveGames`), so the Appearance → Names setup
  only shows on the first run. Move that save aside to see it again. On a resumed game, Enter by the
  hearth opens Cook (the Craft page). Code written for the woodland can still assume woodland heights (estate ground is
  about 87-95 m, Z ≈ 8700-9500) and views; see table 0.1.
- **Sleep tests on the Estate:** the manor bedroll is next to (-25500, -63720), z about 8800.
  `Sim.Sleep` needs a bed within reach; the E prompt reads "Sleep 8 hours". Store testing
  shortcuts (`HomesteadOpenStore` and the counter snap-back on reload) are in
  `docs\general-store-playtesting.md`.
- **Free-camera PIE stills (Estate):** before PIE, spawn a `CameraActor` in the editor world with
  `is_spatially_loaded = False` (a spatially loaded one isn't streamed into PIE). In PIE, call
  `set_view_target_with_blend` on the player controller, and park the pawn near the camera so the
  land around it streams in (table 0.1). `HomesteadMorning <h>` with `h` earlier than the current
  hour rolls to the next day, which can re-roll the weather to Rain; restart PIE for comparable day-1 stills.
- **LB/RB outside the book change the hotbar slot**, not book pages. Open the book first:
  `I` opens Inventory (page 0), Menu opens Settings (page 4).
- The field book opens on the Guidebook at start. Close it with B (`Gamepad_FaceButton_Right`).
  On the Estate map PIE also opens with the book (reported on Appearance, page 6, for the names
  step); close it (Escape or B) before captures.
- An editor "Missing Project Settings / Shader Model 6" notification covers the view after launch.
  Dismiss it: `mcp $SL Snapshot '{"ref":"","maxDepth":12,"bIncludeSourceLocations":false}'`, find
  `button "Dismiss" [ref=bN]`, then `mcp $SL Click '{"ref":"bN"}'`.

### HomesteadPlayTools

Input goes through `APlayerController::InputKey` as simulated events (the same path as the game's
own playtest harnesses), so it reaches gameplay and the native menu. All input tools return
immediately; observe with `get_play_state` and `shot`.

| Tool | Use |
| --- | --- |
| `get_play_state(nearby_count, radius_cm)` | JSON: `worldReady`, `bookOpen`, `bookPage`, `selectedRow`, `focusTitle`, `focusActions`, `toast`, `inventory` (units), `hunger/energy`, `hour`, `location`, `controlYaw`, `speedCmPerSec`, `hotbarSlot`, `walk`, `nearbyResources[]` (`id, kind, x, y, distanceCm, bearingDeg, cleared, ready, focused`) |
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
- `walk_to` reports `arrived` about 150 cm short of the target and stops in doorways; check
  `location` and walk again, or finish with a short `set_sticks`.
- Trees block movement, so `walk_to` a tree usually ends `stuck` at the trunk. That's fine if the
  tree is `focused`.
- Depleted nodes stay focusable with no actions (`cleared: false`, `ready: false`) and can steal focus.
  Filter targets by `ready`.
- `nearbyResources` covers only the currently streamed chunks. After walking far, nodes elsewhere
  (for example creek reeds) drop out of the list; walk back toward them to reload.
- `focusActions` shows the prompt, for example `[A] Gather`, `[RT] Hack with Billhook`,
  `[RT] Fell with Axe`, `Select the scythe`. Use the named button. With keyboard/mouse input last
  used it reads `[LMB] Hack with Billhook`; `tap_key LeftMouseButton` works then. The same text
  floats above her head as the interact cue. `focusTitle` names a missing tool or tier, e.g.
  `Bramble thicket  (needs an iron billhook)`.
- **Stick to one input device per sequence.** The prompts, and which keys act, follow the last
  device used. After any keyboard `tap_key` (even a hotbar key such as `Three`) they switch to
  keyboard (`[LMB] Fell with Axe`): then `tap_key LeftMouseButton` works, while
  `Gamepad_RightTrigger`, `Gamepad_RightTriggerAxis` and `E` silently do nothing. After gamepad taps,
  `Gamepad_FaceButton_Bottom` works and `E` doesn't. To swing from a gamepad sequence, pick the tool
  with the D-pad or `hotbarSlot` rather than number keys.
- **Test clearing anywhere without the salvage route:** `HomesteadGive Scythe 1` or `HomesteadGive Billhook 1`
  (console, with the player controller) grants the tool directly. Then tap `Three` or `One` and
  `LeftMouseButton` (keyboard mode throughout). Verified on the derelict farm's 550000+ weeds and a thin bramble.
- **Tool swings are refused while she's moving.** Any velocity or acceleration blocks the action
  in the anim instance, so an action key right after `set_sticks` or `walk_to` is silently dropped.
  `release_all`, wait until `speedCmPerSec` is 0, then act.
- Hotbar keys are `One`..`Zero`. The default layout is billhook, axe, scythe, pickaxe, hoe, pail
  (slots 1-6) and berries (7); the knife and machete are retired. On the woodland the playtest kit
  grants all six tools; on the fixed estate she hafts them from salvage (a hafted tool is slotted
  and selected). Check `hotbarSlot` (0-based) and `focusActions` after selecting.
- Overgrowth (add-overgrown-estate-clearing): stumps, rocks and thickets take several swings; each
  non-final swing toasts "N more swings.", the last one clears and toasts the yield. Walking more
  than 1.5 m away resets the count. On the estate, take the Branches from the standing-room chest
  (below). `HomesteadGive RustedBillhookHead 1` + two Branches is only a shortcut for testing
  "Haft a billhook" elsewhere (for example on the woodland map).
- **Estate tool route (new game, verified in PIE on main 17a64854):**
  - Setup: PIE opens Appearance. `Gamepad_FaceButton_Right` closes it and shows Names. Press
    `Gamepad_DPad_Down` three times to reach Begin, then `Gamepad_FaceButton_Bottom`. She stands in
    the standing room at about (-25750, -63800). To leave on foot, `walk_to` (-25750, -64300) and
    then (-25750, -64900).
  - Salvage piles: 520001 (-25600, -64350), 520002 (-25450, -65250), 520004 (-23950, -65150),
    520003 (-24600, -66200) and 520005 (-26100, -66100). Search each with `E`/A.
    - Each search gives the next missing head plus 1 scrap iron, in the order billhook, axe,
      scythe, pickaxe, hoe. The order follows your search order, not the pile.
    - **All five are reachable on foot** (checked with `walk_to` waypoints and a capsule-swept
      reachability grid). Walk these waypoints in order (world cm), then check `focusTitle` is
      "Salvage pile":
      - 520001: (-25725, -63950) → (-25725, -64150) → (-25580, -64300)
      - 520002: (-25600, -65400) → (-25450, -65250)
      - 520003: (-25225, -64150) → (-24650, -65075) (the cross-wall gap) → (-24650, -66100)
      - 520004: (-24300, -64400) → (-24300, -65050) (the rear gap) → (-23950, -65100)
      - 520005: (-25650, -66000) → (-25650, -65450) (it snags briefly on the door rubble at about
        y -65590 but gets through) → (-26100, -65450) → (-26100, -66000)
    - A straight `walk_to` to 520003 or 520005 gets stuck on the ruin walls; use the waypoints.
    - **Close the field book before walking.** A by the hearth opens the Cook page, and `walk_to`
      then reports `stuck` without moving.
    - If `walk_to` doesn't move her after a Python `set_actor_location` teleport indoors (she stays
      frozen at the teleport Z), set the movement mode back to `MOVE_WALKING` and teleport again about
      60 cm higher.
  - The first haft needs no trip outside: the standing-room chest (about 200 cm +X of the
    spawn; `walk_to` (-25560, -63950)) holds the pail and 4 Branch. Open it with A, pick the
    Branch tile, then Y > "Take to pack". Then search 520001 and haft the billhook (Craft tile 4,
    hold Enter about 3 s). On foot, go (-25750, -64900), then (-25650, -65450), then
    (-25950, -65450), and the focus reads "Thin bramble [RT] Hack with Billhook" in the doorway
    (verified in PIE on 31810d1d). If PIE opens on Settings with "Saves from earlier test builds
    can't be opened", move `Saved\SaveGames\Estate\*` aside and restart PIE.
  - Branch piles (each haft takes 2 Branch; a pile gives about 5): 500001 (-26650, -63400), 500002 (-26950, -64500)
    and 500003 (-27350, -62900).
  - Craft (`C`) tiles, left to right: Haft an axe, hoe, scythe, billhook, pickaxe, then the two root
    dishes and Split firewood. Hold `Gamepad_FaceButton_Bottom` (or `Enter`) about 3 s to haft.
  - Hafted tools auto-slot to the hotbar: `One` billhook, `Two` axe, `Three` scythe, `Four`
    pickaxe, `Five` hoe (`hotbarSlot` 0-4).
  - Worn-tier targets near the manor:
    - thin bramble 510001-510007 just outside the fallen front door's 3 m gap, X -26000..-26380, Y -65640..-65270. `EstateManorFrontDoor()` is (-25900, -65450). Walk out from (-25750, -64900) to (-25650, -65450), then south to (-26000, -65450);
    - three thin bramble outside the rear north-wall gap (-24100, -65150), at (-24040, -65290), (-24040, -65010) and (-23830, -65020), flanking salvage pile 520004;
    - forecourt tall grass and weeds 510008-510037, around (-26450, -62100);
    - rubble 510040 (-26550, -65800) and a small rock 510043 (-25150, -66600);
    - a two-swing sapling 510076 (-30592, -61863);
    - a three-swing small stump 510086 (-30981, -63104);
    - a fallen bough 510062 (-24272, -61097).
  - Iron-tier prompts: thicket 510087 (-28972, -64048), boulder 510095 (-23350, -67000), large
    stump 510091 (-33451, -62959) and fallen log 510092 (-33729, -59833).
  - Approach by teleporting 260 cm short of the target, then `walk_to` it with a 10 cm stop.
    Tapping during the axe's ~3 s recovery is dropped, so wait about 3.5 s between axe swings.
  - The scythe arc reaches only about 1.6 m ahead ("Step closer to mow.").
- An action that silently does nothing usually left a reason in `toast` (`toastIsError: true`),
  for example "Not enough pack space." when a felled tree's wood won't fit. Read it before
  debugging the animation.
- A clean world for a playtest: stop PIE, move `Saved\SaveGames\*` into a dated backup folder,
  start PIE again (new game at 06:00 with the starter kit; the field book opens, close it with
  `Gamepad_FaceButton_Right`). `HomesteadMorning <hour>` (pass the player controller) only moves
  the clock, so it costs no energy; night playtests are too dark to judge.
- `HomesteadGive <Item> [count]` (console, pass the player controller) adds to the pack by item
  name, spaces optional: `HomesteadGive Berries 10`, `HomesteadGive RoastedRoots 3`. The toast
  says what was added or why not (full pack, unknown name).
- Energy: work costs it (gather 0.5, fell 4, till 2, overgrowth 0.3-6 by kind and tier, build 1.5; the full tables
  are `Homestead::Exertion` and `HomesteadOvergrowth.cpp`), time awake drains only 0.6/game hour.
  Below a 5-Energy reserve work is refused with "You're too exhausted to keep working." Eat or
  sleep before a long test loop.
- Eating (MetaHuman): food in the hotbar (Berries start in slot 6; the field book's pack page
  has Pin to hotbar / Unpin from hotbar on any food or tool) is eaten with LMB/RT. She reaches
  into the hip pouch, pinches a bite (the berry shows in her fingers from 0.63 s to the bite at
  1.33 s), eats and chews; it layers over walking. Clicks while she's eating are ignored.
  The clip is authored by `homestead_agent.eat_berry` (`eb.build()` in the editor, not PIE;
  `eb.report()` prints the pinch-to-mouth distance). For a close look, raise the spring arm's
  `target_offset` z to 50-55 with arm length 130-150, face her away from the sun (yaw 180 in the
  generated woodland around 13:00) and use `slomo 0.25`.
- Felling a tree (MetaHuman, hatchet selected): she walks up to the trunk if the felling stance
  is more than 35 cm away, eases into it, swings three strokes (a sapling takes one), and the tree
  topples away from her, bounces once, lies 4 s and sinks. Each stroke plays one of
  `Audio/Effects/ChopA..C` and the landing plays `TreeFall` (all cut from CC0 recordings by
  `Scripts\generate_chop_sounds.py`). `LogTemp` Verbose lines starting with
  `Fell:` give the trunk centre, radius, stance and where the walk-up ended
  (`log LogTemp Verbose` to see them). To check contact, sample the hatchet prop's edge,
  `Held_SM_FlintHatchet` transformed at local (-0.21, -13.89, 43), a few times a second during the
  swing; it should come within the trunk radius of the centre at about 95 cm above the ground.
- Gardening (MetaHuman): the garden grid is 1 m squares (`Homestead::GardenCellSize`, 3 x 3 per
  3 m building cell; `Plot.cellX/Y` are garden coordinates, so use `PlotCenter(Plot)` or
  `GardenCellCenter`, never `CellCenter`). With the stone hoe (slot `Three`), LMB/RT turns the one
  square whose cell holds the point 85 cm ahead of her, or weeds it if already tilled. She lifts
  the hoe overhead and chops twice, and `SM_TilledBed` appears on the first bite. On a bare
  tilled square the focus offers `[E] Plant roots` / `[F] Plant berry seeds` (gamepad A / X):
  she kneels, presses the seed in with a pinch and scoops loam over it, and `SM_SoilMound` shows
  on the bed. A square with moisture of 0.4 or more uses `MI_TilledBed_Wet`. To find beds from
  Python, look for static mesh components whose mesh name contains `TilledBed` on
  `HomesteadWorld_0`.

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

- Menu (or Escape) opens the book on **Settings** (page 4), which has no tab bar and isn't in the
  LB/RB cycle. LB/RB cycle the tabs `FieldBookPages` (`HomesteadController.cpp`): 0 Inventory,
  1 Craft, 2 Build, 7 Map, 3 Guidebook, 6 Appearance, wrapping (`I` opens Inventory and `M` opens
  the Map directly).
  Loop LB until `st().bookPage` is the page you want. B closes/backs out.
- **Settings** is a centred single column: Resume (focused on open), a top row Save | Load latest
  save | Quit game, Game/Sound/Video tabs, then the current tab's list. D-pad Down from Resume
  enters the top row (Left/Right moves across it); Down again lands on the active tab, where
  Left/Right switches tabs; Down again enters the list, where Left/Right changes a value and A
  toggles. Up from the first list row returns to the tabs. Sound holds the volume sliders; Video
  holds resolution scale and vertical sync; everything else is on Game. Verify the selected row
  visually before activating.
- "Start a new woodland" opens a Cancel-default confirmation; B backs out safely.
- **Inventory**: there is no details pane or action-button column on the pack/chest page. A on an
  item starts a *move*; Y (or F) opens the item's context menu, where D-pad + A picks an action
  (`Eat 1`, `Drop 1`, `Move to chest N`, ...). X splits in half, S sorts. RT cycles content and
  equipped slots only. Shift+Enter is the keyboard Shift+click (quick move / pin / wear).
- **Craft**: recipes sit in a horizontal row, so D-pad **Right/Left** moves between them (Down
  doesn't). The details list requirements. Crafting is **press and hold**; a tap does nothing
  (`hold_key Gamepad_FaceButton_Bottom 3`, or `hold_key {"key":"Enter","seconds":2.5}` on keyboard).
- **Build** (page 2) is a grid of plans, not a list: Right moves from Foundation (row 0) to
  Wall (row 1), and Up/Down jump between row 0 and Chest (row 6). `B` reopens the book on its
  *last* page (often Inventory), so close it fully (loop Escape until `bookOpen` and `planning`
  are both false) and press B again. Enter on a plan enters the preview; it sometimes takes two
  presses, so check `st`. In the preview she walks freely. The piece sits about 350 cm ahead
  along the camera yaw, so aim with `pc.set_control_rotation(...)`, then `E` places and `R`
  rotates. Pieces snap to the nearest open side, edge or cell of a building within reach, and
  aiming mid-floor snaps a foundation to a free side. A red preview means blocked, and the
  planning panel's second line says why (for example "Gather 2 Branch first."). Top up with
  `HomesteadGive Branch 20` / `Fiber 12` / `Stone 8`.
- Feedback messages ("Ate Berries.", "Made Crude hatchet.") appear as a banner on the book and in
  `toast` briefly. World-side hints (for example "Craft a crude hatchet before felling trees") may be
  visible in captures without appearing in `toast`, so capture after actions.
- **Right-click and Ctrl+click popups**: `HomesteadPackMenu <tile> [mode]` (console, pass the
  player controller, book on the pack page) opens the right-click context menu (mode 0) or the
  Ctrl+click amount slider (mode 1) on pack tile `<tile>` (0-based), anchored at the tile. Clicks
  and keys go to the popup until it closes; Escape or B cancels without changing anything.
  `HomesteadPackMenu 0 2` opens the nearest storage chest (and logs its position) when she is
  within 280 cm of it, which is the quickest way into the Storage page in PIE. A synthetic
  Shift+click in automation needs a `ProcessMouseMoveEvent` onto the tile first, or Slate never
  delivers the click.
- **Portrait**: the pack page's left column is a live cut-out capture of the real heroine (two
  SceneCapture2Ds on her own components, lit only by lighting channel 2 while the book is open),
  so it shows her current clothes and held tool. It is not a copy of her mesh; if it shows
  scenery, check `M_PortraitCutout` and the coverage render target.
- **Action hints retire**: each keyed cue ("[A] Gather", "[LMB] Fell with Hatchet") hides after
  three successful uses. Counts live in `Saved\Config\WindowsEditor\GameUserSettings.ini` under
  `[Homestead.ActionHints]` (id = verb + `_` + resource kind, alphanumerics only, e.g.
  `Gather_Berrybush=3`). Delete the section or use Settings > "Show action hints again" before
  a playtest that needs to see the cues.
- **Appearance** (page 6) is a compact vertical list (no Grid): Hairstyle chips, then Hair
  colour / Skin / Eyes swatch strips. A click on a chip or swatch applies it immediately
  (`MenuSetAppearance`); D-pad Left/Right steps the highlighted row. Selecting the Eyes row
  (id 3) eases the camera to her face. While the preview is open the character raises the near
  clip plane to ~60% of the camera distance (restored on close/EndPlay) so no-collision branches
  can't block the view.
- **MetaHuman eye colour** only works because `MI_EyeL/R_Homestead` have `Use Baked Material`
  False and the static switch `Use Custom Iris` True; without that switch the iris hue/value
  scalars do nothing (`Iris Color Multiply` never shows). Iris hue 0 = blue, 0.5 = green,
  0.9 = amber-brown. Skin tone uses `Basecolor Global Multiply Post-Bake` on face and body MIDs.
- **Screen capture of the book**: run PIE with `PlayMode_InEditorFloating` and use a Win32 window
  grab; `CaptureEditorImage` misses the floating window. Real clicks (Win32 `SetCursorPos` +
  `mouse_event`) land on Slate normally. Save assets (`save_asset`) only with PIE stopped; it
  returns False during PIE.

Settings changes persist ("Choice saved in game settings") in
`Saved\Config\WindowsEditor\GameUserSettings.ini`, so restore anything you change.

## 5. Other recipes (verified)

```powershell
mcp $S get_current_level
mcp $S load_level '{"level_path":"/Game/SurvivalGame/Maps/Homestead"}'
mcp $L GetLogEntries '{"category":"","pattern":"LogHomestead|Error","maxEntries":40}'   # pattern is a required regex
mcp $E CaptureViewport '{"captureTransform":null,"annotations":null,"bShowUI":false}'   # editor camera, not the game
con 'HomesteadMorning 8'                                        # console command with the player controller
```

### Character lab (model and animation iteration)

An endless flat grid floor with the real heroine: the same character, camera, input, anim graph and
foot placement, but no simulation, woodland, menus or saves. It doesn't touch Jenny's saves.

- PIE: `py "unreal.SystemLibrary.execute_console_command(None, 'homestead.CharacterLab 1')"`,
  then `StartPIE`. Set it back to 0 for the woodland. Packaged: `CharacterLab.cmd`, or
  `-HomesteadCharacterLab`.
- `get_play_state` reports `"characterLab": true`; sticks, keys and `walk_to` work as usual.
- Console: `LabAction Gather|Sticks|Stones|Roots|Berries|Reeds|Eat|Water|Chop|Knife|Till|Machete|Fell`,
  `LabHold Knife|Hatchet|DiggingStick|Pail|Machete|None` (the hand-carry prop for that tool, as
  when it's selected on the hotbar),
  `LabProp Sticks|Stones|Roots|Berries|Reeds|None` (puts that pile on the ground in front of her, the way the
  woodland does), `LabLoop <action>|Off` (replays the action every few seconds from the same
  spot with a fresh pile, so Jenny can watch it repeat), `LabSun <hour>`, `LabCourse`
  (10/20/30° ramps and 10/20 cm steps at x = 2500), `LabTeleport <x> <y>`, `slomo 0.25`,
  `homestead.FootPlacement 0|1`.
- **Pass the player controller** for the lab exec commands, or they silently don't run:
  `w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world();
  unreal.SystemLibrary.execute_console_command(w, 'LabLoop Sticks', unreal.GameplayStatics.get_player_controller(w, 0))`.
  CVars such as `homestead.FootPlacement` work without it.
- The anim instance drops an action requested in the same frame as `CancelAction`, while the
  cancel is still blending out. Wait a moment before replaying; `LabLoop` waits 0.25 s.
- The HUD shows speed, gait/action weights, foot placement state, frame time and sun hour.
- Framing for animation reviews: keep her whole body in view, head to feet, so Jenny can judge
  the full pose. Set the `SpringArmComponent` `target_arm_length` to about 300 and `target_offset`
  to (0, 0, -35), then aim with `pc.set_control_rotation`. For a front view use a control yaw of
  her yaw + 180 ± 45 and a pitch of about -18. Tighter shots (200 or less) crop her legs and head
  when she kneels; use them only for a specific close-up, and share the full-body view too.
- Contact sheets of an action: `hshot` (HighResShot) takes about 3-4 s per still, so slow the action
  with `slomo 0.08`-`0.1` to get several frames across a 3 s clip, or record video (below). Material
  `Time` follows the dilation too (`slomo 0.3` gives about 1.1 s of game time per still).
- `homestead_agent.prop_clearance`: `start()`, play the action, then `print(stop())` reports the
  worst clearance per carried stick and body part in PIE (negative cm = inside her).

### Author an animation with the MetaHuman Control Rig

`homestead_agent.rig_authoring.Session` builds a level sequence with the heroine body and her
`MetaHuman_ControlRig`, keys controls, and bakes an AnimSequence. To author a new clip, copy the
closest existing clip module in `Content\Python\homestead_agent\` and run its `build()` in the editor
(not PIE): `active_idle`, `axe_fell`, `eat_berry`, `hoe_till`, `kneel_gather`, `kneel_plant`,
`kneel_pouch`, `kneel_reeds`, `machete_hack`. Their docstrings describe the timing contracts with
C++. Details that cost time to find:

- **Plan poses within her reach before keying.** In authoring component space (forward +Y, her
  left +X, up +Z) the shoulders sit at about (±15, 0..11, 140) and shoulder to wrist is about 50 cm.
  Two-handed holds need the hands' span to fit the object (a pail pour designed for a 33 cm span
  was unreachable; the hands reached only 16-22 cm). The bake report's IK error is the only signal.
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
  baked clip with `bone_positions`, then key and bake again. `kneel_gather.build()` bakes four
  passes, so the right hand can rest on the bundle the baked left arm carries, lined up with the
  forearm from the pass before. Each pass takes about 5 s.
- Check wrists as well as positions. The angle between `lowerarm_r→hand_r` and
  `hand_r→middle_01_r` in the baked clip should be under about 25° for a relaxed hand. A fixed
  finger direction can leave it at 70-80°, which Jenny sees as sharp. Aim the fingers along the
  forearm instead.
- `kneel_gather.clearance(anim)` reports per-frame forearm-to-spine and forearm-to-thigh
  distances. Use it to catch an arm passing through her body before you look at captures:
  under about 16 cm to the spine, or about 11 cm to the thigh, means intersection.
- Lab `LabAction Sticks` plays the kneeling stick gather with the stick props.
- `LabAction Stones` reuses the stick clip with two stones (palm grip, then nested along the left
  forearm). `LabAction Roots|Berries` plays `AN_HeroineMH_KneelGatherPouch` (`kneel_pouch.py`,
  two-pass bake keyed in the pelvis frame): each pickup is pinched in the right hand and slipped
  into the hip pouch at `POUCH_OPENING`. Pick/stow frames live in `POUCH_EVENTS` and must match
  `GatherPouchTiming` in `HomesteadCharacter.cpp`.
- `LabAction Reeds` (and `LabProp Reeds`) plays `AN_HeroineMH_KneelCutReeds` (`kneel_reeds.py`,
  `kr.report()` prints the left fist and blade middle against the stems): she kneels, closes her
  left fist on the stems 40 cm up, saws through them just below it with the flint knife (blade out
  of the thumb side, edge toward the stems), then rises holding the cut bundle (`CarriedReeds`, the
  clump mesh narrowed) in her left fist. The knife shows in her right hand throughout with the
  wrist carry off; the left hand closes through `SetLeftHandGrip`. `EVENTS` must match
  `GatherReedsTiming`. The stems stand at `kneel_reeds.STEMS` (34 cm ahead, 10 cm to her right,
  clear of the forward knee); the C++ settle uses the same offsets. The arms can't reach lower
  than about 30 cm while kneeling, so keep grasp and cut heights around there.
- A bake (`rig_authoring`, `craft_hands`, ...) leaves a `HeroineRigAuthoring` actor in the level and the
  Sequencer open. Clean up afterwards: `unreal.get_editor_subsystem(unreal.EditorActorSubsystem).destroy_actor(a)`
  and `unreal.LevelSequenceEditorBlueprintLibrary.close_level_sequence()`, and don't save the level.
- Never bake a clip while PIE is running: the bake opens a hidden "Overwrite Existing Object"
  modal behind the PIE window that doesn't take input and blocks MCP. Stop PIE first; if it
  happens anyway, `Stop-Process` the editor by PID and restart it.
- `LabAction Machete` plays `AN_HeroineMH_MacheteHack` (`machete_hack.py`): a wide overhead
  forehand, then a backhand, with the Blender `SM_Machete` in her right hand. The rig's finger
  controls don't key usefully, so the fist is built at runtime in `FHandGrip`
  (`HomesteadAnimInstance.cpp`): it curls the right-hand fingers and applies a wrist "carry"
  (46° ulnar deviation about the palm normal) that tips the blade down and forward at idle.
  `Carry` fades to 0 as the hack clip blends in. The prop attaches with `HandGripTransform`
  (0.78 of the way from hand to middle knuckle, plus 2.6 cm along the palm; the blade runs
  along +Z with the edge toward −Y).
- `FindUnderbrushNear` takes the `Simulation` and skips cleared plants. Without that, a cleared
  bramble can shadow a live one, or keep showing as focus until the cover rebuilds.
- Loose stones use the Blender `HandStones` set (`SM_HandStone_A-C`) when imported; until then
  they fall back to the `MossRocks` cluster, which is an 8 m group of rocks, so scaling it down
  reads as a scatter of pebbles, not one stone.

### Tune hair motion live

Her hair groom (`MetaHumanHair`) uses the Niagara strands solver, which reads the groom
component's `simulation_settings` every tick, so edits in PIE show up immediately:

```python
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
ch = unreal.GameplayStatics.get_player_character(w, 0)
hair = [g for g in ch.get_components_by_class(unreal.GroomComponent) if g.get_name() == 'MetaHumanHair'][0]
s = hair.get_editor_property('simulation_settings')
setup = s.simulation_setup; setup.linear_velocity_scale = 0.5; s.simulation_setup = setup
```

- Nested structs are copies: modify, then assign them back, then `set_editor_property`.
- `simulation_setup` velocity scales (0-1) cut the motion she passes to the hair and apply on
  their own.
- `override_settings` = True replaces the asset's drag, bend, stretch, friction and collision
  values with the component's (`external_forces.air_drag`, `material_constraints.bend_damping`
  and so on). It also needs `solver_settings.enable_simulation` = True, or the hair stops simulating.
- The shipped values are in `AHomesteadCharacter`'s groom setup (air drag 1.0, bend damping
  0.05, bend stiffness 0.15). Change them there after tuning live.
- The velocity scales follow her speed every tick (`UpdateHairMotion`), from walk values to
  sprint values, so edit them with the CVars rather than Python: `homestead.HairLinearWalk`
  0.5, `homestead.HairLinearSprint` 0.65, `homestead.HairAngularWalk` 0.4,
  `homestead.HairAngularSprint` 0.45.

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

A PIE screenshot that is always the right window: run the console command `shot showui`
(`con 'shot showui'`). It writes `Saved\Screenshots\WindowsEditor\ScreenShotNNNNN.png` with the
HUD, independent of which monitor or window is in front. With in-viewport PIE it captures the whole
editor window at native resolution (3840x2076 here), so crop the viewport yourself; `hshot`
(HighResShot) captures just the game view, without Slate UI. To judge foliage wind while she stands still, record about 6 s with
ddagrab, decode to grayscale at half size and look at the per-pixel standard deviation over
time (`v.std(0)`, scaled ×8); moving leaves light up, still ground stays black.

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
| Compile C++ into the running editor | Not in agent editors: quit, rebuild, relaunch (Live Coding blocks other worktrees' builds; section 0) |
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
  the skipped hours, so day-rollover logic (the store's sell-down, weed creep) doesn't run; only
  Advance or Sleep steps it. The same console command works in the packaged Development build. The
  character lab has its own sun (`LabSun <hour>`, default 10).
- Jenny likes to watch you work. Prefer `PlayMode_InEditorFloating` with the PIE window brought to
  the front (section 5) over hidden in-viewport PIE.
- If Jenny has the packaged game open from `Build\Windows` when the integration session needs to repackage,
  close it (`Stop-Process -Id <pid>` on the `SurvivalGame` processes) and build in place. She's only messing
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
- Measure only inside a perf window (section 0: one Unreal process, no builds, `Start-PerfWindow.ps1`);
  numbers taken alongside other editors or builds aren't comparable.
- Jenny's performance bar: the framerate must be **smooth**, not just high. Never report a
  performance result from average FPS alone. Check frame pacing on the `Playtest-Visual.ps1
  -PresentationDiagnostics` timing passes (median, p95, p99, max, frames over 20 ms and over
  33.3 ms), look for periodic spikes (for example, the paired 30-39 ms Virtual Shadow Map stalls
  that continuous sun rotation caused), and watch real walk/turn/camera sweeps in play for visible
  hitches. Test both capped (60 FPS) and uncapped. PIE timing is indicative only; smoothness
  sign-off needs the packaged build.

## 8. Build, package and test

- **Editor module:** close your editor, then `& 'E:\Program Files\UE_5.8\Engine\Build\BatchFiles\Build.bat'
  SurvivalGameEditor Win64 Development "-Project=<worktree>\SurvivalGame.uproject" -WaitMutex
  -NoHotReloadFromIDE` (2-5 min; it queues behind other worktrees' builds).
- **Native rules and persistence:** `Scripts\Test-Native.ps1 -Configuration Release` runs every
  CMake suite (Simulation, WorldGeneration, RegionalGeneration, Parcel, Economy, ...) in about
  3 min; Debug takes about 10. For one suite, build its target and run
  `Build\Native\Release\<Suite>.exe *> <log>`; stdout is buffered, so a crash loses unredirected output.
- **Package (integration session only during multi-lane rounds):** `Scripts\Build-Game.ps1 -Package` builds the editor module, regenerates content
  (`bootstrap_unreal.py` and the character/locomotion imports run as `UnrealEditor-Cmd`
  commandlets, one at a time, about 25 min) and runs UAT. `-PackageOnly` skips the content steps when
  this worktree's generated content is already current. UAT is single-instance machine-wide; the
  script builds the game target with `-WaitMutex`, waits for UAT (`-WaitForUATMutex`) behind other worktrees, and cooks without the shared Zen store (`-SkipZenStore`). Each Unreal step counts
  toward the 2-process limit, and the cook starts more. It writes `Build\Logs\bootstrap.log` and
  `Build\Logs\package-<time>.log` in your worktree; the UAT log under `%APPDATA%` is shared and
  unreliable. The bootstrap re-saves tracked `.uasset`s, so check `git status` afterwards. The
  script refuses to package over a running player; close `SurvivalGame`/`JennysHomesteadGame`
  processes from that folder first (for Jenny's builds, see sections 0 and 7).
- **Packaged walk drivers: `Walk` after every `BugItGo`.** `BugItGo` switches on Ghost (flying, no
  collision), so a driver that then holds W flies her level and under rising ground, and the
  terrain-recovery toast repeats. It looks like missing landscape collision; three map-lane runs were
  lost to it (table 0.1). `Scripts\Examples\packaged_walkoff.py` is a working reference driver: it
  launches the package, finds its window by process image, types console commands, and records the
  walk with ffmpeg ddagrab.
- **Keep packaged or standalone test runs off Jenny's saves:** launch with
  `-userdir=E:\CopilotScratch\<session-id>\pkguser -log=<name>.log`, so saves, config and logs go
  under that folder instead of the package's own `Saved\`.
- **UI at real resolutions and DPI (standalone window, not PIE):** launch
  `UnrealEditor.exe "<worktree>\SurvivalGame.uproject" /Game/SurvivalGame/Maps/Estate -game -windowed
  -ResX=3840 -ResY=2160 -log=ui-4k.log` (and 1280x720; for 4K use `-fullscreen` instead of
  `-windowed`, since a 4K window doesn't fit the 175%-scaled desktop; `[GameWin]::Capture` works in
  fullscreen), wait for `MUSIC_TRACK started` in that log
  plus about 20 s, then dot-source `Scripts\GameWindow.ps1`: `Find-GameWindow -ProcessId <pid>`,
  `[GameWin]::Key/Char` (PostMessage input, which works where SetForegroundWindow/SendInput don't)
  and `[GameWin]::Capture` (DPI-aware PrintWindow). It counts toward the 2-process limit; close it
  by PID. This isn't packaging, so lanes may run it.
- **Packaged smoke and route tests (integration session only, like packaging):** `Scripts\Test-Game.ps1` with hashtable splats (table 0.1);
  point it at a non-default package with `-PackageDirectory <dir>` (and `-OutputDirectory`).
  They run a plain `-game` process with `-HomesteadSmokeTest`, which uses the legacy heroine; check
  the MetaHuman heroine yourself (field notes).
- **Blender props into Unreal:** import them in your running editor with `py` (see Props in the
  field notes), not with the headless `Import-Props.ps1`, which drops LODs and collision.
- **C++ conventions that bite:** see the unity-build, C2487 and enum-default rows in 0.1. UI code
  includes Simulation headers as `../Simulation/<Header>.h`. Hold loaded assets in `UPROPERTY()`
  members, never function-local statics (field notes). Font assets: a `FontFace` needs
  `loading_policy` INLINE, and an `FTypefaceEntry` is built with `Emplace_GetRef(name)` then
  `.Font = FFontData(Face)`. Slate: `FSlateDrawElement::MakeCustomVerts` expects sRGB vertex
  colours (`ToFColor(true)`), and `FGeometry::GetLocalSize()` returns `FVector2f` (convert before
  mixing with `FVector2D`).

## 9. Field notes

Grouped by topic, dated, newest first within a topic where it matters. Promote anything durable
into the sections above, and add failures with a clear fix to table 0.1. Playtest bugs go in
OpenSpec changes, not here.

### Editor, MCP and PIE

- 2026-09-28: **PIE stills.** `HighResShot 1920x1080` (via `execute_console_command(w, cmd, pc)`) writes
  to `Saved\Screenshots\WindowsEditor\`.
  `hshot` in `Scripts\McpHelpers.ps1` wraps this.
- 2026-09-26: **Go and look.** Put her next to the target with `get_player_pawn(w, 0).set_actor_location(v,
  False, True)` at z ≈ 200 (she drops to the ground) and `set_actor_rotation`. Then use `walk_to`
  into a boulder to prove collision: `stuck` at about the footprint radius.
- 2026-09-26: `EditorAssetLibrary.save_asset(path, False)` returned False on material instances during PIE.
  After stopping PIE, `save_loaded_asset(obj, False)` saved them.
- 2026-09-26: **Modal dialogs block MCP.** One modal (for example "Overwrite Existing Object" during a
  reimport while PIE runs) blocks every later `run_python` call indefinitely. Find it with
  user32 `EnumWindows` on the editor PID and click its button. Call `SetProcessDPIAware` first,
  because the desktop is scaled. Stop PIE before reimports.
- 2026-09-25: Killing the editor leaves `Saved\Autosaves\PackageRestoreData.json`. The next launch
  opens a modal "Restore Packages" dialog that blocks startup and the MCP server, and synthetic
  clicks on Skip Restore don't dismiss it. After a kill, delete that file (and scratch autosaves)
  before relaunching. Quitting via `run_python` `unreal.SystemLibrary.quit_editor()` (after
  `LevelEditorSubsystem.editor_request_end_play()`) exits cleanly in seconds and leaves restore
  disabled.
- 2026-09-25: Console commands in PIE: `run_python` with
  `unreal.SystemLibrary.execute_console_command(unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world(), '<cmd>')`.
  To A/B lighting live, call `set_actor_tick_enabled(False)` on `HomesteadController` so its
  Refresh stops overwriting the sun. Then set the sun's rotation, intensity and colour, and the
  `MeadowExposure` post-process settings (set `override_<name>` too), and capture. Turn the tick
  back on afterwards. A new game starts at hour 6 (dawn), and one game hour is 2.5 real minutes.
  `con '<cmd>'` in `Scripts\McpHelpers.ps1` does this with the player controller.
- 2026-09-25: Slate `Click`/`PressKey` on game menu widgets only moved hover/focus; the game reads
  input through the PlayerController, not Slate focus. That is why `HomesteadPlayTools` exists.
- 2026-09-25: PIE shows an on-screen editor warning, "Multiple directional lights are competing to
  be the single one used for forward shading...". It comes from the game's lighting setup, not MCP.
- 2026-09-25: A fresh headless Copilot session loaded `unreal` from `.github\mcp.json` and called
  `list_toolsets` and `get_current_level` natively.
- 2026-09-25: First PIE start took about 76 s (shader and animation compilation); `StartPIE`
  returned "Timed out waiting for PIE to start" while PIE was running.
- 2026-09-25: `CaptureViewport` rejects calls that omit `captureTransform`/`annotations`; pass `null`.
  It returns the PNG inside JSON text (`returnValue.image`), which `editor_mcp.py` extracts.
- 2026-09-25: `find_actors` with `{}` fails; it needs explicit keys (check `describe_toolset`).
- 2026-09-25: The view tool sometimes reports a freshly written PNG as missing; view it again.
- 2026-09-27: Slate `Click` does not reach game menus, but a real Win32 click does. With the PIE
  window pinned at 0,0 1936x1119 by `pietop.ps1`, call `SetProcessDPIAware`, then
  `SetCursorPos` in window pixels and `mouse_event` down/up (2/4). Park the cursor afterwards.
  Jenny's own mouse can interfere if she is at the PC.
- 2026-09-25: Python can't spawn actors into the PIE world. Test through the character's own component stack
  instead.
- 2026-09-25: Tools like the hatchet are hidden except during their action, so capture a burst about
  0.5-1.3 s after the action key. For which keys act after a keyboard or gamepad tap, see
  "Stick to one input device per sequence" in section 4.
- 2026-09-25: For a face close-up in PIE, set the `CameraArm` `target_arm_length` (about 90),
  `socket_offset` 0 and `target_offset` (0,0,70) via `run_python`. Restore afterwards (330,
  (0,45,55)).

### C++, builds and Live Coding

- 2026-09-28: **Live Coding can crash PIE.** A patched module crashed in `UsesMetaHumanHeroine`: a CVar
  was null during BeginPlay. To recover, quit the editor, delete `Binaries\Win64\*patch*`,
  then rebuild through `Start-EditorMcp.ps1`. Prefer a full rebuild for changes to world
  generation.
- 2026-09-27: **Live Coding patch files stay locked** for about 60 s after `quit_editor`. Wait for the
  process to exit, then delete `Binaries\Win64\*patch*` before `Build-Game.ps1 -Package`.
- 2026-09-25: Live Coding (`LiveCodingToolset` `CompileLiveCoding`) patches function bodies while PIE runs. No longer used by agents: it switches Live Coding on and blocks every other worktree's build (section 0).
  Standalone `-game` runs load the DLL from disk, so do a real build first.
  Live Coding is now off in agent editors (section 0); rebuild instead.
- 2026-09-26: **Never cache a loaded asset in a function-local `static UStaticMesh*`.** `static M =
  LoadObject(...)` caches nullptr forever if the asset didn't exist (or wasn't saved) when the
  line first ran. Worse, a raw static is not a GC root: once no component uses the asset, GC
  frees it and the next use crashes. The packaged FullLoop crashed in `SetStaticMesh` from
  `BuildPlot` after the sown-seed mound mesh went unused. Hold it in a `UPROPERTY()
  TObjectPtr<>` member loaded on first use (`if (!Member) Member = LoadObject(...)`). Save new
  assets with `EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)` before testing.

### Tests and packaging

- 2026-09-27: When scripting the packaged suites, pass switches as a hashtable splat
  (`$p=@{Packaged=$true; NativeMenu=$true}; .\Scripts\Test-Game.ps1 @p`). An array splat such as
  `@('-NativeMenu')` binds as a positional string and silently runs only the default smoke test.
  Move `Saved\Automation\Packaged\native-wardrobe-fixture*` into `History\` before a NativeMenu run. An empty switch string errors with "Generated resume requires...".
- 2026-09-26: **Check the real heroine yourself.** `Test-Game.ps1 -Packaged` shows the legacy heroine
  (`-HomesteadSmokeTest`). To see the MetaHuman, launch
  `Build\Windows\...\JennysHomesteadGame.exe -Res=0x0wf` and bring it to the foreground. Use the
  Alt `keybd_event` trick before `SetForegroundWindow`, or the Copilot app stays on top. Then
  capture a short ddagrab burst (`-t 1.2 out_%02d.png`); a single `-frames:v 1` grab sometimes
  writes nothing.
- 2026-09-25: `Test-Game.ps1` smoke routes run a plain `-game` process (not UE automation) with
  `-HomesteadSmokeTest`, which selects the legacy heroine.
- 2026-09-25: The Hotbar route's "Held mapped Shift again reaches active grounded sprint" step is flaky
  after `NewGame`; it passed on rerun.
- 2026-09-25: Frame-cost breakdown: `-ExecCmds "t.MaxFPS 0, r.GPUCsvStatsEnabled 1, csvprofile start"` on
  `Playtest-Visual.ps1`. The CSV lands in `Saved\Profiling\CSV\`. Before timing, check for other
  heavy processes (a Blender render from another session skewed runs).

### Heroine: MetaHuman, Creator, hair and clothing

- 2026-09-27: **She starts barefoot.** `NewGame` equips only the tunic (`nextWearableId` 2). Woven
  footwraps (8 Fiber) are the craftable footwear; LeatherShoes remain only in older saves.
  Tests needing shoes use the `Shod(sim)` helper in `Tests/HomesteadSimulationTests.cpp`.
- 2026-09-27: **Hair auto-reset.** `UpdateHairMotion` calls `ResetSimulation()` after long frames
  (> 0.1 s), real-time gaps > 0.25 s between ticks (pause and menus stop actor ticks), or a
  head jump > 25 cm or > 40° in one frame. This is a defensive fix for stray flyaways, which
  haven't been reproduced since.
- 2026-09-26: **Baked underwear on the body.** The MetaHuman body textures (`T_Body_{BC,N,SRMF}_VT`) have the
  grey top and briefs painted in. `Scripts\Characters\remove_body_underwear.py` inpaints every
  underwear pixel, covered or not (the tank top's back scoop and straps showed grey otherwise).
  Export the three textures with `AssetExportTask` + `TextureExporterTGA`, run the script, and
  reimport the `_Clean.tga` files over the originals with `replace_existing_settings=False`,
  then re-check `srgb`, compression and VT streaming.
- 2026-09-25: **Body shape via Python works.** `try_add_object_to_edit`, then `get_body_constraints`; build a
  *new list* (changing the Array's structs in place doesn't stick), set `is_active` and
  `target_measurement`, then `set_body_constraints` and `commit_body_state`. The model shifts
  other measurements (Hip down pushed Waist up), so pin those too.
- 2026-09-25: A rigged character must have its face rig removed first (`remove_face_rig`), then be re-rigged
  (`request_auto_rigging`, ~25 s), reassembled, and re-retargeted.
- 2026-09-25: `metahuman_look.set_body` wraps this. Always `remove_object_to_edit` afterwards, or Creator
  later fails to open the asset ("already added for editing... may be corrupted").
- 2026-09-25: `build_meta_human` can raise RuntimeError (Control Rig "Cannot break link") even when the log
  says the assembly succeeded, **and it doesn't save**. Call
  `EditorAssetLibrary.save_directory(path, False, True)` on the Assembled and Common folders.
- 2026-09-25: Texture resolution: set all eight `skin_settings.desired_texture_sources_resolutions` fields to
  `RES4K` before `request_texture_sources`. `has_high_resolution_textures` is a property.
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
- 2026-09-25: Creator scripting recipes (via `run_python`). Duplicate a preset from
  `/MetaHumanCharacter/Optional/Presets/<Name>` to start a character. Swap hair with
  `c.internal_collection.try_add_item_from_wardrobe_item('Hair', wi)` plus
  `default_instance.set_single_slot_selection('Hair', key)`, using wardrobe items under
  `/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/`. **Scripted edits don't appear in an open
  Creator window, even after Refresh Preview.** Close the editor (`close_all_editors_for_asset`),
  release edit mode, save, then reopen. For review shots, set `viewport_settings.camera_frame`
  (FACE/BODY) and `show_viewport_overlays = False` before opening, then use Slate `Screenshot` on
  the `MHC_<name>` window. Body shaping through Python works (see the body-shape note).
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
- 2026-09-27: `homestead_agent.metahuman_hair` copies stock MetaHuman grooms into
  `/Game/Characters/Heroine_MH/Common/Optional/Grooms/GroomAssets/Hair/<Style>`, repoints their
  materials at the heroine's hair instances and builds a `<Style>_Binding` against her face. Its
  `STYLES` order must match `HomesteadLook::MetaHairGroom`.
- 2026-09-27: `HomesteadWear <key|name>` (console, in PIE) grants the materials, crafts one garment
  and puts it on, for example `HomesteadWear fur-coat` or `HomesteadWear woven-sandals`. The
  MetaHuman garments live in `/Game/Characters/Heroine_MH/Assembled/Heroine/Garments`. To
  re-import them, run `Scripts/Characters/import_heroine_garments.py` in the editor (set
  `GARMENT_NAMES` first to import a subset). Deer remains show up as `DeerRemains` in
  `get_play_state`; harvesting gives 3 Fur.

### Animation authoring and held tools

- 2026-09-28: **Finger spread on the rig.** Pitch on `{finger}_01_{l,r}_ctrl` fans the fingers. On the
  left hand, positive index and negative pinky draw them together. `active_idle.py` keys the
  curl and spread for both hands (`CURL_*`, `SPREAD_*`).
- 2026-09-28: **Resting carries are turned by `Flip` in `UpdateHeldTools`.** The hatchet and knife are
  turned about the haft (Z) so the edge hangs down. The hoe is turned about the palm normal
  (X) so its blade sits low in front. The flip eases out with `HeldToolTilt`, so authored
  actions keep their working grip.
- 2026-09-27: **Held-prop placement must run inside `UpdateHeldTools`.** `UpdateHeldTools` runs in Tick
  (TG_PostUpdateWork) and rewrites every held prop's relative transform. A placement done from
  `OnBoneTransformsFinalized` was overwritten each frame, so the felling hatchet stayed in her
  right fist only. `UpdateFellingHatchet()` is now called at the end of `UpdateHeldTools`. If
  you change the felling clip, re-measure `FellBit*`/`FellCut*` in `HomesteadCharacter.h` from
  `axe_fell.py`'s `bit_at_strike` report.
- 2026-09-27: **Keep the two-handed backswing in front of her.** Check the left arm against the chest in
  front and side lab captures. An early `back` key sent the left forearm through her chest.
- 2026-09-27: **Finger curl on the rig.** On `{index,middle,ring,pinky}_0N_l_ctrl`, yaw curls the finger
  and negative yaw curls toward the palm. Roll does nothing. `kneel_gather.key_knee_fingers`
  and the `KNEE_*` constants (knee joint at about (16, 36, 51) component space when kneeling)
  rest the left palm on top of the kneecap in all four kneel clips. Key the curl on at the
  kneel and off at the rise.
- 2026-09-26: **Hand palm direction.** For these MetaHuman hands, `across x along` (index-minus-pinky
  crossed with wrist-to-knuckle) points out of the *back* of the right hand, and the relaxed
  fingers curl the opposite way. `FHandGrip` and `HandGripTransform` curl and seat props toward
  `along x across`. Getting it backwards bends the fingers backwards (the "cursed" grip).
- 2026-09-26: **Resting idle.** `AN_HeroineMH_ActiveIdle` (`active_idle.py`) replaces LivingIdle02 when
  present: shoulder-width stance, knees tracking straight over the feet (Jenny found them too
  wide at first), arms hanging close, weight on the right leg, hands clear of the pouch. Held
  tools tip forward in the fist (`HeldToolTilt`) instead of bending the wrist.
- 2026-09-26: **Felling is a right-shoulder chop.** In `axe_fell.py` the left hand holds the knob and the
  right hand slides; `UpdateFellingHatchet` lays the hatchet through both fists. The bit lands
  to her right (`FellBitLeft` is negative).
- 2026-09-25: **Migrating from another project headless:** run `UnrealEditor-Cmd <other>.uproject
  -run=pythonscript` with `AssetTools.migrate_packages(pkgs, <our Content dir>, MigrationOptions(prompt=False))`.
  It follows every dependency. The GASP clips dragged in 272 foley sounds and about 40 extra
  clips via notifies, which had to be pruned.
- 2026-09-25: **Retargeting UEFN → `IK_MH_IKRig`:** keep the Root Motion op **off**. Even with target root
  `root` and `COPY_FROM_SOURCE_ROOT`, it wrote a root that snaked and yawed up to 16° per step.
  Locking that root in game made the heroine's head and shoulders wobble. `gasp_locomotion`
  retargets with the op off, then `straighten_root` puts the travel on a straight root before
  locking the clips in place (`set_root_motion_enabled`). Check a new clip with
  `get_bone_pose_for_time(anim, 'root', t, False)`: the root should have no yaw and no sideways
  drift.
- 2026-09-25: Retargeting to `IK_MH_IKRig`: the "Run IK Rig" op blew poses out to about -9000 cm, and "Root
  Motion" sank the pelvis. Disable both by index (`set_retarget_op_enabled(i, False)`). After
  fuzzy auto-mapping, clear the bad maps (Root from a foot, metacarpals from fingers).
- 2026-09-25: `IKRetargetBatchOperation.run_batch_retarget` returns nothing while PIE runs; stop
  PIE first.
- 2026-09-25: After `AnimationDataController.set_bone_track_keys`,
  `AnimationLibrary.get_bone_pose_for_frame` reports the pelvis about 10.7 cm lower than the
  written keys (a skeleton-vs-retarget-source offset). The game doesn't apply that offset. Write,
  read back, and compensate (see `gasp_locomotion.straighten_root`).
- 2026-09-27: Sequencer animation bakes (`homestead_agent.axe_fell`, `hoe_till`) fail and hang while
  PIE runs ("Editor is currently in a play mode"). Stop PIE before baking.
- 2026-09-27: Idle tool angles can be tuned live with `homestead.CarryHatchet`, `CarryHoe`,
  `CarryMachete` and `CarryKnife` (degrees; -1 keeps the default). If the hoe carry changes, re-read
  its transform relative to `hand_r` and update `HELD` in `hoe_till.py` before rebaking.

### Props, materials and imports

- 2026-09-25: Original props are authored in Blender by the `blender-assets` skill
  (`docs\blender-assets.md`, `Assets\Props\*`). Its `Import-Props.ps1` opens the project in its own
  editor, so quit an MCP editor session first; never run both at once. The heroine's MetaHuman
  pipeline doesn't use Blender.
- 2026-09-26: **Masks samplers need non-sRGB textures.** Engine `WhiteSquareTexture` and `Black` are sRGB. On
  a Masks sampler the whole material fails to compile and silently renders as the default grid,
  so search the log for `Failed to compile`. `import_props.py` uses its own
  `T_PropDefault{White,Black}` textures and `/Engine/EngineMaterials/DefaultNormal`.
- 2026-09-26: **Import props in the running editor.** Under `-run=pythonscript` the
  `StaticMeshEditorSubsystem` is missing, so collision and LOD import fail. Instead, `run_python`
  `sys.path.insert(0, r'<repo>\Scripts\Blender'); import import_props; import_props.main([...])`
  (or `import_prop(name, M_Field)` per prop), with a long `editor_mcp.py --timeout`. `_LODn` FBXs
  become LODs of their base mesh. Reports with a `wind` block get `M_PropFoliage`: masked,
  two-sided foliage, packed R roughness / G translucency / B AO, vertex-colour wind WPO and a
  camera-safe dither.
- 2026-09-26: **Reloading `import_props` after editing it:** a plain `import` returns the cached module.
  Load it fresh with `importlib.util.spec_from_file_location('import_props',
  r'<repo>\Scripts\Blender\import_props.py')` + `module_from_spec` + `exec_module`, then
  `m.main(['TilledBed', 'Seeds'])`. Extra textures (for example a `_wet` variant) go through an
  `AssetImportTask` with `replace_existing=True`.
- 2026-09-26: **An invisible ground prop is usually back-face culled.** Check the FBX's average face
  normal z in Blender; an open sheet from `kit.recalc_normals` can face down (see
  docs/blender-assets.md).

### World, terrain, water and lighting

- 2026-09-28: **Measure tree bases against the terrain meshes, not engine traces.** `line_trace_multi`
  mostly hits tree capsules or nothing. HomesteadWorld's 25 terrain
  `ProceduralMeshComponent`s answer `line_trace_component(start, end, True, False, False)`
  (it returns a tuple, or None on a miss). Compare against the tree mesh's lowest LOD-rim
  vertices (active trees render at MinLOD 1, outer trees at LOD 2).
  `ResolveGeneratedTreeVisual` sinks each tree by `4 + RimLift * Scale` below the lowest ground
  sampled at 0.5× and 1× footprint in 8 directions. RimLift is 30 for the conifer, 19 for the
  accent and 3 for the broadleaf.
- 2026-09-26: **Count decorations by tag.** In the PIE world, loop over actors, then
  `get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent)`, keep
  `component_has_tag('WoodlandGranite')` (or `'WoodlandUnderbrush'`), and read
  `get_instance_transform(i, True)` per mesh. The log line `Generated cover refresh: ...
  underbrush= granite= big_granite=` gives the totals.
- 2026-09-25: Trees are components of the `HomesteadWorld` actor. When searching for terrain with
  traces, require the `ProceduralMeshComponent` hit and a single hit per column; otherwise a
  teleport lands the heroine on a tree canopy.
- 2026-09-27: **Single Layer Water, not Translucent.** A translucent surface read as a flat dark sheet under
  the canopy. SLW renders in the opaque pass, so it takes the woodland's shadows and Lumen
  reflections, and the bed shows through tinted by the real water depth.
- 2026-09-27: **SLW coefficients act per centimetre.** Absorption around 4/1.6/1.2 made the 25-38 cm creek
  opaque. The shipped defaults are Absorption (0.062, 0.025, 0.02) and Scattering
  (0.0003, 0.0006, 0.0007). Absorb red fastest for a clear green-teal.
- 2026-09-27: **Transient compile errors.** While the graph is being authored, the log fills with
  "No inputs to Single Layer Water Material" / `Failed to compile` lines from the intermediate
  compiles. Judge only the final state: `recompile_material`, then
  `MaterialEditingLibrary.get_statistics(m).num_pixel_shader_instructions` > 0 (about 1048) and
  `get_inputs_for_material_expression` on the SLW output node. Stop PIE before re-authoring,
  because `import_asset` fails during PIE.
- 2026-09-27: **Tune live with a MID.** In PIE, find the `ProceduralMeshComponent`s tagged `CreekWater`
  (5 around the start), make one `create_dynamic_material_instance(0, M_CreekWater)`, set it on
  all of them, and drive `set_scalar/vector_parameter_value`. Bake the winners into the bootstrap
  defaults. Confirm the flow with a 3 s ddagrab clip; compare frames 0 and 25.
- 2026-09-25: The sun uses ray-traced shadows (`homestead.RayTracedSun`, default 1) and moves every
  refresh. `homestead.RayTracedSun 0` restores VSM with 0.5° steps. Don't reintroduce continuous
  rotation on the VSM path. See `docs/research/rendering-baseline/README.md`. The `-Presentation`
  smoke route fails at its first case for a pre-existing reason: `ApplyAppearance` rejects
  "Owned wardrobe appearance requires PrepareEquipment".
- 2026-09-25: Rotating a directional light invalidates every cached Virtual Shadow Map page. The
  world refresh (every 0.25 s) used to rotate the sun continuously, causing paired 30-39 ms stalls
  at 4K. On the VSM path `UpdateLighting` steps rotation by 0.5°; the default ray-traced sun
  moves continuously.
