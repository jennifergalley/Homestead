# Tasks

## 1. Reference and Input Baseline

- [x] 1.1 Archive links/notes for the reviewed Minecraft, Factorio, Coral Island and Dreamlight Valley hotbar screenshots/controls, recording only reusable conventions and explicit non-copy boundaries; verify every claim has a reputable source and no proprietary image enters the repo
- [x] 1.2 Record selected-build mouse wheel/camera, number keys, left click, E/F, LB/RB/R3, current tool ownership/capacity/save behavior, and 720p/4K HUD geometry so input conflicts and protected regions are reproducible

## 2. First Playable Toolbelt

- [x] 2.1 Add a gameplay-only ten-slot hotbar snapshot and original Pine/cream/Gold Slate overlay reusing current icons, with stable `1-0` labels, carried/ghost/empty/selected states and no instruction legend; verify pointer hit targets, semantic state and measured 720p/4K bounds
- [x] 2.2 Implement `1-0`, unmodified wheel wrap, pointer-click, and gameplay LB/RB selection through the accepted input path; verify direction, number `0`, one-step admission, device changes, menu tab ownership, click consumption and no world mutation
- [x] 2.3 Reassign gameplay camera distance to Ctrl+wheel while preserving R3 and Look/menu/planning ownership; verify zoom never changes selection and hotbar scrolling never changes camera distance

## 3. Ownership, Tool Use, and Persistence

- [x] 3.1 Add validated ten-slot tool-reference and selected-index save state with stable default Knife/Hatchet/Digging Stick/Watering Can assignments; verify no added capacity, duplicate/invalid sanitization, current-version save/reload and safe disposable-save reset disclosure
- [x] 3.2 Resolve slot availability live from carried inventory so chest-stored tools ghost and cannot act while returned tools recover; verify transfer, craft, load, new woodland and inventory refresh without stale ownership
- [x] 3.3 Route left mouse/controller right trigger selected Knife/Hatchet/Digging Stick/Watering Can use into existing authoritative clear/fell, till/weed and water/fill transactions; verify valid, wrong-tool, empty, stored, range, capacity, water, plot and target cases mutate exactly once or not at all
- [x] 3.4 Preserve E/A ordinary interaction plus menu/dialog/planning/failure/load/appearance isolation and existing action presentation/cancellation; verify no click-through, delayed action, duplicate reward, orphaned prop or input leakage

## 4. Integrated Acceptance and Promotion

- [x] 4.1 Run ordinary mouse/keyboard routes selecting and using all current tools by number, wheel and click, with Ctrl+wheel camera changes and rapid switching; inspect selection clarity, tool response and heroine/context visibility at 720p/4K
- [x] 4.2 Run controller parity, storage/craft/save/reload/new-world, menu/planning/recovery and full-loop routes; verify LB/RB/R3/RT behavior, authoritative state, current settings and unrelated UI remain stable
- [x] 4.3 Run focused hotbar/input/save/source contracts, full portable simulation/world/regional suites, strict OpenSpec validation and SurvivalGameEditor Win64 Development build from a clean integrated checkpoint
- [x] 4.4 Build one immutable Shipping candidate under the serialized engine slot, run fresh consumer and comparable cadence routes, update PRODUCT/DESIGN/setup/playtest docs, and promote only if the bar is useful without clutter, input conflict, copied styling or performance regression

## 5. Seed Outline and Plant Cue (Jenny's playtest)

- [x] 5.1 Add side-effect-free `Simulation::CheckSow` (`Plant` calls it first), `GardenTool::Seed` in `PreviewGarden` on the focused plot the A/E sow uses (red `Till this square before sowing.` on the hoe's next square where `CheckTillGround` passes), and `DescribeSow` for the bare-plot focus line (`Plant <seed>` keyed; refusal or `Select <seed> (<key>) to plant` unkeyed); native `SeedSowPreview` covers the valid/invalid matrix, cue text, no mutation and preview-plot == sown plot
- [x] 5.2 Build SurvivalGameEditor and run the Hotbar suite's seed steps (`garden-outline-seed-{valid,invalid}.png`) in the next Unreal slot; inspect the green/red outline and the Plant/Select cues on keyboard and gamepad _2026-09-30 slot (water-slot-1001 + Menu a37d693d):
  - The Hotbar route (editor -game) passes all steps, including the seed outline: green with "[E] Plant Seeds" before E sows, red with "A crop is already growing here." after.
  - In PIE, Spring:
    - carrot seed over untilled ground: "Till this square before sowing.";
    - turnip seed (out of season) over untilled ground: nothing;
    - tilled with the hoe, then carrot seed: "[E] Plant Carrot seed";
    - turnip seed on the tilled plot: "Turnips grow in Autumn and Winter.";
    - nothing selected: "Select Carrot seed (4) to plant", and E only repeats it without sowing;
    - carrot seed + E sows, then the red "A crop is already growing here."
  - Captures are in the Water lane's scratch (hotbar-run, slot-pie)._
