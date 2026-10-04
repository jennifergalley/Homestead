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
- The current round: [round-3.md](round-3.md) (scheduled builds with a lean, token-efficient team).
  Earlier rounds are history only; don't read them unless you need a specific detail:
  [round-2.md](round-2.md) and [round-1.md](round-1.md).

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

Updated 2026-10-02 (Jenny): protect visual quality, gameplay and performance while measuring AI
credits per build. This policy supersedes the earlier Opus/Sonnet/GPT-6 Sol role tiers. See
[agent-lifecycle.md](agent-lifecycle.md) for handoffs, session reuse and accounting requirements.
Use default context unless the task needs long context; record both configured tier and actual usage.

| Role | Model (exact ID) | Reasoning | Context |
| --- | --- | --- | --- |
| Orchestrator Agent | Claude Opus 5.5 (`claude-opus-5.5`) | high | long (Jenny requested 1.1M; actual runtime context remains separately observed) |
| Blender / Unreal **asset making and asset integration only** | Claude Opus 5.5 (`claude-opus-5.5`) | high | default; long only when necessary |
| Gameplay/UI/environment code, general Unreal work, visuals or performance implementation | GPT-6.1 Sol (`gpt-6.1-sol`) | high | default; long only if needed |
| Architecture / gameplay or save-format review | GPT-6.1 Sol (`gpt-6.1-sol`) | high | default |
| Documentation / straightforward status and accounting | GPT-6 Luna (`gpt-6-luna`) or GPT-5.6 Terra (`gpt-5.6-terra`) | low / medium as needed | default |
| Integration / building / scripted test execution | GPT-5.6 Terra (`gpt-5.6-terra`), GPT-6 Luna (`gpt-6-luna`), or GPT-6.1 Sol (`gpt-6.1-sol`) | low / medium as needed | default |
| Disk Cleanup Agent | unchanged (existing session) | | |

Integration escalates a failure it can't explain in one attempt to the orchestrator, which assigns it to
the owning lane rather than having Integration debug gameplay.

Claude Opus 5.5 is not the general implementation tier: only orchestrator launches and Blender/Unreal
asset creation and asset integration use it. Jenny requested a 1.1M orchestrator context; record that
request separately from the configured `long` tier and any unknown actual runtime context. These are
future-launch settings, not a retune of an existing session.

## Token budget (Jenny, 2026-10-01)

Tokens are the scarce resource. Every session follows these rules:

- **Fresh sessions, focused histories.** Bias toward a new session for a new task or unrelated work.
  Reuse a session for a tightly related follow-up when its working context is still useful. Before
  replacement or eligible archival, persist a compact handoff and its usage attribution. Cached history
  still costs credits, but restarting also costs context reconstruction; measure rather than assume.
  A replacement orchestrator starts from this page, the current round page and the linked task handoffs.
- **Short reports.** Lanes report in two lines at most: `[ready] <change> @ <sha>` plus what Jenny should
  try. No narration, recap or progress updates.
- **No acknowledgements.** Don't reply to idle notices, status updates or thanks. Message only when the
  recipient must act.
- **Batch per lane per build.** The orchestrator sends each lane one briefing per build slot with all
  of its scheduled items, not a stream of separate requests.
- **No standing check-ins.** The orchestrator wakes only for build slots (about an hour before each)
  and for incoming messages; no 30-minute polling.
- **Scheduled coordinator wake.** At each build-slot wake, read the current
  `docs/handoff/priority.json` and `docs/handoff/backlog-inbox.json` before assigning the *next*
  scheduled build. Assign only newly selected, unscheduled work; do not duplicate active lane
  ownership or turn notifications back on.
- **Review only risky diffs** (save format, gameplay logic) with the quality tier above.
- **Docs reports only for real findings**: something broke, a doc was wrong, or a recipe other lanes
  need. No running commentary.
- **Jenny verifies in-game.** Lanes don't open the editor for screenshots or self-QA unless the task
  can't be checked any other way (animation and art usually need it; UI and logic usually don't).

**At most three concurrent hands-on game-development implementers** do gameplay code, Blender,
Unreal, or game-asset work. This is a cap across active game-development work, not a role-label
exemption, and is separate from the 2-Unreal-process machine cap. The Integration Agent counts while
merging, compiling, PIE testing or packaging, but not while only coordinating; Architecture counts
while editing or building game code. Planner, backlog, and build-cost canvas-extension work is outside
this cap and needs no slot. Time-critical integration gets a slot by pausing a lane. The orchestrator
grants the next slot before a waiting lane resumes. An idle or waiting session schedules a wake-up and
ends its turn; it doesn't hold a slot by sleeping or polling.

## Roles

