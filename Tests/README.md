# Portable homestead simulation tests

This target builds the same C++17 simulation used by Unreal, without engine
headers, third-party libraries, or exceptions. Checks execute in Release builds.
No test writes to the filesystem or requires network access.

From a Visual Studio developer command prompt:

```powershell
cmake -S . -B build-native -DCMAKE_BUILD_TYPE=Release
cmake --build build-native --config Release
ctest --test-dir build-native -C Release --output-on-failure
```

If CMake is unavailable, use the installed MSVC compiler in its initialized
developer environment:

```powershell
cl /nologo /std:c++17 /O2 /W4 /EHs-c- /D_HAS_EXCEPTIONS=0 /I Source\SurvivalGame\Simulation Source\SurvivalGame\Simulation\HomesteadSimulation.cpp Tests\HomesteadSimulationTests.cpp /Fe:build-native\HomesteadSimulationTests.exe /Fo:build-native\
build-native\HomesteadSimulationTests.exe
```

## Integration rules

`Scripts\Test-HotkeySafety.ps1` exercises exact F5/F9 callback, viewport/full
ShowFlags and screenshot-request behavior, real save errors, repeated actions,
synthetic preview-profile persistence and a normal unautomated saved-preview
startup. Use a fresh output; see `docs\hotkey-safety-playtesting.md` for scope,
isolated destinations and the engine's deliberate non-Lit mode policy.
`Test-Game.ps1 -RequireLit` and `Test-FeedbackLayout.ps1 -RequireLit` observe
every active smoke tick and reject an unexpected mode/ShaderComplexity flag or
missing guard report. They do not set Lit. Do not use that guard with the focused
hotkey fixture's intentional non-Lit preservation step. The hotkey test flag
alone does not spawn a test actor or change normal physical-input policy.

The actual-engine smoke route is separate from this portable target. Automated
smoke and visual runs accept only `FInputKeyEventArgs::IsSimulatedInput()` events;
normal gameplay still passes physical events through the same controller input
handler. This prevents another game using the same controller from changing a
test's menu selection or walking away from its resource target.

The smoke route includes deliberate physical-source keyboard/gamepad rejection
probes, followed by the real mapped synthetic controls. It checks the runtime
animation instance, relaxed stance width, speed-calibrated walking, return to
idle, and exact resource focus before gathering. Per-step engine-log traces
include position, velocity, focus, appearance values, cumulative game time and
crop growth/moisture/weeds. Assertions are not retried until lucky or weakened
to accommodate physical-input interference.

`Scripts\Characters\verify_locomotion.py` checks the exported animation files
with Blender; `Scripts\Import-Locomotion.ps1` verifies their persisted Unreal
references in a fresh editor process. Neither replaces ordinary-play image
review. See `docs\visual-playtesting.md` for evidence and sampling limits.

The smoke route checks the restored baseline's actual runtime Default Lit
skin/eye shading, masked eye blend mode and current skin/iris color parameters
after every successful step. It also rejects the removed experimental skin
scatter and wet-eye parameters. This covers material binding during all wardrobe
changes and after save/load, rather than checking only stored menu indices.
These expectations deliberately describe the restored baseline, not acceptance
of the visually rejected subsurface/clear-coat experiment.
Material-shading-model checks do not by themselves establish the viewport's
render mode; use the separate `-RequireLit` guard for that assertion.

For a controlled material comparison, use `Test-Game.ps1 -Presentation` with
fresh `-OutputDirectory` values and matching `-Width 1920 -Height 1080`.
It captures fixed front/three-quarter noon and 22:00 firelight views plus an
alternate skin/eye color. This fixture disables controller simulation ticking,
uses a copied visual state, freezes the existing idle clip at time zero, and
uses a test camera without custom lights/exposure. Day-to-night adaptation
settles for 18 seconds. It is explicitly **not** ordinary-play, full-loop or
human visual approval. `-Presentation` cannot be combined with `-FullLoop` or
`-WithAudio`. Run full-loop and normal visual routes separately.

