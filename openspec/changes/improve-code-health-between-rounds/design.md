## Context

See `docs/architecture.md` sections 2, 7 and 10 for the current structure and measurements.

## Decisions

1. **Pure moves first.** Splitting a file is a move of whole function definitions plus the includes
   they need; file-local helpers move with their only users or into a small
   `Homestead<Area>Detail.h`. No renames or logic changes in the same commit, so a reviewer can check
   a split by diffing the function bodies.
2. **Headers stay.** `HomesteadController.h` keeps one class; its members get grouped into
   commented per-feature sections that name the `.cpp` holding them. Splitting the class itself
   (components or subsystems) is a larger design question, not part of this change.
3. **Unity-build hygiene on the way.** Moving helpers exposes anonymous-namespace name clashes; give
   them file-specific names or named namespaces as they move, and compile the `SurvivalGame` game
   target after each split (the editor build hides clashes in git-modified files).
4. **Refresh gating is measured, not assumed.** Record `LastRefreshMilliseconds` and frame time on
   the Estate (perf window, one Unreal process) before and after; keep the change only if it helps.
5. **Legacy tooling removal needs a yes** from the orchestrator (and Jenny through it), because it
   deletes evidence folders.

## Lanes and ownership

| Step | Owner | Files | Depends on |
| --- | --- | --- | --- |
| 1 Controller split | Architecture Agent | `HomesteadController*.cpp/.h` | all lanes merged |
| 1 World split | Architecture Agent | `HomesteadWorld*.cpp/.h` | all lanes merged |
| 1 Character split | Architecture Agent | `HomesteadCharacter*.cpp/.h` | all lanes merged |
| 1 Menu split | Architecture Agent | `UI/SHomesteadMenu*.cpp/.h` | all lanes merged |
| 2 Refresh gating | Architecture Agent | `HomesteadWorld*.cpp`, controller tick | 1 (world) |
| 3 Palette | Architecture Agent | UI files, HUD | 1 (menu) |
| 4 Anim table | Architecture Agent | `HomesteadAnimInstance.cpp/.h` | none |
| 5 Test harness | Architecture Agent | controller `BeginPlay`, `Homestead*Test.cpp` | 1 (controller) |
| 6 Log categories | Architecture Agent | many | 1 |
| 7 Legacy tooling | Architecture Agent | `SurvivalGameEditor/FernSpike*`, `Tests/`, `docs/research` | orchestrator yes |
| 8 Tool enum names | Architecture Agent | simulation, controller, tests | 1 |

Each step is its own `[ready]` to the orchestrator; the Integration Agent merges them one at a time
with its usual batch checks. Coordinate step 1 with the Build Speed Agent (`6e131c6a`), which owns
`*.Build.cs` and the PCH.

## Sequenced plan (orchestrator-approved 2026-09-28; window scheduled after the Performance pass)

Steps run in this order. "Conflict risk" is what an in-flight lane edit to the same files would
cost; every step needs a quiet window for the files it names (check `git worktree list`, each
worktree's `git status`, and unmerged lane branches first).

| # | Step | Files | Conflict risk | Why this position |
| --- | --- | --- | --- | --- |
| 1 | Anim table (`EHandAction` rows) | `HomesteadAnimInstance.{h,cpp}` | Medium (4 lanes touched it in round 1) | Self-contained; no dependency on the splits |
| 2 | Controller split into `HomesteadController<Feature>.cpp` | `HomesteadController.cpp` (header comments only) | **High** while any lane is open; none once all have merged | Biggest merge-conflict and compile-time win; unblocks 5 and 6 |
| 3 | World split into `HomesteadWorld<Area>.cpp` | `HomesteadWorld.cpp` | High, as above | Unblocks the refresh gating |
| 4 | Refresh gating + integer signatures, measured in a perf window | `HomesteadWorldRefresh.cpp` (after 3), controller tick | Low after 3 (one small file) | Needs the Performance Agent's baseline; coordinate |
| 5 | Test admission and route dispatch out of `BeginPlay` | new `HomesteadTestAdmission.cpp`, controller | Low after 2 | Product code stops carrying test plumbing |
| 5a | Narrow the widest headers: move `FHomesteadRow`, `FHomesteadHotbarSlot` and the menu enums out of `HomesteadController.h` into `HomesteadMenuTypes.h`; forward-declare instead of including where only pointers or references are used; keep `HomesteadSimulation.h` out of headers that only need `Homestead::State` by reference | `HomesteadController.h`, UI headers, simulation headers | Medium (header edits ripple) | A change to `HomesteadController.h` or `HomesteadSimulation.h` currently rebuilds all 6 unity blobs (~400 CPU-s per target, Build Speed Agent 2026-09-28); fewer dependents means smaller rebuilds |
| 5b | Move the in-game test harness (`AHomesteadSmokeTest`, `AHomesteadVisualPlaytest` and their ~25 route files, about a third of the module's source) into a `SurvivalGameTests` module that ships only in Development builds; the controller spawns the harness by class name, and routes reach controller internals through the existing friend declarations. Anything the routes call across the module boundary needs `SURVIVALGAME_API` (expect a first round of LNK2019s listing exactly what). The Architecture Agent moves the code; the Build Speed Agent writes `SurvivalGameTests.Build.cs` (own small PCH, `SurvivalGame` as a private dependency), the `.uproject` entry (Runtime, Default) and `ExtraModuleNames.Add("SurvivalGameTests")` in non-Shipping game targets. Verify the class-name spawn in a packaged Development run, not only PIE. Rerun the Build Speed Agent's standalone `cl /Zs` include check after steps 2, 3 and 6 | new module, `*.Build.cs`, `*.Target.cs`, `.uproject`, test files, controller spawn site | Medium (test files are edited by lanes adding routes) | Gameplay edits stop recompiling test code and Shipping stops carrying it. After 5 |
| 6 | Character and menu splits | `HomesteadCharacter.cpp`, `UI/SHomesteadMenu.cpp` | High, as 2 | Same pattern as 2-3 |
| 7 | Palette in menu, shop, vitals, HUD, controller | UI files, HUD | Low after 6 (value-identical one-liners) | Cosmetic consolidation |
| 8 | Named log categories | many, one area per commit | Low (single lines) | Do per file as it's split |
| 9 | Tool item enum names (`Axe`, `Hoe`, `Pail`) | simulation, controller, tests | Medium (touches many files; keys unchanged) | Last: pure rename, easiest to redo if it conflicts |
| - | Explicit ids for hand-placed estate placements (debt item 9) | `HomesteadEstate.cpp` | Medium (lanes add placements) | Only if a lane is about to reorder placements; otherwise documented rule suffices |

Steps 2-4 and 6 are coordinated with the Build Speed Agent (`6e131c6a`, owns `*.Build.cs` and the
PCH) and the Performance Agent (step 4's measurement). Each step is its own `[ready]`.

## Verification per step

- `Scripts\Test-Native.ps1 -Configuration Release` 9/9.
- `Build.bat SurvivalGameEditor` and `Build.bat SurvivalGame` (game target, to catch unity clashes).
- One PIE pass on the Estate: move, clear with two tools, craft, open the book and the shop, save and
  load.
