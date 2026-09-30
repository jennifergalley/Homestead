# Homestead architecture

How the game's code fits together, where each feature lives, and where the known debt is. Owned by
the Architecture Agent; coding rules are in `.github/copilot-instructions.md` ("Code practices") and
the `homestead-*` skills under `.github/skills/`. Line numbers drift; search for the symbol.

Surveyed on `main` at `8762ba46` (2026-09-28); four-file split updated at `b3576693`
(2026-09-29).

## 1. Modules at a glance

| Area | Where | What it is |
| --- | --- | --- |
| Simulation core | `Source/SurvivalGame/Simulation/` | Plain C++17, no Unreal headers. All game rules and state: time, vitals, inventory, resources, building, crops, estate placements, overgrowth, parcels, shops, manor, save format. Built twice: into the game module by UBT and into `HomesteadSimulation.lib` by CMake for the native tests. |
| Game module | `Source/SurvivalGame/` | Unreal presentation and input around the simulation. `AHomesteadController` (owns the simulation, input, menus, saves, audio), `AHomesteadWorld` (turns state into meshes), `AHomesteadCharacter` (heroine, MetaHuman stack, held tools), `UHomesteadAnimInstance` (code-built anim graph). Each of the first three classes now spans feature-specific `.cpp` files. |
| Slate UI | `Source/SurvivalGame/UI/` | Field book (`SHomesteadMenu`), shop, hotbar, vitals, minimap/map, icons, new-game names, arrival title. |
| Canvas HUD | `HomesteadHUD.cpp` | Calendar panel, first-minute controls strip, parchment toasts and focus cues, drawn every frame with `UCanvas`. The time, vitals, hotbar, minimap and compass are Slate widgets in the same HUD units. |
| In-game test routes | `Homestead*Test.cpp`, `Homestead*Playtest.cpp`, `UI/Homestead*Test.cpp` | Scripted input routes run by `AHomesteadSmokeTest` / `AHomesteadVisualPlaytest` in `-game` processes. Compiled into Development builds. |
| Editor module | `Source/SurvivalGameEditor/` | Python-callable libraries (`HomesteadAgentPlayLibrary`, `HomesteadEstateAuthoringLibrary`) and the estate map import. |
| Editor Python | `Content/Python/homestead_agent/` | MCP toolset for play (`toolset.py`), animation authoring on the MetaHuman Control Rig (`rig_authoring.py` plus one module per clip), MetaHuman look/hair, store and town set-up. |
| Pipelines | `Scripts/Terrain`, `Scripts/Map`, `Scripts/Blender`, `Scripts/Characters` | Offline Python: LIDAR → heightmap and layout, scenery/placement scatter, ground and ocean bakes, the minimap bake, Blender prop recipes and import. |
| Build/test scripts | `Scripts/*.ps1` | `Start-EditorMcp`, `Build-Game`, `Test-Native`, `Test-Game`, MCP helpers, perf lock. |
| Native tests | `Tests/*.cpp`, `CMakeLists.txt` | CTest suites against the simulation library (eight on 2026-09-29; count them with `ctest -N`) (`Scripts\Test-Native.ps1`). `Tests/` also holds older policy/evidence scripts that nothing runs routinely. |
| Plans | `openspec/changes/` | One change per feature, bug or refactor. |

The formerly oversized presentation files are now small lifecycle entry points; members remain on
the same classes, with no save or gameplay rule moved:

| Class | Feature files to edit |
| --- | --- |
| `AHomesteadController` | `HomesteadController{Hotbar,Focus,Clearing,Interaction,Farming,Placement,Book,Settings,Saves,Audio,World,Water,Lamp,Input,MenuBridge,Playtest}.cpp`; existing `HomesteadControllerManor.cpp`, `HomesteadControllerSeedPouch.cpp`, `HomesteadShopFlow.cpp` and other focused files remain separate. The private `HomesteadController{Config,Helpers,Preferences,Text}.h` headers share the former file-local helpers. |
| `AHomesteadWorld` | `HomesteadWorld{Woodland,Estate,Terrain,Regional,Trees,Resources,Structures,Garden,Lighting,Refresh,Preview,Geometry}.cpp`; `HomesteadWorld{Common,Keys,Log,Look,VisualHelpers}.h` share only presentation data and helpers. |
| `AHomesteadCharacter` | `HomesteadCharacter{Appearance,Gather,Equipment,Actions,Movement}.cpp`; hair and groom setup live in `Appearance.cpp`. |
| `SHomesteadMenu` | `UI/SHomesteadMenu{Pages,Details,Inventory,Map,Dialog,Focus,Input,Actions}.cpp`; `SHomesteadMenuPrivate.h` holds the existing internal Slate styles and widgets. |