`Test-Game.ps1 -HairLength` reuses that isolated fixture for all six long-wave
body/outfit combinations at noon, with full-upper-body back and three-quarter
views (1920x1080). It asserts the exact selected skeletal mesh. Do not combine it
with `-Presentation`, `-FullLoop` or `-WithAudio`.
`Review-HairLength.py BASELINE --candidate CANDIDATE --output REVIEW` verifies
matching actor/camera/mesh/head framing before making labeled local sheets.
Original captures retain their requested dimensions; review sheets are cropped
and scaled, never evidence of native frame rate.

The first length experiment passed authoring/import/functional checks but failed
visual review and was restored. Its local diagnostic evidence is described in
`docs\visual-playtesting.md`; no experimental export/import wiring is retained.

`Test-Game.ps1 -Gathering` runs the picking lifecycle separately from full-loop,
presentation and audio fixtures. It uses mapped A/E controls and isolated
teleport setup, not ordinary walking evidence. Expected inventory/resource
state comes from exactly one harvest on a simulation copy; only real mapped
interactions mutate the running game. It checks visible hand travel, planted
toes, unchanged actor/camera, natural recovery, depleted rapid input, stick
interruption, paused menus/Look/planning, saving during action, load cancellation,
color-only and mesh swaps, and range/full-pack rejection. Its pack is filled by
actual mapped gathering, not a mutable-state test backdoor.

All smoke modes additionally reject an active hand-action pose after settled menu,
planning or failure steps. Keep the existing full-loop route intact and run it
against the same fresh package. `verify_gathering.py` independently checks the
FBX bind, idle seams, duration, planted toes and absence of bone scaling.
The ordinary visual route settles after approaching an actual berry/flower,
orbits through mapped camera controls, records a picking dwell and recovery,
and fails unless gathering, action presentation and recovery are all observed.
These tests do not certify exact contact, human comfort or full-motion quality.

`Test-Game.ps1 -Watering` runs 181 focused checks on a separate fresh test save.
Setup gathers/crafts/tills/plants through real mapped transactions, with clearly
test-only teleport travel. It checks A/E, a same-frame fully-wet rejection,
no-can/empty/exhausted/range rejection, exactly one water debit and expected
moisture/growth after normal clock advancement, coalesced action requests,
movement, paused book/Look/planning, save/load and all 18 appearance combinations.
The attached prop must be collision-free, hand-bound, world scale one and
bounded in actual world units; checking only socket position missed an initial
oversized-prop defect. Color changes must not recolor the prop, and cancellation
must hide it. The full-loop route additionally checks that mature crop harvests
consume no water and start no watering action; its existing failure/retry
assertions remain intact. Action fixtures cannot be mixed with other modes.

`Playtest-Visual.ps1 -Watering` is different: it starts from the normal clearing,
walks to five supply patches, crafts both tools, refills at the stream, tills,
plants and waters using mapped controls. There is no injected save or live
state/time edit. Ordinary crafting retains its existing time cost. Setup
screenshots are sampled at 1 Hz, final full-body/tool action at requested 8 Hz.
Run `Review-Watering.py <capture-folder>` for the local sheet and measurements.
The observer verifies queued inputs after processing, not in the submitting
tick; the launch script also checks the recorded gameplay outcome independently
of process exit. Actual raw dimensions are verified before accepting evidence.

`Test-Game.ps1 -Weeding -FixtureSave <dedicated-test-save>` adds 37 focused
checks using the existing step runner. `Initialize-TestWorldFixture.ps1` accepts
only a saved envelope under `Saved\Automation`, copies it byte-for-byte into an
empty output `SmokeSave`, and records the source/hash/setup in `fixture.json`.
Engine loading still performs the real checksum/schema validation. Never use a
player save or describe this as a fresh-start setup. Functional travel is
explicitly test-only teleporting.

