# Tasks

## 0. Done while lanes were active (cold files only)

- [x] 0.1 `docs/architecture.md`, code practices in `.github/copilot-instructions.md`, and the `homestead-*` skills
- [x] 0.2 `UI/HomesteadPalette.h`, adopted in the hotbar, icon, map view, map painter and names widgets (`21d5e08f`)

## 1. Split the big four (between rounds)

- [ ] 1.1 `HomesteadController.cpp` → per-feature `HomesteadController<Feature>.cpp`; game target builds; PIE pass
- [ ] 1.2 `HomesteadWorld.cpp` → `HomesteadWorld<Area>.cpp`
- [ ] 1.3 `HomesteadCharacter.cpp` → `HomesteadCharacter<Area>.cpp`
- [ ] 1.4 `UI/SHomesteadMenu.cpp` → `UI/SHomesteadMenu<Page>.cpp`

## 2. Runtime cost

- [ ] 2.1 Measure Estate `LastRefreshMilliseconds` and frame time (perf window)
- [ ] 2.2 Gate `AHomesteadWorld::Refresh` on revision / ready-hour bucket / held produce; integer signatures; re-measure

## 3. Consolidation

- [ ] 3.1 Palette in menu, shop, vitals, HUD and controller
- [ ] 3.2 Table-driven `EHandAction` in `HomesteadAnimInstance.cpp`
- [ ] 3.3 Test admission and route dispatch out of `AHomesteadController::BeginPlay`
- [ ] 3.4 Named log categories replace `LogTemp`
- [ ] 3.5 Tool item enum names (`Axe`, `Hoe`, `Pail`), keys unchanged

## 4. Legacy tooling (needs the orchestrator's yes)

- [ ] 4.1 Remove `FernSpike.cpp`, the authoring-probe commandlet and its scripts
- [ ] 4.2 Remove unused policy tests and `docs/research/**/receipt.json` evidence; fix or drop `HomesteadMenuSourceTests.py`
