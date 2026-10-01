# Start here: how the Homestead agent team works

This folder is the entry point for any agent joining Homestead work, especially at the start of a
new round. Read this page, then the current round's page, then the documents they link. The docs
agent keeps both current.

## Current direction (five lines)

- Homestead is a cozy life sim on an early-Victorian Cornish coast. You restore a derelict family
  estate: clear, farm, fish and mine, sell in town, rebuild the manor.
- There is one fixed hand-authored map (`/Game/SurvivalGame/Maps/Estate`, a 4 km World Partition level
  from real LIDAR), with no procedural world, cold, death or predators.
- The authoritative design and round order: `openspec\changes\pivot-to-cozy-estate-life-sim\design.md`.
- The working policy (playable increments, reuse first, OpenSpec before each round): `docs\game-plan.md`.
- The current round: [round-2.md](round-2.md) (the farming year and period crafting). Round 1, "Walk
  your estate", is recorded in [round-1.md](round-1.md).

## Product copy and UI principle

Jenny's standing direction (2026-09-30): assume the player is familiar with farming sims and avoid
explaining mechanics on every screen. Do not toast obvious outcomes. Focus cards contain the name
and keyed verbs only; detail panes/tooltips contain stats, requirements and price, not rules
explanations; Settings contain a label and value; refusal reasons stay about 4–6 words. Review all
new player-facing copy against this rule before it ships. The approved parchment/EB Garamond theme
does not change this content rule. The forthcoming `realistic-animation` skill and Animation
Inspector must carry the same concise review standard once they land.

**Control rule (Jenny, 2026-09-30):** E/the interact button only interacts—harvest, plant, pick
up, open, talk, eat or sleep. Tools act only through click/the gamepad tool button. Never cross
these paths: E on an unripe crop must not water it. Hold-to-repeat belongs only to tool input.
Menu's control audit and Props' repeat bindings verify this before delivery.

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

## Roles

