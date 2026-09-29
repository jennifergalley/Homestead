# Working with Jenny on Homestead

## Model, reasoning and implementer slots

Jenny's standing team preference (2026-09-29). These are **required settings for future session
launches**; documenting them does not change a live session's model or reasoning level.

| Role | Model (exact ID) | Reasoning | Context |
| --- | --- | --- | --- |
| Documentation Agent | GPT-5.6 Terra (`gpt-5.6-terra`) | **high** | **long** |
| Architecture Agent | GPT-6 Sol (`gpt-6-sol`) | high | long |
| Orchestrator Agent | GPT-6 Sol (`gpt-6-sol`) | **medium** | **long** |
| Implementer (Blender, Unreal or code work) | Claude Opus 5.5 | high | long |

**At most three concurrent implementers** do Blender, Unreal or code work. This is a cap across
implementer agents, separate from the 2-Unreal-process machine cap. Coordinating, documentation,
architecture and review-only work do not use an implementer slot unless they start implementation.
The orchestrator grants the next implementer slot before a waiting lane resumes. A lane that is idle
or waiting schedules a wake-up and ends its turn; it doesn't hold a slot by sleeping or polling.

## Loops

Jenny works in two modes. When she hasn't said which, treat short requests as the Interactive Loop.

- **Interactive Loop** (default when she's around): work in short, incremental loops that each
  deliver one small end-to-end improvement she can playtest (for example, one new animation). When an
  improvement is done and verified, commit it with a descriptive message and push it to `main` (a lane
  sends `[ready]` instead; see "Delivering lane work" in `docs/handoff/README.md`), then report what
  to try. Don't batch several improvements into one delivery. Packaging now follows the playtest
  schedule below rather than every improvement.
- **Playtest builds on a schedule** (Jenny, standing preference). A packaged build must be on the
  "Homestead Estate" desktop shortcut by **7:30 AM every day** (weekends too) and by **4:00 PM on
  weekdays**, after her work. On weekends, also cut one as soon as features she'd notice land. The
  orchestrator triggers the integration session at about 5:30 AM and 2:00 PM. It packages `main`,
  runs the packaged suites, retargets the shortcut (keeping the Homestead icon) and reports
  `[playtest] ready @ <sha>` with what's new and what to try. Lanes close their editors while a
  build is being cut. If packaging or the suites fail, the last good build stays on the shortcut.
  **`main` must stay playable:** push only verified work.
- **When Jenny pauses development to play,** every session stops launching editors and builds until
  she says to resume. Finish or park your current step, close your editor, and wait by ending your
  turn (see "Waiting means ending your turn" below).
- **The orchestrator only coordinates** (Jenny's standing preference). It never builds, merges,
  packages or verifies: while it's busy with hands-on work its turn stays open, and queued messages from
  lanes can't reach it (a manager too busy being an IC to manage). It forwards `[ready]`s, relays
  results, assigns follow-ups, and ends its turns promptly. A dedicated integration session does all
  hands-on integration work.
- **Autonomous / Autopilot Loop**: when she puts the session on autopilot, work continuously for
  hours on long-running improvements or full feature build-outs, selecting the next thing to
  iterate on as each one completes.

## Judgment calls

- When you're unsure whether to make a change, make it if it would enhance realism and
  verisimilitude (for example, carried sticks lying across her body instead of jutting forward,
  keeping limbs out of her torso, or a more natural pose). Say what you changed when you report.

## Builds

- **Only the integration session runs UAT** during multi-session rounds: `Scripts\Build-Game.ps1 -Package`
  or `-PackageOnly`, `RunUAT BuildCookRun`, and packaged-game tests. Several worktrees packaging at
  once fought over the machine-wide build mutex and the shared Zen server, and each package costs
  20-40 minutes of CPU, disk and VRAM. The separate `mvp-survival` line packages its own
  deliverables to `E:\Repos\HomesteadMVP\Windows`, after telling the orchestrator.
- If the packaged game is running from `Build\Windows` when you (the integration session) need to
  repackage, close it and build in place. She is only experimenting in it for now and prefers
  getting the newest build.
- The editor skill (`.github/skills/unreal-editor-mcp/SKILL.md`) covers driving the live editor,
  playtesting, and the character lab (`-HomesteadCharacterLab` / `homestead.CharacterLab 1`).
- Record bugs and features as OpenSpec changes under `openspec/changes/`.

## Start here, and report to the docs agent (every session)

Several sessions work on this repo in parallel. A blocker solved once must not be re-solved by the
next five sessions.

- **New to the work, or starting a round?** Read `docs/handoff/README.md` (roles, direction,
  protocol) and the current `docs/handoff/round-<n>.md` (who's who, lane status, open blockers)
  first.
- **Report what you learn to the docs agent** listed on the round page, via `send_session_message`.
  That covers failures and fixes, missing or wrong docs, recipes, and interfaces other lanes need.
  The docs agent verifies each finding and records it once in the canonical place. The report
  template is in `docs/handoff/README.md`. Send reports as you go; don't batch them until your
  feature lands.
- **No docs agent listed, or it's archived?** Ask the orchestrator to spawn one. Until one exists,
  fix the canonical doc yourself in the same change, and commit it to `main` promptly. The
  orchestrator reconciles merge conflicts.
- **Blockers:** anything that blocks you for more than about 15 minutes, or affects other worktrees
  (shared ports, GPU/VRAM, Live Coding, locks, a broken `main`), goes to the docs agent and the
  orchestrator immediately, with the exact error and what you tried.
- **Urgent messages reach busy sessions through the mailbox.** `send_session_message` waits until
  the target's turn ends, which can be hours on autopilot. For blockers, rule changes and
  stop/rebase requests, also use `mailbox_send` (to a worktree folder name, a branch or `all`). The
  target sees it after its next tool call. If you're a long-running session and `mailbox_send`
  isn't among your tools, run `extensions_reload` once. When a mailbox message arrives, act on it
  before continuing. Details are in `docs/handoff/README.md`.
- **Before debugging a tool or build failure,** check "Known failures → fixes" (section 0.1 of
  `.github/skills/unreal-editor-mcp/SKILL.md`).
- **Lanes still maintain their own OpenSpec change** and feature-only docs. Shared docs (skills,
  `docs/setup.md`, `docs/version-control.md`, `docs/handoff/`) go through the docs agent.

## Parallel sessions on one machine (hard rules)

The full list with fixes is in the editor skill, sections 0 and 0.1. In short:

- **Session names** (Jenny, standing preference). Every session keeps its app name as
  "<one or two words> Agent", describing its current work ("Clearing Agent", "Integration Agent",
  "Lamp Agent"). Rename yourself with `rename_session` (`force: true`) when you start and whenever your
  main task changes. The orchestrator names new sessions this way when it creates them. If the tool
  refuses because Jenny renamed the session herself, keep her name.
- **Waiting means ending your turn** (Jenny, all sessions). Never sleep, poll or loop in a shell while waiting (for an editor slot, the UBT queue, a `[ready]`
  or a perf window): a blocking wait keeps your turn open, so queued `send_session_message`s never
  arrive. Schedule a wake-up with `save_session_automation` (`interval: "once"` with a `run_at` a few
  minutes ahead, or `"minutes"` with `every_minutes`), say in its prompt what to check, end your turn,
  and clear the automation when it's no longer needed. Waiting on a build or command you started
  yourself is fine through the tool's own completion notification (async shell / `initial_wait`).
- **At most 2 Unreal processes on the machine** in total: editors, packaged games and commandlets
  all count, and one of the two is reserved for the integration session, so the other lanes share one
  slot. `Start-EditorMcp.ps1` enforces both; check other launches with
  `Get-Process UnrealEditor*,SurvivalGame*,JennysHomestead*`. Each editor commits 15-17 GB, and three
  filled RAM and grew the pagefile on C: to 81.5 GB. **Close your editor as soon as a verification
  pass is done.**
- **Perf measurements get the machine to themselves:** one Unreal process, no builds. Claim it with
  `Scripts\Start-PerfWindow.ps1` and release it with `Stop-PerfWindow.ps1`.
- **Give your editor its own MCP port** (`Start-EditorMcp.ps1 -Port 87xx`) and set
  `$env:UNREAL_MCP_URL` to match. Port 8765 and the native `unreal` MCP tools may belong to another
  worktree's editor.
- Close your editor before `git pull`/`rebase`, and before building your editor module
  (`Scripts\Stop-MyEditor.ps1` closes only this worktree's editor).
- Never retarget or overwrite `Desktop\Homestead.lnk` or anything under `E:\Repos\HomesteadMVP\`.
  Never merge `mvp-survival` with `main`.
- No worktrees, builds, renders, videos or big binaries on C:. See the user-level disk rules.

## Code practices (owned by the Architecture Agent)

The architecture map is `docs/architecture.md`; the long form with reasons is the
`homestead-code-conventions` skill. Recipes: `homestead-add-item-or-interactable`,
`homestead-add-hud-element`, `homestead-animation-layer`.

- **Rules live in the simulation.** Anything that decides what she gets, pays, can do or saves goes
  in `Source/SurvivalGame/Simulation` (plain C++17, no Unreal types), returns `Homestead::Result`,
  and gets a native test (`Scripts\Test-Native.ps1 -Configuration Release`). Actors present and
  animate.
- **Don't grow the big four.** New features go in new files: `HomesteadController<Feature>.cpp`
  for controller members, a new actor or widget for new things on screen or in the world. Add new
  simulation `.cpp` files to `CMakeLists.txt`.
- **Enums that name data are append-only** (`Item`, `ResourceKind`, `Recipe`, `Piece`...): add
  before `Count`, never reorder or delete, and keep the parallel tables (`ItemCatalogue`,
  `ResourceName`, `OgTable`) in step. Estate placement ids are claimed on the round page and never
  reused or renumbered.
- **Saves:** never change `SimulationSaveVersion` or `bakeVersion` yourself. Tell the orchestrator
  before your `[ready]` if you changed what `Serialize` writes or moved or removed placements.
  Appending an `Item` is save-safe (version 13 stocks carry their width); write any new list or
  per-enum array with its count first.
- **Unity-build safe:** file-local names are unique or live in a named namespace
  (`namespace <Widget>Style`); no `using namespace` at file scope. Warnings are errors.
- **No runtime reads during static init:** globals, file-scope statics and CVar defaults never call
  `FCommandLine`, `FParse`, `GConfig`, `FPaths`, `GEngine` or `LoadObject`; the packaged game crashes
  at launch (777006). Read them inside a function on use.
- **Assets** are held in `UPROPERTY() TObjectPtr<>` members, never function-local statics.
- **Per-frame cost:** nothing per tick or per paint that scales with the number of placements or
  components; gate work on `Simulation::GetRevision()` or on an actual change.
- **UI:** Slate widgets read `TWeakObjectPtr<AHomesteadController>` in cheap `*_Lambda`s; colours
  come from `UI/HomesteadPalette.h`.
- **Name tuning numbers** (`constexpr` with units and what they're tuned to); timings that match an
  authored clip cite the `.py` that authored it.
- **New code logs to a named category** (`LogHomestead<Area>`), not `LogTemp`.
- **Python:** module docstring with the usage line, constants at the top, generated files marked
  "do not edit by hand", big intermediates on `E:`.