| Role | What it does | How to find it |
| --- | --- | --- |
| **Orchestrator** | **Coordinates only** (Jenny's standing preference): plans the round, spawns lane, docs and integration sessions, owns shared interfaces and decisions (such as the save-version bump), forwards lanes' `[ready]`s to the integration session, relays results to Jenny, assigns follow-ups, and reconciles doc conflicts. It **never builds, merges, packages or verifies**: while its turn is busy with hands-on work, queued messages from lanes can't reach it. It ends its turns promptly | The round page's registry; `get_sessions_status` ("Orchestrator Agent") |
| **Integration session** | Does all hands-on integration: merges the lane work the orchestrator forwards, resolves conflicts, builds, runs native and packaged tests, PIE and perf checks, and **is the only session that packages** (the only one running UAT). Reports `[integrated] <what> @ <sha>` to the orchestrator | The round page's registry ("Integration Agent") |
| **Docs agent** | Task-scoped session when needed. It receives actionable findings and records each once in the canonical doc, keeps shared skills and setup docs current, and leaves a handoff before replacement | The round page's registry ("Documentation Agent") |
| **Architecture agent** (code steward) | Task-scoped session for a real refactor or risky review. Owns the assigned architecture or convention change, avoids files another lane is editing, and persists findings and a handoff before replacement. Docs owns process documentation; both keep their assigned changes consistent | The round page's registry ("Architecture agent") |
| **Disk Cleanup Agent** | Daily 10:00 AM project-storage steward. Removes unnecessary project-owned scratch, renders, test output, stale build staging and excess releases from `C:`/`E:`; Jenny's current save game is the protected boundary. It verifies process paths and shortcut targets before deleting a release, and preserves the current shortcut Shipping target plus one rollback. | The round page's registry ("Disk Cleanup Agent") |
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

   An explicitly user-blocked or completed lane parks in interactive mode with no automation. Generic
   completion reminders never override an explicit user hold.

   1. Schedule a wake-up with `save_session_automation`: `interval: "once"` with a `run_at` a few
   minutes ahead, or `interval: "minutes"` with `every_minutes`. Its prompt says what to check.
2. End the turn, so the session goes idle and messages can reach it.
3. Clear the automation (`clear: true`) when it's no longer needed.

Waiting on a build or command the session itself started is fine through the tool's own completion
notification (async shells / `initial_wait`); a sleep loop isn't. For an editor slot, check
`Get-Process UnrealEditor*` once: one slot is reserved for the integration session, so if another
lane's Unreal process is already running, schedule a wake-up about 5 minutes out and end the turn. The orchestrator wakes only
for build slots and incoming messages (see "Token budget").

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

**Planner canvas:** `openspec-task-planner` reads `builds.md`, `backlog.md`, and `priority.json` for
Jenny's scheduling. Dragging reorders priority; **Top** promotes an item, **Next build** assigns it
to a build, and **Quote** exposes its player ask. **Remove** archives that OpenSpec change, so use
it only for shipped, retired, or clearly stale work. All of this is agent-free: it writes straight to
`priority.json` with no chat notification, so check its `updated` timestamp when you need to know if
Jenny changed anything. The quick backlog-entry form (title/description/screenshot) writes durable
captures to `backlog-inbox.json`; `backlog.md` is its Markdown mirror for triage and display.

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
5. **Retire secondary worktrees in the same turn:** after a per-task worktree's slice lands or is
   parked, push its branch if it must survive remotely, then run
   `git worktree remove <secondary-worktree-path>` and `git worktree prune`. Never remove a lane's
   active primary worktree or another session's worktree. Reuse one secondary worktree per lane
   rather than creating one per task; abandoned `props-*`, `water-*` and `*-0930-*` worktrees cost
   14–25 GB each, with about 5 GB each of DDC and Intermediate after a build.
6. **When Jenny's explicitly flagged items for the current build are done,** end the turn and go
   idle. Do not autonomously take an unflagged queue item. If little or nothing is prioritized, the
   orchestrator asks Jenny to schedule work.
7. Message the orchestrator (`send_session_message`, `delivery_mode: "immediate"`; never enqueue):

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

**Main-checkout Shipping promotion (Jenny, 2026-10-04):** `Homestead Estate.lnk` may target only a
build of `main` installed at `E:\Repos\SurvivalGame\Build\Windows`, never a session worktree or an
external release root. Before installation, check the shared main checkout is clean, on `main`, and
up to date; never overwrite its uncommitted work. Build and verify the candidate in Integration's
worktree, then copy its complete verified `Windows` package into the main checkout, preserving its
package-local `Saved\SaveGames` and `Saved\Config`. Hash every installed file against the source,
run `Assert-ReleaseSaveIsolation.ps1` and the F5/F9 save proof against the installed copy, and only
then retarget the shortcut and its exact package-local `-UserDir` there. The source worktree
package is staging only, never the promoted release. The first recovery moves the former Sept. 19
package to `Build\Windows-20260919-old` rather than deleting it; later promotions use the same
main-checkout `Build\Windows` location.

## Playtest builds (schedule)

Jenny's standing preference (2026-09-30): three packaged Estate builds every day.

| Slot | Build starts | Shortcut ready |
| --- | --- | --- |
| Morning | about 6:30 AM | 7:30 AM |
| Afternoon | about 3:00 PM | 4:00 PM |
| Evening | about 8:00 PM | 9:00 PM |

Start each build about an hour before its slot. Only work already **UE-verified and code-reviewed**
enters; everything else waits for the next slot. The first evening build under this policy is
October 1, 2026 (the 9 PM window on September 30 had already passed).

**Feedback-complete fast path (Jenny, 2026-10-01):** when all current playtest feedback is
addressed, Integration ships the verified build immediately rather than waiting for the next
7:30 AM/4:00 PM/9:00 PM slot. After that early delivery, lanes end their turns and clear their
wake-up automations until Jenny supplies new feedback or the orchestrator starts new work. The
scheduled slots remain the fallback cadence while feedback or verified work is still pending.

**Slot-only work (Jenny, 2026-10-01):** lanes implement only what Jenny has explicitly prioritized
for a specific build, then go idle. They do not pick up unflagged queue work. If a build has little
or no prioritized work, the orchestrator asks Jenny to schedule it.

**Before every Shipping build, reclaim dated release space safely:** retain the current
Estate-shortcut Shipping release and at most its immediately previous Shipping rollback. A named
Development reference is only temporary during active QA and is deleted after the Shipping cut.
Before pruning older dated releases,
`Playtest-09xx` folders or stale `Build\Windows` staging, verify no process path or shortcut target
uses them. Do not delete the current shortcut target, live save data, or the one retained rollback.

**Project disk stewardship (every lane):** delete your own project scratch, renders, recordings and
test output when the task that needed them ends. Keep no large binary in
`C:\Users\Jenny\.copilot\session-state\...\files`; use `E:\CopilotScratch\<session-id>` while it is
needed, then clean only the paths you own. Keep only the current shortcut Shipping release and its
immediately previous rollback; delete Development releases and `Saved\Automation` test sandboxes
after the Shipping cut unless they are actively needed. When in doubt, delete unnecessary
project-owned artifacts—but never Jenny's current save game.
The Disk Cleanup Agent performs the daily broader sweep at 10:00 AM.

1. The orchestrator notifies lanes at the freeze; lanes close their editors
   (`Stop-MyEditor.ps1`) until the build is done, because Integration owns the Unreal slot.
2. The integration session merges admitted `main` work, runs UBT and packaged suites, makes the
   Shipping acceptance check, and
   packages under its own `Build\Releases`, then—only after confirming the shared main checkout is
   clean, current, and on `main`—installs the complete verified `Windows` package at
   `E:\Repos\SurvivalGame\Build\Windows`. It hashes every installed file against its source, runs
   `Assert-ReleaseSaveIsolation.ps1` and the F5/F9 save proof on that installed copy, and only then
   retargets the shortcut to
   `E:\Repos\SurvivalGame\Build\Windows\SurvivalGame\Binaries\Win64\JennysHomesteadGame.exe`,
   keeping the Homestead icon. `Homestead Estate.lnk` is the only active desktop game shortcut.
   **Before retargeting to a new package folder, copy Jenny's saves and settings across:** packaged
   Development builds keep them inside the package (`<package>\SurvivalGame\Saved\SaveGames`, with an
   `Estate\` subfolder, and `Saved\Config`). Copy both from the old package into the new one, or she
   loses her game.
3. It reports `[playtest] ready @ <sha>` to the orchestrator with what's new and what to try, and the
   orchestrator relays that to Jenny. Integration also updates `docs\handoff\builds.md` with the
   date/slot, SHA, status and player-facing changelist, moving unshipped work to its **Later** list.
4. **Final delivery closure is mandatory:** export all allocated local call records with
   `Scripts\Export-BuildUsage.py`, then create
   `docs\handoff\accounting\reports\<build-id>.json` with `Scripts\Report-BuildUsage.mjs`. Commit
   the loader-compatible report with the delivery metadata so the planner shows the build and its
   observed costs. Then remove shipped cards from the active planner/inbox/Markdown mirror, retain
   their receipt and source evidence outside the active inbox, and refresh the delivered planner
   data and Measured build cost tab before reporting to Jenny. Preserve exact integer nano-AIU
   arithmetic and deduplication; label AIU as observed rather than billing-reconciled credits, keep
   unknown context/post-capture tails visible, and never turn Jenny's unchecked playtest acceptance
   into completion.
5. **Build-card timestamps are mandatory:** every shipped entry in `docs\handoff\builds.md` is
   headed exactly `## YYYY-MM-DD — h:mm AM/PM` using its actual local promotion time, ordered by
   that time. Put build IDs, scope, admission evidence, and descriptions in the card body—not its
   heading. An admission that did not ship belongs in the delivered build card body, not in its own
   build heading.
6. If packaging or the suites fail, it leaves the last good build on the shortcut and reports the failure.

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
