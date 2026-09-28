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
- The current round: [round-1.md](round-1.md).

## Roles

| Role | What it does | How to find it |
| --- | --- | --- |
| **Orchestrator** | Plans the round, spawns lane and docs sessions, owns shared interfaces and the save-version bump, merges ready lane work, **is the only session that packages** (and runs packaged tests), and reconciles merge conflicts, including in docs | The round page's registry; `get_sessions_status` ("Orchestrator Agent") |
| **Docs agent** | Standing session for the whole round. It receives findings and blockers from every session and records each once in the canonical doc. It keeps this folder, the skills and the setup docs current, and relays cross-lane blockers to the orchestrator | The round page's registry ("Documentation Agent") |
| **Lanes** | One worktree and one OpenSpec change each. They own the files named in their design's "Lanes and ownership" | The round page's registry |

If the round page lists no docs agent, or the one listed is archived, ask the orchestrator to spawn
one (`send_session_message`). Until one exists, record findings yourself in the canonical doc.

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

Use `send_session_message` with `delivery_mode: "enqueue"` to the docs agent's session ID. Don't
wait for a reply, and don't hold reports until your feature lands. One message can carry several
items. Template:

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

## Delivering lane work (only the orchestrator packages)

UAT runs only in the orchestrator's worktree. That covers `Scripts\Build-Game.ps1 -Package` or
`-PackageOnly`, `RunUAT BuildCookRun`, and packaged-game tests (`Test-Game.ps1 -Packaged`,
`Playtest-Visual.ps1 -Packaged`). Several worktrees packaging at once fought over the machine-wide
UBT mutex (`Result: Failed (ConflictingInstance)`, UAT exit 10) and the shared Zen server on port
8558, and each package costs 20-40 minutes of CPU, disk and VRAM.

A lane delivers an increment like this:

1. Implement it, and verify it in your own editor (MCP/PIE).
2. Run the native tests: `Scripts\Test-Native.ps1 -Configuration Release`.
3. Compile-check the editor module: `Build.bat SurvivalGameEditor Win64 Development
   "-Project=<worktree>\SurvivalGame.uproject" -WaitMutex -NoHotReloadFromIDE`.
4. Commit only your files. Push to `main` when you're rebased and tested; otherwise commit to your
   lane branch. All worktrees share one local repository, so the orchestrator can read unpushed
   lane branches directly.
5. Message the orchestrator (`send_session_message`, `delivery_mode: "enqueue"`):

   ```text
   [ready] <lane> — branch <branch> @ <sha> (pushed to main: yes/no)
   Changed: ...
   Verified: native tests ..., editor compile ..., PIE ... (what you saw)
   Try in the packaged build: ...
   Known issues / needs from integration: ...
   ```

The orchestrator merges ready lane work in its worktree, resolves conflicts, runs the native tests,
packages once, runs the packaged tests, and pushes the integrated result to `main`. Then it tells
the lanes to rebase and reports to Jenny what she can try.

The separate MVP survival line (`mvp-survival`) packages its own build to
`E:\Repos\HomesteadMVP\Windows`, only for real deliverables, and tells the orchestrator before
starting.

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
4. Point lanes at the shared-machine rules (editor skill, section 0).
5. `create_session` can time out creating the worktree (`git command timed out after 300 seconds`)
   and still start the session on a half-checked-out tree. Every lane's first step is to confirm that
   `git status` is clean and `SurvivalGame.uproject` exists; if not, `git reset --hard HEAD`.

## End-of-round handoff (docs agent)

Before a round closes, bring `round-<n>.md` up to date so a new agent can pick up cold:

- what shipped (commits, build, what Jenny can try) and what didn't,
- open blockers and known bugs (link OpenSpec changes),
- decisions made during the round that aren't yet in a design,
- interfaces and conventions later rounds depend on,
- the recommended first steps for the next round.