The weeding fixture requires an immature planted plot with weeds >= 0.125,
plus existing water/tool stock and an available berry patch for alternation.
It checks X/F, same-frame already-clean rejection, exactly the expected weed
reduction without inventory/water/moisture/resource mutation, visible motion,
normal recovery, movement, paused book/Look, planning, appearance and save/load.
Real water/weed and gather/clear transactions exercise arbitration, not merely
direct animation requests. Watering and weeding share the clock-adjusted
expected-state helper; it now also checks weeds and resource state. The existing
full loop additionally rejects pick starts during berry planting, tilling and
fueling. Existing failure/retry and all-appearance assertions are retained.

`Playtest-Visual.ps1 -Weeding -FixtureSave <dedicated-test-save>` loads the same
explicit fixture, then walks to a visibly weedy plot and interacts through
mapped X. It makes no debug position/time/state changes after loading. The
recording includes saved test appearance/location; source setup used functional
teleports and ordinary sleep. Review with
`Review-Watering.py <capture-folder> --weeding` (shared action review tooling).
Approach sampling is requested at 2 Hz, action at 8 Hz; neither is game FPS.
The script requires real weed removal, one visible action, recovery and no can
or water debit. Functional fixtures and visual recordings use separate outputs.

`Test-Game.ps1 -Clearing` uses fresh mapped gather/craft setup with explicit
functional-only teleport travel. Its 188 checks cover the real hatchet
requirement, X/F, single clear/yield/resource-ID and normal energy progression,
rapid E/F/F branch harvest/clear followed by sapling clear, repeats, movement,
paused book/Look/planning, save/load and all 18 appearance scale/grip/hide/tint
cases. It fills the pack through real gathering: a ready sapling rejects an
unaffordable ten-item yield, while a depleted sapling legitimately clears for
zero yield. Do not invent a depleted/full-pack rejection that the rules lack.
Sapling A/E harvesting remains distinct and does not swing the contextual tool.
The shared expected-state helper also preserves `nextId`, failure state and
energy after ordinary clock advancement. All smoke modes reject orphaned
hatchets, ownership failures and simultaneous can/hatchet visibility.

Existing 181 watering checks additionally challenge an active watering action
with a pending clear request; no competing hatchet or lost/double transaction is
allowed. Full-loop no-pick checks for berry planting/tilling/fueling also count
clearing starts. Existing failure/retry, crop, save and color assertions remain.

`Playtest-Visual.ps1 -Clearing` is a different, fresh-start ordinary route:
walk to three supply patches, gather, craft a real hatchet, stage/approach an
actual sapling, orbit/zoom with mapped controls and press X. It shares the
existing supply/craft/walking observer rather than a second runner. No injected
save or debug position/time/state changes are used; normal crafting retains its
existing time cost. Review with
`Review-Watering.py <capture-folder> --clearing`. The review requires one action,
actual permanent clearance and eight-branch/two-fiber yield, world scale one,
bounded tool size, no water/can side effect and recovered hidden tool. Sampling
and an immediately disappearing sapling do not establish impact synchronization
or continuous smoothness.

- `RecipeRequirements` and `PieceRequirements` return stable `const char*`
  descriptions generated from the same costs used by transactions, including
  tool/fire/foundation prerequisites. HUD callers need not duplicate cost tables.
  `Craft` and `Place` never advance time themselves; the engine caller applies
  its action duration once after a successful result.
- Coordinates and distances are centimeters. Cells are 300 cm, centered on
  `(cell + 0.5) * 300`. Build centers must be within 700 cm; gathering, garden,
  sleep, storage, and fueling interactions use 300 cm. Cooking and fire warmth
  reach 450 cm. Valid cell indices are -13 through 12.
- Wall/doorway rotations are quarter-turn indices: 0=north (+Y), 1=east (+X),
  2=south, 3=west. Signed turn counts normalize modulo four; saved rotations
  must be canonical 0–3. Shared edges cannot be occupied twice.
  A doorway is a self-closing door. Shelter requires foundations, roofs, and
  every exterior edge enclosed, including across connected rooms.
