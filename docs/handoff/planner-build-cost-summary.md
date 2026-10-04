# Planner build cost summary

## Scope and delivery

Jenny's direct request, October 4: simplify Build cost into shipped features, actual shipment
date/time, observed AI cost and coarse feature/category groups, with a current-cycle daily chart.
She confirmed **September 30 through October 30 inclusive, resetting October 31**. The chart assigns
cost to shipment day, not usage-call day; zero means no shipment, unknown costs never become zero.
Added direction: both tabs use `YYYY-MM-DD — h:mm AM/PM`, newest-first numeric ordering, visible
invalid-heading errors, and no stale historical/deferred upcoming card.

- Change: `simplify-build-costs`; source task 1.1 done, integrated Jenny check 2.1 pending.
- Branch: `jennifergalley-planner-editing-agent`; delivery SHA is the commit containing this handoff.
- Runtime/app ID: `ac593377-587f-4319-a033-b17f309be13e`.
- Worktree: `E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-cuddly-eureka`.
- WIP `6def04fe` was explicitly unverified; latest main was merged non-destructively before final
  checks. Integration alone publishes the finished commit; coordinator reloads its existing canvas.
- Jenny now exempts planner/backlog/cost tooling from the game-development implementer cap.
  No game slot remains held; no Unreal/editor/build/package/shortcut or live-data action occurred.

## Code and checks

Extension-owned files: `build-time.mjs` validates/normalizes shipment timestamps; `cost-view.mjs`
joins immutable shipped changelist facts to explicit report/manifest IDs and sums integer nano-AIU;
`billing-cycle.json` persists the confirmed dates. `planner-data.mjs`, `planner-html.mjs` and the
SDK refresh action expose the view without changing report evidence or invoking an agent.
Raw `accounting` API data remains available. No new runtime dependency.

Focused tests: `PlannerBacklogTests.mjs`, `BuildUsageTests.mjs`, `PlannerCostTests.mjs`,
`PlannerEditingTests.mjs` and the four data tests in `PlannerStatusTests.mjs`: **43 passed**,
including isolated headless Edge, desktop 1440 and narrow 390 layouts, date/time errors and ordering,
deferred-card exclusion, precise grouped totals, zero/unknown/estimated costs, cycle expiry,
malformed metadata, keyboard chart readout and fixture-file byte preservation.
The pre-existing stale `the canvas exposes` renderer assertion is excluded as previously reported.
Strict OpenSpec validation and `git diff --check` passed.

```powershell
# Set TEMP/TMP and PLANNER_BROWSER_MODULE to this task's E-drive scratch helper before browser checks.
node --test --test-reporter=spec --test-skip-pattern="the canvas exposes" Tests\PlannerBacklogTests.mjs Tests\BuildUsageTests.mjs Tests\PlannerCostTests.mjs Tests\PlannerEditingTests.mjs Tests\PlannerStatusTests.mjs
openspec validate simplify-build-costs --strict
```

Browser/server fixtures and profiles are disposable on E:, not Jenny's windows or profile. The missing
`playwright-core` helper was installed only after the dependency probe failed, solely in owned E-drive
scratch; reviewed images/helper/cache/temp are removed before delivery. No helper server remains.
The normal extension provider is registered; Jenny's live canvas was not opened or modified here.
Inbox, priority, screenshots/attachments and shared accounting records remain unchanged.

## Configuration and fresh usage boundary

- Actual model: `gpt-6.1-sol`; actual reasoning: `high`. No model change or subagent/LLM helper.
- Retained session launch context: `default`; actual runtime context tier remains **unknown**.
- This is a **new tooling task**, not prior build02 `edit-planner-feedback` or build01. Do not
  append it to their closed accounting windows. Coordinator assigns its build/overhead category.
- First new-task usage: event **72125**, `2026-10-04T19:26:26.697Z`; previous session event **69548**
  is outside this task. Include task startup, planning, pause/WIP, verification and delivery tail.
- Observed snapshot through **72542**, `2026-10-04T20:03:18.722Z`: 39 calls,
  **165154370000 nano-AIU / 165.15437 observed AIU**, not billing-reconciled credits.
- This snapshot is not final: handoff/closure/publication messages after it belong to the same
  fresh segment. Capture the final disjoint range after the lane goes idle; missing tail is not zero.

## Next

Integration admits the source, then the coordinator refreshes its existing canvas. Jenny checks
the simpler tab and exact `2026-10-04 — 11:29 AM` measured02 heading. The cycle configuration
deliberately expires on October 31: obtain the next confirmed dates rather than guessing a monthly
rollover. Reading/refreshing/scheduling/editing remain agent-free, notifications OFF.

Publish by cherry-picking **6def04fe plus the finished delivery commit**, not a wholesale branch
merge: intermediate merge `58e98a8c` retained differences in four shared policy/registry documents
relative to origin/main. Those are not this lane's edits or scope and must not overwrite the current
game-only-cap policy. All files in the two source commits are extension/tests/feature artifacts only.

Shared-doc issue reported to the coordinator: measured02's shipped changelist is one long meal
paragraph, while travel/rest/HUD are only identified by the delivery registry. Integration owns
normalizing that record into concise actual shipped-feature bullets with the provisional-art qualifier.
Do not replace shipment facts with mutable inbox titles or silently claim deferred dishes shipped.