## 2. Runtime flow

```mermaid
flowchart LR
    Input[Enhanced Input / InputKey] --> PC[AHomesteadController]
    PC -->|commands: Harvest, ClearOvergrowth, Craft, Place, Buy...| Sim[Homestead::Simulation]
    PC -->|Advance(dt) every tick| Sim
    Sim -->|Result ok/message/revision| PC
    PC -->|Refresh(Sim) every 0.25 s| World[AHomesteadWorld]
    World -->|reads State| Sim
    PC --> Char[AHomesteadCharacter]
    Char --> Anim[UHomesteadAnimInstance proxy]
    PC -->|owns widgets; lambdas poll controller| Slate[Slate widgets]
    HUD[AHomesteadHUD DrawHUD] -->|polls every frame| PC
```

- **Ownership.** `AHomesteadController` holds `Homestead::Simulation Sim` by value. Nothing else
  mutates it. Everything else reads it through `Controller->State()` / `Simulation()`.
- **Startup** (`AHomesteadController::BeginPlay`): resolve the save route (and the test-sandbox
  admission checks), pick the mode by map name (`GetCurrentLevelName == "Estate"`), activate the
  estate heightfield, `Sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements())`
  or `Sim.NewGame(seed)`, spawn `AHomesteadWorld`, `LoadLatest()`, then show the book or the new-game
  setup and the hotbar.
- **Tick** (`AHomesteadController::Tick`, ~250 lines): arrival/spawn waits, fall recovery,
  `Sim.Advance(dt, player, paused)`, stores, placement preview, footsteps, pending swings/fells,
  held-produce bookkeeping, then every 0.25 s `Landscape->Refresh(Sim)` and `UpdateFocus()`, then
  woodland chunk-edge preparation and autosave.
- **Actions.** Input → controller verb (`Interact`, `Secondary`, `UseSelectedTool`,
  `SwingAtOvergrowth`...) → simulation command and character presentation. Timed tool swings
  commit at the clip's contact beat (`Sim.ClearOvergrowth(...)`); some hand gathers commit before
  the kneeling presentation. A
  `Homestead::Result` comes back with `ok`, a player-facing `message` (shown as a toast) and the new
  `revision`. Commands that change state bump `Simulation::GetRevision()`.
- **World presentation** (`AHomesteadWorld::Refresh` in `HomesteadWorldRefresh.cpp`). No events:
  a revision/visual-input integer key skips unchanged refreshes; per-object signatures
  (`FHomesteadWorldVisual::Signature`) rebuild only visuals whose inputs changed. Resources,
  structures, plots and drops are individual `UStaticMeshComponent`s;
  scenery, ground cover and trees are HISM batches. Estate scenery comes from
  `Content/SurvivalGame/Estate/Runtime/EstateScenery.bin` (`HSC1`, `BuildEstateScenery`) and is
  hidden under built pieces and live resources.
- **UI.** Pull-based. Widgets hold a `TWeakObjectPtr<AHomesteadController>` and read it in
  `*_Lambda` attributes; actions call `Menu*` methods on the controller. The controller creates the
  viewport widgets (`ShowHotbar`, `ShowNativeMenu`, shop, names). There are no view-models or
  delegates.

## 3. Two game modes in one codebase