- Foundations support walls/roofs. A bed, chest, or fire can stand outdoors or
  on a foundation, but furnishings cannot share a cell. Plants block buildings;
  uncleared saplings block buildings and garden plots. Picking other forage
  does not obstruct construction.
- `Transfer` uses positive amounts for pack-to-chest and negative for
  chest-to-pack. Both have a 120-item total capacity; every transaction is
  all-or-nothing. Nothing is silently dropped. Water refills the carried stock
  to six, requiring enough inventory room for the full refill.
- One branch fuels one selected fire for four game hours, up to 48 hours.
  Fires burn independently, including during sleep and while far away.
- Tool costs: hatchet 4 branches/3 stones/2 fiber; digging stick 3 branches/
  1 stone; watering can 3 branches/2 fiber. Tools are not consumed.
- Pot-free recipes: roasted roots use 2 roots; herbed roots use 2 roots and
  1 meadow herb. Both require a nearby fueled fire. Berries are raw-edible.
- Build costs (branches/stones/fiber): foundation 4/2/0; wall 3/0/1;
  doorway 4/0/1; roof 4/0/3; fire 3/4/0; bed 4/0/4; chest 5/0/2.
- `CropKind` contains `Roots` and `Berries`; `CropName` supplies the HUD label.
  `Plot.kind` defaults to roots and is appended to the struct so existing
  aggregate initialization remains valid. `Plant(id, player)` still plants roots;
  `Plant(id, player, CropKind::Berries)` plants a berry bush.
- Wild roots give 2 roots and 2 seeds. Root planting consumes 1 seed; mature roots
  give 4 roots and 2 seeds and leave the plot bare. Roots have a 30-game-hour
  baseline growth duration at continuously perfect moisture and no weeds.
- Berry planting consumes exactly 1 foraged berry, using the seeds from its fruit;
  it does not consume the root-seed inventory item. Each mature harvest gives
  6 raw-edible berries and resets growth to zero while retaining the planted bush,
  crop kind, moisture, and weeds. Initial growth and subsequent fruit regrowth
  share a slower 42-game-hour baseline. An established bush cannot be overwritten
  by planting another crop. Harvesting with insufficient pack space changes nothing.
- Both crops share forgiving tending: dry soil and weeds slow growth but never kill crops.
  Rain occurs 09:00–15:00 on every third day starting day two. Weeds grow
  gradually rather than appearing fully grown at dawn.
- Gathering patches regrow after 24 hours (branches/herbs/reeds), 36 (berries),
  48 (stones/roots), or 168 (cut saplings). Clearing is permanent; clearing a
  depleted patch grants no second harvest. Saplings require the hatchet.
- The cumulative clock starts at hour 6. Default day length is 60 real minutes,
  configurable from 1 to 1440. Seasons cycle every 14 days.
- Time advances in steps of at most 30 game seconds, split at hour/fuel
  boundaries. Pause freezes all systems. No offline advancement occurs on load.
  Sleep is 0.25–12 hours beside a bed and restores energy while advancing all
  other systems. Zero hunger, warmth, or energy stops progression/actions until
  a checkpoint is deserialized (or an explicit new game starts).
- Version 3 ASCII persistence includes payload length and FNV-1a checksum for
  truncation/corruption detection, not tamper-proof authentication. Parsing
  validates all fields, counts, IDs, stock, ranges, relationships, and placement
  conflicts before replacing live state. Loading never partially mutates state.
  Plot records append a required crop-kind integer (0=roots, 1=berries).
  Version 2 saves are accepted and explicitly migrated with every plot typed as
  roots; all existing progress, crop growth, stock, clearing, and structures remain
  intact. The next save uses version 3. Invalid/missing kinds, unsupported versions,
  and impossible bare berry plots are rejected rather than silently defaulted.
  Unreleased version 1 used degree rotations and is rejected rather than
  silently loading walls on the wrong edges.
