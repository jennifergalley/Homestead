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
| **Orchestrator** | Plans the round, spawns lane and docs sessions, owns shared interfaces, the save-version bump, integration and packaging, and reconciles merge conflicts, including in docs | The round page's registry; `get_sessions_status` ("Orchestrator Agent") |
| **Docs agent** | Standing session for the whole round. It receives findings and blockers from every session and records each once in the canonical doc. It keeps this folder, the skills and the setup docs current, and relays cross-lane blockers to the orchestrator | The round page's registry ("Documentation Agent") |
| **Lanes** | One worktree and one OpenSpec change each. They own the files named in their design's "Lanes and ownership" | The round page's registry |

If the round page lists no docs agent, or the one listed is archived, ask the orchestrator to spawn
one (`send_session_message`). Until one exists, record findings yourself in the canonical doc.

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
(shared ports, GPU/VRAM, Live Coding, locks, a broken `main`), send the report immediately, with
`delivery_mode: "immediate"` to the docs agent and the orchestrator. Include what you tried.

**What lanes still write themselves:** their own OpenSpec change (tasks, design notes), docs that
belong only to their feature, and code comments. Shared docs (`.github\skills\**`,
`docs\setup.md`, `docs\version-control.md`, this folder, and cross-cutting script help) go through
the docs agent, so each finding lands once instead of five times.

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
   Send findings and blockers to the docs agent `<id>`. Your MCP port is `<p>`."
4. Point lanes at the shared-machine rules (editor skill, section 0).

## End-of-round handoff (docs agent)

Before a round closes, bring `round-<n>.md` up to date so a new agent can pick up cold:

- what shipped (commits, build, what Jenny can try) and what didn't,
- open blockers and known bugs (link OpenSpec changes),
- decisions made during the round that aren't yet in a design,
- interfaces and conventions later rounds depend on,
- the recommended first steps for the next round.
