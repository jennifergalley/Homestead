# Tasks

- [x] 1.1 `Homestead::NightLightAt` schedule and native tests (no brightening after 18:50, night key, no pops, no moon glare, smooth sky blend, the old schedule's failure reproduced).
- [x] 1.2 `UpdateLighting` wired to it; CVar defaults 0.2 / 0.3 / -1 (the test checks them against `HomesteadWorldLighting.cpp`).
- [x] 2.1 Editor build; PIE at 18:00, 19:00, 21:00 and midnight. _Done 2026-09-30 (ray tracing off in the agent editor): 18:00 warm dusk; 21:00 in town nearly black; midnight dim blue moonlight that doesn't read as sunlight (`night_meadow_*`, `town_square_21h.png`). 21:00 may now be too dark: judge in 2.2._
- [ ] 2.2 Integration: packaged ray-traced fixed-camera captures (clear and rain) at 18:00, 19:00, 21:00 and midnight; check that lamps and the hearth read well. Tune the CVars if needed, then update the editor skill's default row.
