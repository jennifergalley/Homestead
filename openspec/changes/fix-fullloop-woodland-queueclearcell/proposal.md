# Fix FullLoop Woodland QueueClearCell failure

## Why
Round-4 build (`20261010-next-02`): the packaged `FullLoop` test fails identically on two runs at
"Approach stable obstruction key -1:-1:16777238 for cell (-4,-3)"
(`Source\SurvivalGame\HomesteadFullLoopTest.cpp` ~lines 490-499). This is the Woodland
positional-map route, not player-facing (Jenny plays Estate only; the packaged Shipping client
can't even select the Woodland map). The real save-proof gate (F5/F9 via
`Test-Game.ps1 -NativeSaveRetry`) passed against both the worktree's own package and the installed
copy, so the build shipped with this as a Known Issue instead of blocking.

Likely cause: Gameplay's round-4 "Start a new woodland" → "Start a new game" menu label change may
have broken a text-based selector FullLoop uses to start a fresh game, leaving a stale route
assumption about cell occupancy/obstruction state.

## What
- Root-cause whether FullLoop's new-game selector or its cached obstruction/cell state is stale
  after the label change (or another round-4 input), and fix the test or the underlying route.
- If the route itself is fine and only the test's selector is stale, update the test's selector to
  match the new label.
