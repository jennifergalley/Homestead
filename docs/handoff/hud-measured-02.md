# HUD measured-02 checkpoint

**Not ready.** Source-only delivery for `20261003-measured-02`, selected by Jenny at
`2026-10-04T02:19:32.169Z`; Oct 3 9 PM / Oct 4 7:30 AM fallback. Each change has
its source task checked and compile/player acceptance unchecked.

## Identity and persistence

- Branch `jennifergalley-hud-agent`; worktree
  `E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-potential-happiness`.
- App session `84aadda6-c29e-4852-b07f-e3cbacd4114d`; runtime
  `2424d5bc-bc5a-4eec-a37f-a2f82cb995d0`.
- Latest source SHA `a3d7c6a8d59bce0fc949b370d0a3fa5a862dcd4a`, pushed and matched
  to `origin/jennifergalley-hud-agent`; original clean/complete base `dec8e135`.
  No reset/restore, main integration, release or shortcut action.
- Actual model/effort `gpt-6.1-sol` / high, confirmed by runtime usage.
  Launch requested high/default; actual context tier unknown. No model changes or helpers.

## Selected feedback -> source

| Selection | Implementation |
| --- | --- |
| `backlog:jenny-mut6p856-a7gysc` / `emphasize-chest-hud-names` | `HomesteadHUD.cpp::DrawInteractCue`: larger primary-ink chest title, 20-screen-font-unit floor, measured title/action wrapping. Keyed verbs and existing retirement remain. |
| `unify-hud-notices` | `HomesteadHUDNotices.cpp` and `UI\HomesteadHudNoticeLayout.h`: existing parchment/EB Garamond focus, explicit notice and aggregated gain slips use one bounded, stacked HUD band clear of calendar/vitals. |
| Preserve meaningful feedback | `HomesteadControllerFocus.cpp`: quiet success retains active notices; separate `QuietActionSerial` preserves hint learning without restarting book notices. `HomesteadControllerHotbar.cpp`: redundant tool-name toast removed; selection, cancellation and audio unchanged. |

`HomesteadControllerPickups.cpp` gain detection/aggregation is unchanged. Old floating
`UI\SHomesteadPickups` rendering/root/anchor is removed; its exact 2.6/.15/.6-second timing
and four-line cap moved to `UI\HomesteadPickupTiming.h`. Explicit success/error clocks
remain 5/8 seconds. No simulation, catalogue, save format, crop, tool or fishing rules changed.

`HomesteadFeedbackTest.cpp` adds synthetic quiet-success/hotbar notice-retention and hint-learning
cases for exact Travel unlock text and `Too tired`; its existing sandbox guard remains.
`HomesteadFeedbackMeasurements.cpp` compares all non-whitespace characters so hard-wrapped
tokens remain valid; bounds/overlap checks remain. Gallery `focus-chest` uses the 24-character
`Wide Winter Wool Storage` with refusal/gains; `hud-pickups` uses the common slip.

## Checks and next gate

Done: both strict OpenSpec validations; `git diff --check`; direct source review;
source-only comparison of unchanged focus/action/retirement, gain aggregation and save/travel
functions; font-floor/band formulas at 720p/1080p/4K. Impeccable mechanical scan returned `[]`;
its context detector misclassified Unreal as web, so existing native theme was preserved.
These checks do **not** establish compilation, rendering, contrast or real overlap correctness.

**Not done:** Unreal/editor/game compile, Live Coding, feedback regression execution, gallery
captures, keyboard/gamepad 720p/4K acceptance or independent risky review. No UAT/package.
Existing native HUD-timing executable was absent; no native compilation was attempted.
Next: coordinator explicitly lifts compile hold and assigns permitted verification/review;
then execute the two changes' remaining tasks. Jenny is playing; Farming alone owns the lane editor.

Integration must preserve Farming checkpoint `249229b0d5c3f072d4fb6384e9ded0cb0f503d9c`
(`ShouldRetire` / selected seeds on tilled ground) and fishing-widget mounting when merging
adjacent controller-header/Hotbar hunks. Those seed functions are untouched here. Travel's exact
`Fast Travel Destination Unlocked: <Location>` and first-visit/no-repeat path are untouched.
Stale simulation-header comment naming the removed pickup widget was left unchanged by scope.

## Usage and parking

`accounting\hud-measured-02-allocation.json`, `-usage.json` and `-report.json` contain a
single non-overlapping runtime segment for both interwoven selected tasks, including startup,
planning, source checks and handoff costs. No invented per-feature split or helper costs.
Snapshot through event `69756`: 49 recorded calls, `180152410000` nano-AIU;
actual prompt maximum `209319` tokens does not establish configured context tier.
Report totals are recorded integer nano-AIU, **not reconciled billed credits**; coverage is
incomplete, with final commit/push/message tail and future authorized checks excluded.
Integration should close/extend the boundary once, without duplicating these events.

No owned editor/port/process, release dependency, scratch, save or automation. Release slot 2
on parking and request interactive mode through the coordinator; no polling or overnight work.
Canonical policy: `README.md`, `round-3.md`, `agent-lifecycle.md`, `measured-build-02.json`.