| | Estate (current direction) | Generated woodland (older survival mode, still supported) |
| --- | --- | --- |
| Map | `/Game/SurvivalGame/Maps/Estate` (default) | `/Game/SurvivalGame/Maps/Homestead` |
| Selected by | `bEstateMap` = level name is `Estate`; the sim sets `State::fixedEstate` | anything else |
| Ground | Landscape + `HomesteadEstateTerrain` (`EstateHeightfield.r16`) | `AHomesteadWorld::BuildTerrainChunk` procedural mesh chunks |
| Resources | Fixed placements: `ProvisionalEstatePlacements()` from `HomesteadEstate.cpp` and the generated `Simulation/*Placements.inc` | Seeded chunks (`HomesteadWorldGeneration`, `HomesteadRegionalGeneration`), streamed around `State::activeChunk` |
| Saves | `<SaveGames>\Estate\` | `<SaveGames>\` |

Woodland-only code: chunk generation and regional descriptors (all of `Simulation/HomesteadWorldGeneration*`,
`HomesteadRegional*`), the terrain, regional, woodland and tree-batch methods in
`HomesteadWorld{Terrain,Regional,Woodland,Trees}.cpp`, the controller's chunk-edge preparation,
and most in-game test
routes (they still load the woodland map and its knife/reeds content). The MVP survival line
(`mvp-survival`) and the MVP woodland biome (placement ids 560000+) depend on it. **Ask the
orchestrator before removing any of it.**

## 4. Simulation core

- `HomesteadSimulation.h`: the enums (`ResourceKind`, `Recipe`, `ToolKind`, `Piece`...), constants
  (`SimulationSaveVersion`, id bases, capacities), `State` and `Simulation`. Every public command
  returns `Result`; queries are `const`.
- `HomesteadItems.{h,cpp}`: the item catalogue. One enum value plus one `ItemInfo` row, in order.
- `HomesteadOvergrowth.cpp`: `OgTable`, one row per clearable kind (tool, tier, swings, yields).
- `HomesteadEstate.{h,cpp}`: the provisional layout (mirrors `Scripts/Terrain/estate_layout.json`)
  and placements. Placement id ranges per lane are listed in `HomesteadEstate.h` and
  `docs/handoff/round-<n>.md`.
- `HomesteadParcels`, `HomesteadShops`, `HomesteadManor`: estate-only rules with their own save
  sections.
- `HomesteadSimulationDetail.h`: helpers shared between the simulation `.cpp` files.
- Randomness is deterministic (hashes of ids, e.g. `OgRoll`), so tests and saves are reproducible.
- Engine-free on purpose: no `FString`, `TArray` or `UE_LOG` here, so the native tests build in
  seconds with `/W4` and no warnings.

## 5. Saves

- **Container:** `UHomesteadSave` (`HomesteadSave.h`, a `USaveGame`, its own `CurrentVersion`)
  carries the world id, the player transform and camera, appearance, hotbar and the simulation
  payload as a string (`SimulationData`).
- **Payload:** `Simulation::Serialize()` writes ASCII text: a header line
  `HOMESTEAD <SimulationSaveVersion> <size> <checksum>`, then fixed sections in order (vitals, pack
  stock, world id, resource edits, buildings, structures with chest stocks, plots, drops, wearables,
  equipment, layout, cleared underbrush), then **tagged trailing sections**: `parcels` and
  `economy` (read in that order), then any of `tools`, `manor`, `lamp`, `picked` in any order. A
  missing trailing section loads with defaults. A save from a newer build (a later version, wider item
  stocks or an unknown tag) is refused with `ResultCode::NewerBuild` and left untouched on disk; only
  unreadable older saves (`UnsupportedVersion`) are moved to `Retired`. New
  save data is added this way without a version bump (recipe in the conventions skill). Bytes above
  127 are rejected (hex-encode free text such as names).
- **Versioning:** `SimulationSaveVersion` (13) in `HomesteadSimulation.h`. Lanes never bump it; the
  orchestrator does, once per integration. Version 12 migrates; 11 is refused; 7-10 still migrate.
- **Item stocks carry their width** (version 13, `harden-save-item-stocks`): each stock (the pack and
  every chest) and the equipment slots are written as `<count> v0 v1 ...`. A save from a build with
  fewer items loads with the new ones at zero, so **appending an `Item` needs no version bump**; a
  wider stock means a newer build and is refused. Version 12 stocks were positional (always 40 items
  wide on `main`); the reader measures each stock's line. Reordering or removing items still breaks
  saves: enums that name data stay append-only.
- **Estate saves** also check `State::placementBakeVersion` against the placements baked into the
  build; a save from a different layout is refused with a "start a new game" message.
- **Routing:** `HomesteadSaveRouting` picks the save directory (normal, test sandbox, fixtures).
  Manual slot `Homestead_Manual`, rotating autosaves `Homestead_Auto_0..2`, plus recovery
  checkpoints. Autosave settings live in the user settings file, not the save.
- Saves are disposable test data until Jenny says otherwise (`openspec/config.yaml`).

## 6. Threading

Almost everything runs on the game thread. The exceptions, all in `HomesteadWorld.cpp` and all
woodland-only:

- chunk baselines: `Async(EAsyncExecution::ThreadPool, ...)` in `QueueChunkBaselineBuild`;
- regional descriptors: `QueueRegionalDescriptorBuild`;
- missing chunk resources: `ParallelFor` inside a thread-pool task.

Results come back as `TFuture`s polled in `AHomesteadWorld::Tick`, checked against the current seed
and generation version before use. Worker lambdas take copies of plain simulation data only; they
must never touch `UObject`s. The anim graph's `*_AnyThread` node code runs on worker threads and
reads only values the game thread copied into the proxy.

## 7. Where each feature lives

| Feature | Simulation | Presentation / input |
| --- | --- | --- |
| Vitals, time, weather, sleep | `HomesteadSimulation.cpp` (`Advance`, `Step`, `Sleep`) | `SHomesteadVitals`, HUD calendar |
| Items, pack, chests | `HomesteadItems`, `HomesteadSimulation.cpp` (transfers, layout) | `UI/HomesteadMenuInventory.cpp`, `SHomesteadMenu`, `SHomesteadIcon` glyphs |
| Crafting and hafting | `CraftChange`, `Recipe` | `MenuCraftRecipe`/`MenuCraftBeat`, craft animation layer |
| Estate clearing | `HomesteadOvergrowth` | `HomesteadControllerClearing.cpp` (`SwingAtOvergrowth`, `LandOvergrowthSwing`), `HomesteadWorldResources.cpp` (`BuildOvergrowth`) |
| Foraging, berries, spring flowers | `Regrowth`/`Yield` in `HomesteadSimulation.cpp` | `HomesteadWorldResources.cpp` (`BuildResource`) |
| Trees and felling | resources `ForestTree` | `HomesteadControllerClearing.cpp` (`PresentFelling`), `HomesteadWorldTrees.cpp` (`BeginFelling`, `UpdateFallingTree`) |
| Building and furniture | `Place`, `Deconstruct`, `BuildCost` | `HomesteadControllerPlacement.cpp`, `HomesteadWorldStructures.cpp` (`BuildStructure`) |
| Garden plots | `Till`, `Plant`, `Water`, `Weed` | `HomesteadControllerFarming.cpp` (`TillSquareAhead`), `HomesteadWorldGarden.cpp` (`BuildPlot`) |
| Tools and hotbar | `ToolKind`, `toolTiers` | `HomesteadControllerHotbar.cpp` (`HotbarSnapshot`), `SHomesteadHotbar`, held-tool attachment in `HomesteadCharacterEquipment.cpp` |
| Money and shop | `HomesteadShops` | `SHomesteadShop`, `AHomesteadGeneralStore`, `AHomesteadShopkeeper`, `HomesteadShopFlow.cpp` |
| Manor, names, journal | `HomesteadManor` | `HomesteadControllerManor.cpp`, `AHomesteadManorRuin`, `SHomesteadNames`, `SHomesteadArrival` |
| Parcels, map, minimap | `HomesteadParcels` | `UHomesteadMapComponent`, `UI/HomesteadMap*`, `SHomesteadMinimap`, `SHomesteadMapView` |
| Water, creek, pail | water probe (`SetWaterProbe`) | `HomesteadControllerWater.cpp` (`WaterEdgeDistance`), `AHomesteadWaterRibbon`, `HomesteadControllerAudio.cpp` (creek audio) |
| Heroine look and wardrobe | wearables | `HomesteadCharacterAppearance.cpp` (`LoadMetaHumanStack`), `HomesteadAppearance`, `HomesteadWardrobe*` |
| Animation | none | `UHomesteadAnimInstance`, clips authored by `homestead_agent/*.py` |
| Audio | none | `HomesteadControllerAudio.cpp` (`InitializeAudio`, playlist), footsteps via `UHomesteadFootstepNotify` |
| Character lab | none | `HomesteadLab.{h,cpp}` (`-HomesteadCharacterLab`) |

## 8. Offline pipelines and baked data

| Output (in the repo) | Written by | Read by |
| --- | --- | --- |
| `Estate_Heightmap_4033.png`, `estate_layout.json` | `Scripts/Terrain/reshape.py` | Landscape import (editor); layout mirrored by hand into `ProvisionalEstateLayout()`, which `AHomesteadController::PrepareEstateSimulation` currently installs directly. Moving an anchor requires both updates; there is no live `DA_EstateLandmarks` replacement path. |
| `Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16` | terrain pipeline | `HomesteadEstateTerrain::Activate` (ground height outside the Landscape) |
| `Simulation/HomesteadEstatePublicRoad.inc` | `Scripts/Terrain/public_road.py` from `estate_layout.json` road/roadProfile and `EstateHeightfield.r16` | Simulation public-road centreline/chainage, safe travel endpoints, sign anchors and bridge keep-out. Re-run it whenever route, profile or terrain heights move, then verify stops, signs and terrain heights. |
| `Content/SurvivalGame/Estate/Runtime/EstateScenery.bin` (`HSC1`) | `Scripts/Terrain/scatter.py` | `AHomesteadWorld::BuildEstateScenery`; kind bytes must match `EstateSceneryKinds` |
| `Simulation/HomesteadEstateWorldPlacements.inc` and the other `*Placements.inc` | `scatter.py`, `berries.py`, `clearout.py`, `estate_disrepair.py` ("do not edit by hand") | `ProvisionalEstatePlacements()` |
| `EstateGround.bin` (ground lane) | `Scripts/Terrain/bake_ground.py` | `HomesteadEstateGround` / `HomesteadGrassField` |
| Ocean shore textures | `bake_ocean.py`, `build_ocean.py` | ocean material |
| `T_EstateMap` | `Homestead.BakeEstateMap` / `Scripts/Map` | minimap and Map tab |
| Props under `Content/SurvivalGame/Environment/Props/<Name>/SM_<Name>` | `Scripts/Blender/Recipes/*.py` → `import_props.py` | `AHomesteadWorld` (`LoadObject` by path) |

Re-bake order after `scatter.py` changes: `bake_ground.py`, `build_ground.py`, then the estate map
(`docs/setup.md`). Placement ids and scenery kind bytes are registered on the round page.

## 9. Tests

- **Native (authoritative, fast):** `Scripts\Test-Native.ps1 -Configuration Release`, every suite (9 at the four-file split), about
  3 minutes. Add a suite with `add_executable` + `add_test` in `CMakeLists.txt`. New simulation `.cpp`
  files must also be added to the `HomesteadSimulation` library there.
- **In-game routes:** `Scripts\Test-Game.ps1` launches `-game -HomesteadSmokeTest` with a route flag
  (`-HomesteadFullLoop`, `-HomesteadNativeMenuTest`...). A route is a list of `FStep`s (input,
  predicate, timeout) in `AHomesteadSmokeTest`. Most routes still target the woodland map; several
  fail on retired content (round page, "Open blockers").
- **Older scripts:** a few `Tests/*.ps1|*.py` policy checks remain from an earlier, heavier process
  (for example `Test-AuthoringProbePolicy.ps1` for the packaged QA guard). Nothing runs them
  routinely. That process's receipts stay in `docs/research/`.

## 10. Size, hotspots and debt (surveyed 2026-09-28; split 2026-09-29)

Immediately before the split, `HomesteadController.cpp` was 4778 lines, `HomesteadWorld.cpp`
4962, `HomesteadCharacter.cpp` 2833 and `UI/SHomesteadMenu.cpp` 3206; their lifecycle files
now contain 579, 188, 173 and 270 lines respectively. All 185 controller, 71 world, 92 character
and 96 menu member-function bodies were preserved. Each family passed native Release 9/9 and
the Editor and Game unity targets separately before being integrated on `main` through `b3576693`.
The classes and their broad headers are still large; moving the methods reduced hot-file churn
and translation-unit size, not their public surface. `HomesteadController.h` declared about
440 members at the original survey.

Longest functions: test route builders (`PrepareGeneratedWorldChecks` 1049 lines, `PrepareFullLoop`
919, `PrepareHotbarChecks` 867), `AHomesteadWorld::BuildDecorations` 474,
`AHomesteadCharacter::LoadMetaHumanStack` 360, `AHomesteadWorld::Refresh` 280,
`AHomesteadController::Tick` 255. 43 functions exceed 150 lines.

Known debt, highest payoff first (the plan is in `openspec/changes/improve-code-health-between-rounds`):

1. ~~Positional item stocks in saves~~: fixed by `harden-save-item-stocks` (version 13).
2. ~~Oversized hot translation units~~: split by feature through `b3576693`; the broad class
   headers and cross-feature interfaces remain candidates for the separate header-fan-out step.
3. ~~Unconditional world refresh work~~: the Performance Agent's revision/visual-input key and
   integer signatures reduced measured game-thread p95 from about 15 ms to 8 ms.
4. **Colour and font constants are copied**, not shared: the brass gold `(0.92, 0.74, 0.43)` is
   defined 14 times under different names (`MenuGold`, `ShopGold`, `NameGold`, `MvGold`...) to dodge
   unity-build clashes. `UI/HomesteadPalette.h` is the shared home.
5. **Test harness in product code.** Shipping-QA admission checks and route dispatch live in the
   controller's `BeginPlay`; route builders are 500-1000-line lambdas.
6. **Retired woodland content in test routes** (knife, reeds, woodland map).
7. **Legacy item names for new tools** (`Hafting` an axe yields `Item::Hatchet`, a hoe yields
   `Item::DiggingStick`). Harmless for saves (positional), confusing for readers.
8. **Mixed HUD technologies:** Canvas (`AHomesteadHUD`) and Slate (`SHomesteadHudScale` rescales
   Slate into Canvas units so they line up).
9. ~~Editor probe code~~: `FernSpike.cpp`, the authoring-probe commandlet and the policy scripts
   that only served them were removed (2026-09-28).
