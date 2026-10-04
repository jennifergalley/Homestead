# Working with Jenny on Homestead

## Agent Team Makeup

Jenny's standing team (updated 2026-10-04). The orchestrator keeps this section current whenever a
role, model or standing session changes. Model settings are **required for future session launches**.

### Standing roles

| Role | What it does | Model |
| --- | --- | --- |
| **Orchestrator Agent** | Coordinates only: plans, spawns, assigns slots, relays; never builds or merges. Reports to Jenny. | Claude Opus 5.5 / high / long |
| **Integration Agent** | Only session that merges, packages and promotes builds; installs each release into the main checkout (`E:\Repos\SurvivalGame\Build\Windows`), the only place Jenny's shortcut may point. Also the current docs/accounting liaison. | GPT-5.6 Terra or GPT-6 Luna / medium |
| **Docs Agent** | Records verified findings in canonical docs (skills, `docs/setup.md`, `docs/handoff/`). Spawned when needed; Integration covers it otherwise. | GPT-6 Luna or GPT-5.6 Terra / low–medium |
| **Balance Agent** | Reviews game balance (energy, coins, yields, timers, pacing) and cohesiveness (design, flavor, icons, menu/HUD look). Consulted at planning and before `[ready]`; doesn't edit game code. | Claude Opus 5.5 / high |
| **Cleanup Agent** | One bounded disk sweep per day after a delivery. | existing session |

Feature lanes (Gameplay/UI, Environment, Art and so on) are spawned fresh per scheduled build and
archived when their work ships. Never archive a session whose worktree holds something still needed.

### Models

| Role | Model (exact ID) | Reasoning | Context |
| --- | --- | --- | --- |
| Orchestrator Agent | Claude Opus 5.5 (`claude-opus-5.5`) | high | long (Jenny requested 1.1M; actual runtime context is recorded separately) |
| Blender / Unreal asset making and asset integration | Claude Opus 5.5 (`claude-opus-5.5`) | high | default; long only when necessary |
| Balance Agent | Claude Opus 5.5 (`claude-opus-5.5`) | high | default |
| Gameplay/UI/environment code, general Unreal work, visuals or performance implementation, and risky review | GPT-6.1 Sol (`gpt-6.1-sol`) | high | default; long only if needed |
| Documentation, integration / building, scripted tests and accounting | GPT-6 Luna (`gpt-6-luna`), GPT-5.6 Terra (`gpt-5.6-terra`), or GPT-6.1 Sol (`gpt-6.1-sol`) | low / medium as appropriate | default |

Claude Opus 5.5 is allowed only for the Orchestrator, Blender/Unreal asset making and asset
integration, and Balance. The Balance Agent reviews energy, coins, yields, timers, pacing, design,
flavor, and UI against cozy casual fun without grinding, then beauty (including flowers everywhere),
then real-world verisimilitude. It does not edit game code and holds no implementer slot.

## Balance consultation and delivery

Lanes consult Balance while planning every new feature for the proposal and numbers, then again
before `[ready]` for final numbers, player-facing copy, and a screenshot; the ready evidence records
Balance's OK. Integration admits gameplay that changes balance or player-visible flavor only when
the delivery receipt records that OK.

Bias toward fresh task-scoped sessions, retaining an existing session for tightly related follow-ups
when its working context remains useful. Every session, including the orchestrator, maintains a compact
repo handoff and records task/build attribution, actual model/configuration and AI credit usage before
replacement or eligible archival. Model changes within a session are separate accounting segments.
See `docs\handoff\agent-lifecycle.md`; preserve the existing old-agent and live-release archival protections.

**At most three concurrent hands-on game-development implementers** do gameplay code, Blender,
Unreal, or game-asset work. This is a cap across active game-development work, not a role-label
exemption, and is separate from the 2-Unreal-process machine cap. The Integration Agent counts while
merging, compiling, PIE testing or packaging, but not while only coordinating; Architecture counts
while editing or building game code. Planner, backlog, and build-cost canvas-extension work is outside
this cap and needs no slot. Time-critical integration gets a slot by pausing a lane. The orchestrator
grants the next slot before a waiting lane resumes. An idle or waiting session schedules a wake-up and
ends its turn; it doesn't hold a slot by sleeping or polling.

