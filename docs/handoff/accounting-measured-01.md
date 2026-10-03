# Measured build 01 accounting

Build `20261002-measured-01`; authorization `2026-10-03T04:32:10.763Z`; baseline `6390d37f`.
Accounting Agent: app `a03bb295-c2b9-48dc-9b78-3144b3690e66`, runtime
`02a5caf8-3fdf-4c96-be5c-192448d6fcdf`, branch `jennifergalley-accounting-agent`,
worktree `jennifergalley-laughing-lamp`. Actual call model/effort: GPT-6.1 Sol / medium,
verified in local usage. Launch context default; actual runtime context unknown.
Implementation checkpoint `9d8807a8` is pushed to the feature branch; subsequent branch-tip
handoff/capture updates preserve its interfaces. Integration has not merged or packaged it.

## Interface and capture

No dependencies, editor, builds, cloud integration or authentication changes.
The explicit collector reads only `assistant_usage_events` using a read-only SQLite connection;
it never reads chat content. Coordinator membership is in `measured-build-01.json`;
the collector's event allocations are in `accounting/measured-01-allocation.json`.

Run from the receiving worktree after merging:

```powershell
python Scripts\Export-BuildUsage.py --database C:\Users\Jenny\.copilot\session-store.db --allocation docs\handoff\accounting\measured-01-allocation.json --out docs\handoff\accounting\measured-01-usage.json
node Scripts\Report-BuildUsage.mjs --allocation docs\handoff\accounting\measured-01-allocation.json --input docs\handoff\accounting\measured-01-usage.json --out docs\handoff\accounting\reports\20261002-measured-01.json
```

Repeat these two commands at task boundaries and **before any contributor archival**. The raw
snapshot retains previously captured records if local rows disappear; conflicting event IDs
fail rather than silently replacing history. For separately captured snapshots, repeat
`--input <path>` on the report command; duplicates count once. Do not combine shutdown/inclusive
parent totals with per-call records. Commit updated metadata/report with the build handoff.

Reload the project extension using `extensions_reload`, then reopen or refresh
`openspec-task-planner`. Report viewing, scheduling, reordering, quote attachment, removal,
backlog entry and screenshot upload remain agent-free. The planner serves summaries, not raw rows.
The existing HTTP routes are extracted to `planner-server.mjs` for direct regression testing;
`extension.mjs` remains SDK wiring.

## Allocation and limits

Parent accounting uses events after cursor `68682` **and** after authorization; its pre-kickoff
`172910230000` nano-AIU lifetime total is not charged to this build.
UI/Town runtime IDs match supplied IDs in actual usage. UI startup is through `68737`; theme
`(68737,68756]`, map `(68756,68761]`, exact slots `(68761,68789]`, then shared verification.
Town startup ends at `04:38:28Z`; crops end at `04:45:26.975Z`, then signpost work. These are
lane-reported UTC boundaries; its sampled cursors were not exact. Town's combined verification
boundary remains pending. Do not split equally or pretend inherited implementation was free.
All accounting and coordinator calls, including startup/bootstrap investigation, are overhead.
Nested helper calls in a registered runtime are captured automatically and counted once;
separate helper/review/integration runtime sessions must be added explicitly.

Segments have `sessionId`, `task`, `tasks`, `category`, `since`, optional exclusive `afterEventId`,
inclusive `throughEventId`, exclusive `until`, and optional `agentId`. Task/configuration/retry
changes require nonoverlapping segments. Call model and reasoning effort come from telemetry;
`contextTier` is actual only when supported by configuration evidence. Launch context is
`launchContextTier`, not proof of runtime context. Helpers do not inherit actual parent context.
Retry classification defaults to unknown; identify known retry ranges with category `rework`
and `retryClassification: "retry"`. Raw input tokens are prompt-size evidence, not billable input.

Recorded nano-AIU totals are authoritative per-call values. Class costs use supplied
`tokenCount * costPerBatch / batchSize` in integer arithmetic (flooring fractional nano-units).
Never add aggregate input or reasoning tokens on top of those classes. Recorded minus rated
residual is retained where comparable; estimates for calls lacking recorded totals are separate.
The view says **AIU**, not reconciled AI credits: conversion is nano / 1e9; no GitHub billing
export has been supplied. Historical main implementation remains unattributed, excluded, not zero.

Reports remain `incomplete`: the first export is real but interim. Add Integration/review
membership, precise task boundaries and runtime context evidence when available; capture terminal
responses and integration costs afterward. `finalCapture` can mark a final boundary, but does not
erase unknown legacy costs or billing coverage. No automatic capture hooks, polling or overnight work.

## Verification and checkpoint

Targeted commands (set `TEMP` and `TMP` to the lane's `E:\CopilotScratch\...\tmp` first):

```powershell
node --test Tests\BuildUsageTests.mjs Tests\PlannerBacklogTests.mjs
python -B Tests\BuildUsageExportTests.py
```

They cover real-rate cache/reasoning overlap, dedup, cursor deltas, mixed model/configuration
segments, helpers, rework classification, unknown coverage, persisted reports, archival retention,
and HTTP report/backlog/screenshot/quote/reorder/schedule/remove behavior with forbidden model calls.
The first capture's rates reconcile exactly to recorded nano-AIU. UI detector returned no findings.
Sixteen Node cases and two Python cases pass. Tests caught and fixed a Windows SQLite handle
cleanup issue and a browser-renderer scope error; no unrelated planner tests were changed.
The reloaded SDK canvas opens and its refresh action/live HTTP endpoint return the real report.
Ready-time evidence includes `128` calls through cursor `68810`, `447853020000` recorded nano-AIU
and zero recorded-minus-rated residual; later captures supersede that interim snapshot.
Accounting's recorded subtotal there is `103550090000` nano-AIU. This is new-work evidence, not a
complete build invoice. Recapture after this handoff/terminal response and later integration.
No owned editor, live test server, automation, save or release dependency. Scratch test directories
are removed by the tests; the small committed snapshots/report are the durable accounting evidence.
