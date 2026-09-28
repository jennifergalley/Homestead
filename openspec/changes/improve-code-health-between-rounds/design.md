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

## Verification per step

- `Scripts\Test-Native.ps1 -Configuration Release` 7/7.
- `Build.bat SurvivalGameEditor` and `Build.bat SurvivalGame` (game target, to catch unity clashes).
- One PIE pass on the Estate: move, clear with two tools, craft, open the book and the shop, save and
  load.