| Role | What it does | How to find it |
| --- | --- | --- |
| **Orchestrator** | **Coordinates only** (Jenny's standing preference): plans the round, spawns lane, docs and integration sessions, owns shared interfaces and decisions (such as the save-version bump), forwards lanes' `[ready]`s to the integration session, relays results to Jenny, assigns follow-ups, and reconciles doc conflicts. It **never builds, merges, packages or verifies**: while its turn is busy with hands-on work, queued messages from lanes can't reach it. It ends its turns promptly | The round page's registry; `get_sessions_status` ("Orchestrator Agent") |
| **Integration session** | Does all hands-on integration: merges the lane work the orchestrator forwards, resolves conflicts, builds, runs native and packaged tests, PIE and perf checks, and **is the only session that packages** (the only one running UAT). Reports `[integrated] <what> @ <sha>` to the orchestrator | The round page's registry ("Integration Agent") |
| **Docs agent** | Standing session for the whole round. It receives findings and blockers from every session and records each once in the canonical doc. It keeps this folder, the skills and the setup docs current, and relays cross-lane blockers to the orchestrator | The round page's registry ("Documentation Agent") |
| **Architecture agent** (code steward) | Long-lived. Owns how the code is written: `docs\architecture.md`, the "Code practices" section of `.github\copilot-instructions.md`, and the code-convention skills under `.github\skills`. Makes small, safe refactors in files no lane is editing, proposes larger ones as OpenSpec changes for between rounds, and reviews each integrated batch. The docs agent owns process docs (this folder, the editor/Blender skills' shared-machine and failure sections, setup); the two keep each other's docs consistent | The round page's registry ("Architecture agent") |
| **Lanes** | One worktree and one OpenSpec change each. They own the files named in their design's "Lanes and ownership" | The round page's registry |

**Session names:** every session keeps its app name as "<one or two words> Agent", describing its
current work ("Clearing Agent", "Integration Agent", "Docs Agent"), and renames itself with
`rename_session` (`force: true`) when it starts and whenever its main task changes. The orchestrator
names sessions this way when it creates them. The round page maps names to session IDs and work.

If the round page lists no docs agent, or the one listed is archived, ask the orchestrator to spawn
one (`send_session_message`). Until one exists, record findings yourself in the canonical doc.

## Waiting: end your turn, don't loop

Jenny's standing rule for every session. A session that's waiting (for an editor slot, the UBT queue,
another lane's `[ready]`, an `[integrated]`, or a perf window) must never sleep, poll or loop in a shell:
a blocking wait keeps its turn open, so queued `send_session_message`s never arrive. Instead:

1. Schedule a wake-up with `save_session_automation`: `interval: "once"` with a `run_at` a few
   minutes ahead, or `interval: "minutes"` with `every_minutes`. Its prompt says what to check.
2. End the turn, so the session goes idle and messages can reach it.
3. Clear the automation (`clear: true`) when it's no longer needed.

Waiting on a build or command the session itself started is fine through the tool's own completion
notification (async shells / `initial_wait`); a sleep loop isn't. For an editor slot, check
`Get-Process UnrealEditor*` once: one slot is reserved for the integration session, so if another
lane's Unreal process is already running, schedule a wake-up about 5 minutes out and end the turn. The orchestrator uses the same pattern: it checks in every 30 minutes
through its own session automation.

## Jenny's packaged game memory safety

The two-Unreal-process cap is not sufficient by itself when Jenny launches a Shipping/packaged Estate
game: an editor left open alongside it drove Windows available memory to **143 MB** on 2026-09-29.
Treat a user-launched game as a memory-priority event:

1. **Never touch Jenny's PID, save or shortcut.** Do not close, focus, kill or retarget her game.
2. Send the editor owner an urgent `mailbox_send`; that owner closes **only its own** editor (normally
   `Scripts\Stop-MyEditor.ps1`), then ends its turn. The coordinator/orchestrator may alert, but never
   kills an arbitrary process.
3. Do not launch another editor, UBT, UAT, Blender or packaged test until the owner has closed its
   process and memory has recovered. If a command already owned by Integration is safely finishing,
   let it finish, then pause future work.

The reported low-memory symptom is operational contention, not a reason to weaken Windows firewall
alerts, change the user's paging settings, or interfere with her playtest.

## Reaching a busy session fast: the mailbox

`send_session_message` is delivered only when the target's turn ends. That can be hours for an
autopilot lane, even with `delivery_mode: "immediate"`. For anything time-sensitive, use the
**agent mailbox** as well. It's a user-level extension
(`~\.copilot\extensions\agent-mailbox\extension.mjs`, messages stored under
`E:\CopilotScratch\agent-mailbox`). The target sees the message **after its next tool call,
mid-turn**, usually within seconds.

- `mailbox_send(to, message, from_label)`: `to` is a worktree folder name (for example
  `jennifergalley-cautious-pancake`), a branch, a Copilot session id, or `all`.
- `mailbox_who()` lists the sessions that can receive right now.
- A session loads the extension when it starts. A session that was already running needs one
  `extensions_reload` call first.
- A message to an address with no live session is kept as pending and delivered to the next
  session that starts in that worktree or branch.
- The mailbox doesn't replace `send_session_message`; send both for anything that must not be
  lost. The session registry and the handoff docs still use the app session ids.
- It cleans up after itself: every live session deletes messages (delivered, undelivered and
  pending) and dead sessions' folders older than 7 days, at startup and every 6 hours.

Use the mailbox for blockers, rule changes, "stop" or "rebase now" requests, and replies to them.
Keep routine reports on `send_session_message`.

## Reporting to the docs agent

Send a message whenever you:

- hit an error or dead end and get past it (or don't),
- need something that wasn't documented,
- find a doc, skill or script help that's confusing, wrong or stale,
- learn a recipe, convention or interface another session will need.

Use `send_session_message` with `delivery_mode: "immediate"` to the docs agent's session ID (never
default/enqueue). Keep it short, self-contained and actionable; don't wait for a reply or hold reports
until your feature lands. One message can carry several items. For a blocker or rule change that must
reach a busy session mid-turn, also send `mailbox_send` to its worktree. Older queued messages may
arrive late: honor the newest timestamp or explicit decision and ignore stale superseded instructions.
Template:

```text
[docs report] from <session name> (<branch>, port <mcp port>)
1. <kind: failure | missing | wrong/stale | recipe | interface>
   Symptom / exact error: ...
   Cause: ...            (if known)
   Fix / workaround: ... (commands, file:line)
   Time lost: ...
   Affects other lanes? yes/no
```

**Blockers:** if something blocks you for more than about 15 minutes, or affects other worktrees
(shared ports, GPU/VRAM, Live Coding, locks, a broken `main`), report it immediately with
`mailbox_send` to the docs agent's and orchestrator's worktrees (see the mailbox section above),
and also with `send_session_message`. Include what you tried.

**What lanes still write themselves:** their own OpenSpec change (tasks, design notes), docs that
belong only to their feature, and code comments. Shared docs (`.github\skills\**`,
`docs\setup.md`, `docs\version-control.md`, this folder, and cross-cutting script help) go through
the docs agent, so each finding lands once instead of five times.

## Delivering lane work (only the integration session packages)

UAT runs only in the integration session's worktree. That covers `Scripts\Build-Game.ps1 -Package` or
`-PackageOnly`, `RunUAT BuildCookRun`, and packaged-game tests (`Test-Game.ps1 -Packaged`,
`Playtest-Visual.ps1 -Packaged`). Several worktrees packaging at once fought over the machine-wide
UBT mutex (`Result: Failed (ConflictingInstance)`, UAT exit 10) and the shared Zen server on port
8558, and each package costs 20-40 minutes of CPU, disk and VRAM.

A lane delivers an increment like this:

1. Implement it, and verify it in your own editor (MCP/PIE).
2. Run the native tests: `Scripts\Test-Native.ps1 -Configuration Release`. Rebase onto `main` first
   and run them again after the rebase. Other lanes' changes can break your tests (a pail added to
   the pack broke a manor chest test). If the breakage comes from an interaction between lanes, say
   so in your `[ready]` rather than silently patching the other lane's code; the orchestrator
   assigns it.
3. **Build only when your C++ changed** (Jenny's build policy, 2026-09-28). Close your editor (and
   Blender, if it's yours) first: on a loaded machine UBT runs out of memory and retries, and a
   5-minute build took 40. Batch several fixes, then
   one editor build and one PIE pass, not a build per fix. Asset, Blender, Python and config work needs
   no build: launch with `Start-EditorMcp.ps1 -SkipBuild` if your binaries are current. When C++ did
   change, build the editor module once with `Scripts\Invoke-UnrealBuild.ps1` (it skips UBT if the module is
   already built from these sources, and logs to `Saved\Logs`). Don't compile the
   `SurvivalGame` game target; the integration session does that once per batch (unity-build clashes
   the editor build hides show up there; see the editor skill's table 0.1). Keep running the native
   tests (step 2).
4. Commit only your files. Push to `main` when you're rebased and tested; otherwise commit to your
   lane branch. All worktrees share one local repository, so the integration session can read
   unpushed lane branches directly.
5. Message the orchestrator (`send_session_message`, `delivery_mode: "immediate"`; never enqueue):

   ```text
   [ready] <lane> — branch <branch> @ <sha> (pushed to main: yes/no)
   Changed: ...
   Verified: native tests ..., editor compile ..., PIE ... (what you saw)
   Try in the packaged build: ...
   Known issues / needs from integration: ...
   ```

Message flow:

1. The lane sends `[ready]` to the orchestrator.
2. The orchestrator forwards it to the integration session and ends its turn.
3. The integration session merges the ready work in its worktree, resolves conflicts, runs the native
   tests, packages once, runs the packaged tests, and pushes the integrated result to `main`. It
   reports `[integrated] <what> @ <sha>` (with anything that failed) to the orchestrator.
4. The orchestrator tells the lanes to rebase, relays to Jenny what she can try, and assigns
   follow-ups.

**Batching (integration session):** merge `[ready]`s in batches, with one editor build, one game-target
build, one native test run and one PIE pass per batch. A single `[ready]` may wait up to about 60
minutes for company, unless it unblocks another lane. Package only at the end of a round, or when the
orchestrator asks for a playtest build.

Integration merge notes:

- All worktrees share one `.git`, so a lane's local branch can be merged without a push. Lanes
  sometimes rewrite history before pushing (for example ocean `e777db71` became `8a407908`), so
  always merge the exact SHA named in the latest `[ready]`, not the branch tip you saw earlier.
- **Generated placement safety:** any terrain/forage/road generator that emits saved placement IDs
  freezes the committed `id -> kind -> position` mapping. Never compact accepted candidates when an
  earlier candidate becomes rejected: retain reserved/skipped holes, allocate additions as new IDs and
  provide a regeneration regression proving existing cleared/harvested old-save edits still map to the
  same placement. Integration blocks the rebake/merge until that proof exists; current saves are not
  retroactively corrupt merely because this gate was added.
- Incidental `.uasset` re-saves block merges ("Your local changes ... would be overwritten"). Close
  the editor, then `git checkout -- Content` for files you didn't mean to change.
- Run the integration check with `Scripts\Test-Native.ps1 -Configuration Release` (about 3 min).
  Debug takes 16-19 min here.

**Estate-only delivery (Jenny, 2026-09-30):** `Homestead Estate.lnk` is the only active game
shortcut. The retired survival MVP is archived at
`archive/mvp-survival-20260930` (`93612cdf`); Jenny approved deletion of its
`E:\Repos\HomesteadMVP` package/saves and the retired shortcut backup, leaving the Git tag as the
only retained MVP archive reference. The retained
`jennifergalley-mvp-woodland-biome` branch is Water's active Estate Seasons handoff
(`b19a0ad0`), despite its historical name.

## Playtest builds (schedule)

Jenny's standing preference (2026-09-30): three packaged Estate builds every day.

| Slot | Freeze | Integration exclusive slot | Shortcut ready |
| --- | --- | --- | --- |
| Morning | 4:30 AM | 5:00 AM | 7:30 AM |
| Afternoon | 1:00 PM | 1:30 PM | 4:00 PM |
| Evening | 6:00 PM | 6:30 PM | 9:00 PM |

At either freeze, only work that is already **UE-verified and code-reviewed** enters the build;
everything else waits for the next slot. The first evening build under this policy is October 1,
2026 (the 9 PM window on September 30 had already passed).

**Before every Shipping build, reclaim dated release space safely:** retain the current
Estate-shortcut Shipping release, at most its immediately previous Shipping rollback, and a named
Development reference only while it is needed. Before pruning older dated releases,
`Playtest-09xx` folders or stale `Build\Windows` staging, verify no process path or shortcut target
uses them. Do not delete the current shortcut target, live save data, or the one retained rollback.

1. The orchestrator notifies lanes at the freeze; lanes close their editors
   (`Stop-MyEditor.ps1`) until the build is done, because Integration owns the Unreal slot.
2. The integration session merges admitted `main` work, runs UBT and packaged suites, makes the
   Shipping acceptance check, and
   retargets the shortcut to its `Build\Windows\SurvivalGame\Binaries\Win64\JennysHomesteadGame.exe`,
   keeping the Homestead icon. `Homestead Estate.lnk` is the only active desktop game shortcut.
   **Before retargeting to a new package folder, copy Jenny's saves and settings across:** packaged
   Development builds keep them inside the package (`<package>\SurvivalGame\Saved\SaveGames`, with an
   `Estate\` subfolder, and `Saved\Config`). Copy both from the old package into the new one, or she
   loses her game.
3. It reports `[playtest] ready @ <sha>` to the orchestrator with what's new and what to try, and the
   orchestrator relays that to Jenny.
4. If packaging or the suites fail, it leaves the last good build on the shortcut and reports the failure.

Because any scheduled build can pick up `main`, **`main` must stay playable**: push only verified work.
This replaces the old "package after every improvement" step of the Interactive Loop.

**Pause for play:** when Jenny asks to pause feature development so she can use the PC to play, every
session stops launching editors and builds until she says to resume. Close your editor and end your
turn (with a wake-up if you need one).

## Docs agent duties

- Record each report in the canonical place: `.github\skills\unreal-editor-mcp\SKILL.md` (table
  0.1 for failures), `.github\skills\blender-assets\SKILL.md`, `docs\setup.md`,
  `docs\version-control.md`, script help, or `~\.copilot` for cross-repo lessons. Verify against the
  repo first, fix or delete the stale text, and don't append contradictions.
- Commit small doc-only commits to `main` often (`git pull --rebase` first).
- When a fix affects running lanes, tell the orchestrator with a one-line broadcast suggestion (for
  example "rebase to pick up <sha>: new MCP port rule").
- Keep the round page current: registry, lane status, open blockers, decisions, what's next.
- Every few hours, or when a lane goes quiet, ask it for friction it hasn't reported.
- At the end of the round, write the handoff for the next round (below).

## OpenSpec in this repo

- Every feature round is planned as OpenSpec changes under `openspec\changes\` before
  implementation; bugs and deferred fixes are recorded there too (not GitHub issues).
- The house proposal shape is set by `openspec\config.yaml` rules. Copy a complete example such as
  `openspec\changes\add-tool-hotbar\`: the proposal has "Reuse research" and "Smallest useful result
  and first playable demonstration" sections; the design has "Lanes and ownership".
- A proposal-only stub fails `openspec validate` with `Change must have at least one delta`. Give it
  a small outcome-level `specs\<capability>\spec.md` (one or two requirements with scenarios), or
  set `skip_specs: true` in the change's `.openspec.yaml`.
- Every `ADDED` requirement needs at least one `#### Scenario:` with WHEN/THEN. Otherwise
  `openspec validate --changes --strict` fails the whole-repo gate, even when the implementation is
  unrelated to that change.
- `openspec new change` takes 3-4 s each. Scaffold many changes in one background command.
- Plan mode blocks even read-only `openspec list`; run it after plan approval.
- Completed changes haven't been archived yet, so `openspec list` includes finished work. Ask the
  orchestrator before archiving any change.
- The `create` tool doesn't make parent folders; `New-Item -ItemType Directory` first.

## Round kickoff checklist (orchestrator)

1. Spawn the docs agent first, and add it and yourself to a new `round-<n>.md` (copy the previous
   round's structure).
2. For each lane, record the session ID, branch, worktree, MCP port (unique, 8766-8799), OpenSpec
   change and owned files in the registry before or as you spawn it.
3. Every lane's kickoff prompt includes: "Read `docs\handoff\README.md` and `round-<n>.md` first.
   Send findings and blockers to the docs agent `<id>`. Your MCP port is `<p>`. Don't package:
   deliver through 'Delivering lane work' and message me when an increment is ready."
4. Point lanes at the shared-machine rules (editor skill, section 0), especially the **2-Unreal-process
   limit**, with one slot reserved for the integration session and the other shared by all lanes, one
   at a time (`Start-EditorMcp.ps1` enforces it). With several lanes, editors take turns. Lanes close their editor as soon as a verification
   pass is done. Perf measurements need the machine to themselves (one Unreal process, no builds):
   claim it with `Scripts\Start-PerfWindow.ps1`, which holds off other launches until
   `Stop-PerfWindow.ps1`.
5. `create_session` can time out creating the worktree (`git command timed out after 300 seconds`)
   and still start the session on a half-checked-out tree. Every lane's first step is to confirm that
   `git status` is clean and `SurvivalGame.uproject` exists; if not, `git reset --hard HEAD`.
6. **Confirm each spawned session actually started.** A session can be created with its CLI never
   running: one sat for 25 minutes without a commit. Within a few minutes of spawning, check
   `get_session` / `get_sessions_status` for `is_running: true` (not only `activity`), and look for
   commits or new files in its worktree. If it hasn't started, re-send the kickoff or do the work
   through a sub-agent.

## End-of-round handoff (docs agent)

Before a round closes, bring `round-<n>.md` up to date so a new agent can pick up cold:

- what shipped (commits, build, what Jenny can try) and what didn't,
- open blockers and known bugs (link OpenSpec changes),
- decisions made during the round that aren't yet in a design,
- interfaces and conventions later rounds depend on,
- the recommended first steps for the next round.
