## Why

Homestead is built by many agent lanes at once with little human review. The survey in
`docs/architecture.md` (section 10) found the code works and the simulation core is clean (plain
C++, `/W4`, no warnings, 7 native suites), but:

- four files carry most features and most churn: `HomesteadController.cpp` (3965 lines, 79 commits
  in 14 days), `HomesteadWorld.cpp` (4009, 59), `HomesteadCharacter.cpp` (2337, 48) and
  `SHomesteadMenu.cpp` (2983, 46). Every lane edits them, so they cause most merge conflicts, and
  every edit recompiles a 4k-line translation unit on a machine where each compile is memory-bound
  (the Build Speed Agent's finding);
- the world refresh runs 4× a second and formats a signature string for every resource, structure,
  plot and drop even when nothing changed;
- colours, per-action animation bookkeeping and test-harness code are copied instead of shared.

None of this can be fixed safely while lanes are editing those files. This change batches the
refactors for the gap between rounds, as pure moves or behaviour-identical rewrites.

## What Changes

In priority order (payoff over risk):

1. **Split the big four by feature, same classes.** Move member functions, unchanged, into
   `HomesteadController<Feature>.cpp` (Input, Hotbar, Menu, Focus, Clearing, Placement, Settings,
   Saves, Audio), `HomesteadWorld<Area>.cpp` (Estate, Woodland, Resources, Structures, Trees),
   `HomesteadCharacter<Area>.cpp` (MetaHumanStack, HeldTools, Carry) and `UI/SHomesteadMenu<Page>.cpp`.
   No logic changes; one file per commit so `git log --follow` and blame stay useful.
2. **Revision-gated world refresh.** Skip `AHomesteadWorld::Refresh` work when the simulation
   revision, the resource-ready hour bucket and the held-produce state are unchanged; replace
   per-object `FString` signatures with integer hashes. Measure `LastRefreshMilliseconds` on the
   Estate before and after.
3. **Palette everywhere.** Finish adopting `UI/HomesteadPalette.h` in `SHomesteadMenu`,
   `SHomesteadShop`, `SHomesteadVitals`, `AHomesteadHUD` and the controller (started in the cold
   widgets on `21d5e08f`).
4. **Table-driven actions in the anim instance.** One row per `EHandAction` (clip getter, start
   counter, two-handed flag) instead of parallel `if` chains and counters.
5. **Test harness out of the controller.** Move the shipping-QA admission and test-route dispatch
   from `AHomesteadController::BeginPlay` into `HomesteadTestAdmission.cpp`; break the 500-1000-line
   route builders into named step groups.
6. **Named log categories** for the 106 `LogTemp` call sites, by area.
7. **Retire legacy probe tooling** (approved; cold files, any time): `FernSpike.cpp` and the
   authoring-probe commandlet with the scripts that only drive it, the unused `Tests/Test-*Policy.ps1`
   scripts, and the failing `HomesteadMenuSourceTests.py`, after checking references. The receipts
   under `docs/research` stay.
8. **Tool item identifiers** (`Item::Hatchet` → `Axe`, `DiggingStick` → `Hoe`, `WateringCan` →
   `Pail`): enum names only, keys and order unchanged, so saves are unaffected.

Out of scope: removing the generated-woodland mode (still supported; the orchestrator decides),
UI technology changes, the save format (see `harden-save-item-stocks`).

## Reuse research

- Unreal already supports one class across many `.cpp` files; `HomesteadControllerManor.cpp` and
  `HomesteadShopFlow.cpp` prove the pattern here, including in unity builds.
- `Simulation::GetRevision()` already exists and is bumped by every state change; the world's
  `Signature` diffing stays as the second level.
- UBT's adaptive unity and the Build Speed Agent's PCH work pair naturally with smaller files.

## Smallest useful result and first playable demonstration

Step 1 for the controller alone: the game plays identically (native tests, the editor and game
targets build, a PIE pass through clearing, crafting, the shop and a save), and the next round's
lanes touch `HomesteadControllerClearing.cpp` or `HomesteadControllerMenu.cpp` instead of all
landing in one file.

## Impact

- Only between rounds, when every lane has merged and no worktree has uncommitted edits to the
  files involved (check with `git worktree list` + `git status`). Lanes rebase afterwards.
- No save, content or behaviour changes. `SimulationSaveVersion` untouched.
