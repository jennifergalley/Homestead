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
