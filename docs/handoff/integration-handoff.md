# Integration Agent handoff — retiring 2026-10-12

Session `09ae4063-4f60-475e-bb88-7c6d6a7bbc7c` ("Integration Agent"), worktree
`E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-miniature-memory`, branch
`integration-batch`. Retiring per Orchestrator (`fc0ce9e2-4586-4dec-8f40-6561960819c3`) because
context compacted repeatedly during the last closure. A fresh Integration session takes over
future merges; **no merge was started in this handoff turn**, only the explicitly requested
Balance docs cherry-pick.

## Current state

- `main` is pushed at **`0e4e9c26d`** (this handoff + three cherry-picked Balance docs commits
  land right after it; see below).
- Installed/promoted package: `E:\Repos\SurvivalGame\Build\Windows` (shortcut target), commit
  `6c7b1f8e6136afe7670341deda5384e2ee2d5e35` (mine-only build: Mine Art + Mine Gameplay + a Nanite
  usage-flag fix on `M_Field`), packaged 2026-10-12T08:03:38Z, installed 08:58:28Z. All packaged
  acceptance (EstateSmoke/ToolRepeat/F5F9) passed per `Build\Windows\build-receipt.json`.
- Rollback kept: `Build\Windows-20261010-morning` (one rollback, per the prune-to-current-plus-one
  rule).
- **`Build\Releases` is NOT a rollback folder** — it holds old dated capture/profiling artifacts
  (9/19–9/24, e.g. `20260923-ui-fiber-01`), unrelated to package promotion. Don't confuse it with
  `Build\Windows-*` rollbacks.
- `docs\handoff\builds.md` has the correct `20261011-next-01` card with real accounting (2,682
  calls, 41,025.37 AIU, 100% real/recorded) — commit `33c60917f`.
- Accounting for the `20261010-next-02` card was corrected directly by the Orchestrator (not this
  session) in `d009a9a25`: 4,663 calls, 115,296.4 AIU, after fixing a 7-hour allocation-window
  offset and reading `assistant_usage_events` from local `~/.copilot/session-store.db` rather than
  `session_store_sql`. Do not overwrite that commit.
- Cherry-picked onto `main` this turn from `jennifergalley-balance-agent-2c8` (all Balance docs,
  no code): `027b5d7ec`→`a20520c5c` (roots rebalance; had a doc-conflict, resolved additively),
  `d7473c222`→`38e71dbee` (beach refill timing, plot revert), `8a649a2f2`→`0e4e9c26d` (beach
  refill gap correction). All three touch `docs/handoff/balance-agent.md` and/or balance tuning
  docs only.

## This worktree (`jennifergalley-miniature-memory`) — safe to remove

Branch `integration-batch` tip `9a2ffe857` is the direct parent of main's merge commit
`b8ed9677c` (the mine-build closure merge) — its content is fully captured in `main`. No unpushed
or unique work remains here. Recommend `git worktree remove
E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-miniature-memory` then `git worktree
prune` once this session is archived.

## Merge/packaging gotchas (accumulated this round)

- **Binary conflicts can't be merged by picking a side.** `EstateScenery.bin` and the 135
  `__ExternalActors__` landscape files hold several lanes' separate regenerated data. Take one
  side (usually the terrain/map lane's), then rerun `Scripts\Terrain\beach_detritus.py` (re-adds
  scatter/decor counts) and `Scripts\Terrain\build_landscape_holes.py` +
  `PaintEstateLandscapeHoles` (re-opens any authored holes, e.g. a cave/mine mouth). Verify
  affected placement-kind counts and walk the hole in PIE afterward.
- **Nanite usage-flag bug pattern:** a mesh renders Default Material only in packaged builds when
  the **base** material (not the instance) lacks `bUsedWithNanite`/`bUsedWithInstancedStaticMeshes`.
  Search the editor log for `missing usage flag` (once per material per session). Fixed this build
  on `M_Field` (`6c68440ee`).