## Loops

Jenny works in two modes. When she hasn't said which, treat short requests as the Interactive Loop.

- **Interactive Loop** (default when she's around): work in short, incremental loops that each
  deliver one small end-to-end improvement she can playtest (for example, one new animation). When an
  improvement is done and verified, commit it with a descriptive message and push it to `main` (a lane
  sends `[ready]` instead; see "Delivering lane work" in `docs/handoff/README.md`), then report what
  to try. Don't batch several improvements into one delivery. Packaging now follows the playtest
  schedule below rather than every improvement.
- **Playtest builds on a schedule** (Jenny, 2026-09-30). Three Estate builds are cut every day:
  **morning** is on the "Homestead Estate" shortcut by **7:30 AM** (start about 6:30 AM),
  **afternoon** by **4:00 PM** (start about 3:00 PM), and **evening** by **9:00 PM** (start about
  8:00 PM). Only work already UE-verified and code-reviewed is admitted; everything else waits for
  the next build. Integration merges, runs UBT/suites, makes the Shipping acceptance check and
  retargets the shortcut while preserving its icon. Lanes close editors during the build. If
  verification fails, the last good build stays on the shortcut. **`main` must stay playable:**
  push only verified work. Lanes implement only what Jenny has prioritized for their specific
  build, then go idle; they do not autonomously take unflagged `priority.json` work. If little or
  nothing is prioritized, the orchestrator asks Jenny to schedule it.
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
  20-40 minutes of CPU, disk and VRAM.
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
- **Cross-session messages are immediate** (Jenny, 2026-09-29): every `send_session_message` sets
  `delivery_mode: "immediate"`; never use default/enqueue for routine messages. Keep messages short,
  self-contained and actionable. For a blocker or rule change that must reach a busy session mid-turn,
  also use `mailbox_send` to its worktree. Older queued messages may still arrive late: honor the newest
  timestamp or explicit decision, and ignore stale superseded instructions.
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
  target sees it after its next tool call. When a mailbox message arrives, act on it
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
- **Estate-only delivery (Jenny, 2026-09-30):** `Homestead Estate.lnk` is the only active game
  shortcut and is retargeted by Integration only after its save-safety and package checks. The
  survival MVP was retired at tag `archive/mvp-survival-20260930` (`93612cdf`); its old package,
  saves and desktop shortcut backup were deleted with Jenny's approval, leaving the Git tag as
  the only MVP archive reference. The
  `jennifergalley-mvp-woodland-biome` branch is **not** the retired line: it is Water's active
  Estate Seasons handoff (`b19a0ad0`).
- **Feedback-complete fast path (Jenny, 2026-10-01):** when all current playtest feedback is
  addressed, Integration ships the verified build immediately instead of waiting for the next
  scheduled slot. Then every lane clears wake-up automations and ends its turn until Jenny provides
  new feedback or the orchestrator assigns new work.
- **Playtest-driven OpenSpec (Jenny, 2026-10-01):** create or retain a change only for Jenny's
  explicit feedback or an estate goal she selects. Keep its proposal, design and tasks short, with
  one or two player-checkable tasks; do not add a large speculative acceptance matrix. Close or
  archive it when shipped. `docs\handoff\backlog.md` is the player-driven queue and
  `docs\handoff\builds.md` is the current changelist and deferred-later ledger.
- No worktrees, builds, renders, videos or big binaries on C:. See the user-level disk rules.
- **Project storage hygiene (every lane):** delete your own scratch, render, recording and test-output
  artifacts when their task ends. Keep large transient artifacts under
  `E:\CopilotScratch\<session-id>`, never session-state `files`; retain only the active shortcut
  Shipping release and one rollback, deleting Development releases after the Shipping cut. Remove a
  secondary per-task worktree as soon as its slice lands or is parked: push the branch if needed,
  then `git worktree remove <path>` and `git worktree prune`; reuse one secondary worktree per lane,
  and never remove another session's active worktree. Delete stale `Saved\Automation` sandboxes
  after their tests. When uncertain, delete unnecessary project-owned artifacts, but never Jenny's
  current save game. The Disk Cleanup Agent runs the daily broader project sweep at 10:00 AM.

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
