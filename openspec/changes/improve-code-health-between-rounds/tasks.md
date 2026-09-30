# Tasks

## 0. Done while lanes were active (cold files only)

- [x] 0.1 `docs/architecture.md`, code practices in `.github/copilot-instructions.md`, and the `homestead-*` skills
- [x] 0.2 `UI/HomesteadPalette.h`, adopted in the hotbar, icon, map view, map painter and names widgets (`21d5e08f`)

## 1. Split the big four (between rounds)

Source and compile gates landed separately on `main` through `b3576693` (2026-09-29):
Controller 185/185, World 71/71, Character 92/92 and field-book Menu 96/96 member bodies
preserved; each passed native Release 9/9 and Editor+Game unity builds. Keep the tasks open
until Integration's copied-save PIE and Shipping checks establish runtime behavior.

- [ ] 1.1 `HomesteadController.cpp` → per-feature `HomesteadController<Feature>.cpp`; game target builds; PIE pass
- [ ] 1.2 `HomesteadWorld.cpp` → `HomesteadWorld<Area>.cpp`
- [ ] 1.3 `HomesteadCharacter.cpp` → `HomesteadCharacter<Area>.cpp`
- [ ] 1.4 `UI/SHomesteadMenu.cpp` → `UI/SHomesteadMenu<Page>.cpp`

## 2. Runtime cost

- [x] 2.1 Measure Estate refresh cost and frame time (Performance Agent, packaged 0ebd452d: the refresh tick costs 9.3-9.6 ms every 0.25 s, p95 of PlayerControllerTick; hidden behind a 17.5 ms render thread for now)
- [ ] 2.2 Gate `AHomesteadWorld::Refresh` on revision / ready-hour bucket / held produce; integer signatures; re-measure (handed to the Performance Agent, a34483d7, 2026-09-29; the Architecture Agent reviews)

## 3. Consolidation

- [ ] 3.1 Palette in menu, shop, vitals, HUD and controller
- [ ] 3.2 Table-driven `EHandAction` in `HomesteadAnimInstance.cpp`
- [ ] 3.3 Test admission and route dispatch out of `AHomesteadController::BeginPlay`
- [ ] 3.3a Narrow `HomesteadController.h` / `HomesteadSimulation.h` fan-out (menu types header, forward declarations)
- [ ] 3.3b Test harness into a Development-only `SurvivalGameTests` module (with the Build Speed Agent)
- [ ] 3.4 Named log categories replace `LogTemp`
- [ ] 3.5 Tool item enum names (`Axe`, `Hoe`, `Pail`), keys unchanged

## 4. Legacy tooling (approved; receipts in docs/research stay)

- [x] 4.1 Remove `FernSpike.cpp`, the authoring-probe commandlet and its scripts
- [x] 4.2 Remove unused policy tests; drop `HomesteadMenuSourceTests.py`