- **FullLoop Woodland route has a known non-blocking failure** ("Approach stable obstruction key
  ...") — not player-facing (Jenny plays Estate only). Use the Estate F5/F9 save/load proof
  (`Test-Game -Packaged -NativeSaveRetry` or the startup probe) as the real save gate instead of
  FullLoop. Record the Woodland failure in builds.md Known issues; don't block promotion on it
  without a ~15 min investigation first (check route label text changes first — FullLoop may
  select steps by button text).
- **Retired placement ids must be in the retired list** and checked against a pre-change save for
  compatibility before any build that removes/replaces world placements.
- **New placement id ranges and Manor/Simulation test allow-lists** must be updated together — a
  lane adding new ids (e.g. mine's 588000-588199) needs the test id-range allow-list bumped in the
  same merge, or native tests fail on "unexpected placement".
- **Accounting reports: real data only, never fabricate/estimate.** Use
  `Scripts\Export-BuildUsage.py` + `Scripts\Report-BuildUsage.mjs`, reading
  `~/.copilot/session-store.db`'s `assistant_usage_events` table (not `session_store_sql`).
  Anchor the allocation window to the actual kickoff timestamp **in the correct local timezone** —
  a 7-hour offset error was caught and fixed by the Orchestrator this round. If real data truly
  can't be obtained, label the report "unavailable" with a reason; never publish an "estimated"
  number into builds.md (the planner rejects it as "Invalid recorded cost" and it must be removed).
- **Keep editors closed during packaging** (`Build-Game.ps1 -Package`/`-PackageOnly`); it locks
  `.uasset`s and the game module DLLs. Close Jenny's own running package from `Build\Windows`
  before repackaging into the same folder — don't build to a side folder.

## Closure-step checklist (recap)

1. Merge each lane's exact approved SHA into the integration worktree's batch branch; resolve
   binary conflicts per the recipe above; rerun native tests (Release, ~3 min).
2. Full PIE pass covering the batch's checklist items (and any Balance-conditioned capture, e.g. a
   daylight exterior screenshot).
3. Merge the batch branch into `main` in the main checkout (`E:\Repos\SurvivalGame`).
4. `Build-Game.ps1 -Package` (Development), then EstateSmoke/ToolRepeat and the Estate F5/F9 save
   proof on the installed package.
5. Install into `E:\Repos\SurvivalGame\Build\Windows`, copying saves forward; retarget
   `Homestead Estate.lnk` (keep its icon); prune old `Build\` folders to current + one rollback.
6. Push the batch merge to `origin/main`.
7. Closure commit: `docs\handoff\builds.md` card in the README's exact format (Ships list, build
   ID, Known issues), a real accounting report (`Export-BuildUsage.py` + `Report-BuildUsage.mjs`),
   remove shipped cards from the planner/backlog, move "Build after next" items up to "Next",
   archive the shipped OpenSpec changes. Commit and push.
8. Send `[playtest]`/`[integrated]` to the Orchestrator with the final SHA and a one-line summary
   of what shipped, what was verified, and any Known issues.

## Model / usage (this session)

- Model: **Claude Sonnet 5** (per this session's active model info), reasoning effort/context tier
  as configured for the Integration role per `docs\handoff\round-4.md` (Claude Sonnet 5 / medium).
- AIU/usage: not independently re-exported in this closing turn (no new build work occurred here,
  only a docs cherry-pick + this handoff). The mine-build's real accounting is already recorded and
  pushed in `docs\handoff\builds.md` (`20261011-next-01` card, commit `33c60917f`, 2,682 calls /
  41,025.37 AIU) and the `20261010-next-02` card (Orchestrator's `d009a9a25`, 4,663 calls /
  115,296.4 AIU). This handoff turn itself was not separately measured; label any figure for it as
  **unavailable** rather than estimate one.

## Unfinished / nothing else pending

The mine-only build (round 4 interim delivery) is fully shipped, installed, promoted, and closed.
Nothing build-related is left outstanding except this handoff itself. The next Integration session
can start cleanly from `main` at the SHA above.
