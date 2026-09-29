---
name: homestead-code-conventions
description: C++, Slate and Python coding conventions for Homestead (SurvivalGame), grounded in the code as it is. Use before writing or reviewing any C++ in Source/SurvivalGame or Source/SurvivalGameEditor, any editor Python in Content/Python/homestead_agent, or any pipeline script under Scripts/, and whenever you're unsure where new code should go.
---

# Homestead code conventions

The short list is in `.github/copilot-instructions.md` ("Code practices"). This is the long form,
with the reasons. The architecture map is `docs/architecture.md`. When the code and this page
disagree, follow the code and tell the Architecture Agent.

## Where code goes

- **Rules and state go in the simulation** (`Source/SurvivalGame/Simulation/`), never in actors.
  If a behaviour decides what the player gets, costs, can do or saves, it belongs in
  `Homestead::Simulation` (or a helper file beside it) with a native test. Actors present and
  animate; they don't decide.
- **New feature, new file.** Don't grow `HomesteadController.cpp`, `HomesteadWorld.cpp`,
  `HomesteadCharacter.cpp` or `SHomesteadMenu.cpp`. A class can span files: put a feature's
  member functions in `HomesteadController<Feature>.cpp` (see `HomesteadControllerManor.cpp`,
  `HomesteadShopFlow.cpp`) and declare them in the header's section for that feature. New
  simulation `.cpp` files must also be added to `CMakeLists.txt` (`HomesteadSimulation` library).
- **New actor for new world furniture** (store, shopkeeper, manor ruin, water ribbon) rather than
  more branches in `AHomesteadWorld`, when the thing is authored once and doesn't come from
  `State`.
- Test routes go in their own `Homestead<Route>Test.cpp` / `UI/Homestead<Route>Test.cpp`, gated
  by a command-line flag and `#if !UE_BUILD_SHIPPING` where they touch internals.

## Simulation (plain C++17)

- No Unreal types or headers: `std::string`, `std::vector`, `std::array`, `<cmath>`. The native
  build uses `/W4` with exceptions off and must stay warning-free.
- Every command returns `Result` (`ok`, a player-facing `message`, a `ResultCode`, the new
  `revision`). Validate first, mutate a copy or only after validation, and never leave state
  half-changed on failure. Messages are complete sentences Jenny can read in a toast.
- Enums that name data (`Item`, `ResourceKind`, `Recipe`, `Piece`...): **append before `Count`,
  never reorder or delete** (saves and baked placements store the integers). Parallel tables
  (`ItemCatalogue`, `ResourceName` names, `OgTable`) sit next to the enum's owner and are guarded
  with `static_assert` on the count.
- Randomness is a stable hash of ids (`OgRoll`), not hidden RNG state, so tests and saves replay.
- Save format: see `docs/architecture.md` section 5. Lanes never change `SimulationSaveVersion`;
  tell the orchestrator before your `[ready]` if your branch changes what `Serialize` writes.
  Appending an `Item` doesn't (stocks carry their width since version 13).
- **New save data goes in a tagged trailing section**, which needs no version bump (the pattern of
  `tools`, `manor`, `lamp` and `picked`, the last two beside their features in `HomesteadLamp.cpp`
  and `HomesteadCrops.cpp`):
  - **Write** it at the end of `Serialize` as `<tag> <count> ...` (its own line or lines). Leave it
    out when it holds only defaults. Keep the writer beside the feature (`Lamp::WriteSaveSection`,
    `Lamp::SaveTag` in `HomesteadLamp.cpp`) rather than inline in `HomesteadSimulation.cpp`.
  - **Read** it with one `else if (tag == ...)` in `Deserialize`'s tag loop. The loop takes the
    sections in any order. A save without yours must load with your defaults.
  - **Validate everything:** counts in range, ids that exist and suit the feature, no second copy of
    the section. On anything wrong return `invalid()`, so the current game is kept.
  - **Tags are unique lowercase words, never reused.** An unknown tag refuses the whole save with
    `ResultCode::NewerBuild` (it came from a newer build, and its sections can't be skipped safely);
    the game then leaves that save untouched. `parcels` and `economy` are the only positional ones
    (read before the loop); don't add more of those.
  - **Test** a round trip, a save without the section loading with defaults, and a malformed or
    duplicated section being refused.
  - Any new list or per-enum array you write gets its count first.
- Test it natively: add cases to the matching `Tests/Homestead*Tests.cpp` and run
  `Scripts\Test-Native.ps1 -Configuration Release`.

## Unreal C++

- **Unity builds merge `.cpp` files.** File-local helpers go in an anonymous namespace with a
  file-specific prefix or inside a *named* namespace (`namespace HotbarStyle { ... }`,
  `namespace VitalsStyle`); an anonymous namespace nested in a shared named namespace
  (`HomesteadMenus`) still clashes. No `using namespace` at file scope. No file-scope names that
  shadow engine globals (`Face`, `Color`...). Lanes' editor builds hide these clashes; the game
  target shows them.
- **Includes and compile cost:** include what each `.cpp` uses. Every `.cpp` must compile on its
  own: adaptive unity builds git-modified files outside their unity blob, so a missing include only
  shows up once someone edits that file. The module's private PCH (`SurvivalGamePCH.h`) is the
  Build Speed Agent's: add to it only headers nearly every file needs, and never `UnrealEd`. Headers forward-declare classes they only point to; a header that every
  unity blob includes (`HomesteadController.h`, `HomesteadSimulation.h`) makes each edit rebuild the
  whole module, so don't add includes to those two lightly.
- **Nothing at namespace scope may read runtime state.** Globals, file-scope statics, class static
  members and `TAutoConsoleVariable`/`FAutoConsoleCommand` arguments are constructed during static
  initialization, before the engine has set the command line, config, paths or `GEngine`. The
  packaged game is monolithic, so a read there is a fatal launch crash (`CrashDuringStaticInit`,
  exit 777006, and no log is written). Editor and PIE builds hide it, because modules load after the engine is up. Every
  packaged build crashed at launch from `a725ff1d` to `7c5fdc28` because of
  `FParse::Param(FCommandLine::Get(), ...)` in a CVar default. So: CVar defaults are literals (use
  `-1` for "follow the command line" and resolve it in a function on use, as `SkipNewGameSetup()` in
  `HomesteadControllerManor.cpp` does); `FPaths::`, `GConfig`, `FParse`, `FApp::`, `IFileManager`,
  `LoadObject` and `GEngine` belong inside functions (a function-local `static` runs on first call,
  which is fine). Plain `constexpr` values, `FLinearColor`s, `TEXT()` strings and `FName`s are safe.
- Warnings are errors, including `C4458` (a local hides a member) and `C4459` (hides a global).
- In an `_API`-exported `UCLASS`, declare one `static constexpr` per line (several declarators on one
  line give `C2487`).
- Headers that only forward-declare a `Homestead::` enum can't use it in a default argument; use an
  overload or `{}`.
- Include simulation headers from `UI/` as `../Simulation/<Header>.h` (the module root is the
  include path).
- Actors that build their parts at runtime (`AHomesteadManorRuin`, `AHomesteadDerelictFarm`: a
  transient `Parts` array plus `AddInstanceComponent`) must, in `Rebuild`, destroy every
  `UInstancedStaticMeshComponent` from `GetComponents<>()`, not only the tracked ones; PIE
  duplicates the instance components but not the transient list, so every mesh shows twice.
- **Assets:** hold loaded assets in `UPROPERTY() TObjectPtr<>` members loaded on first use. Never
  cache a `UObject*` in a function-local `static` (it's not a GC root, and it caches a failed load
  forever). Asset paths follow `/Game/SurvivalGame/Environment/Props/<Name>/SM_<Name>`.
- **Tick budget:** the controller ticks every frame and refreshes the world every 0.25 s. Don't add
  per-frame work that scales with the number of resources, placements or components; gate it on
  `Simulation::GetRevision()` or on the thing actually changing. No `TActorIterator` or
  `GetAllActorsOfClass` outside setup. No `LoadObject` per frame.
- **Threads:** only plain simulation data crosses into `Async`/`ParallelFor` lambdas; results come
  back through `TFuture`s checked on the game thread (`AHomesteadWorld::Tick`). Anim-node
  `*_AnyThread` code reads only values the game thread copied into the proxy.
- **Strings:** simulation text is UTF-8 `std::string`; convert with `UTF8_TO_TCHAR(s.c_str())` at
  the boundary. Player-facing text belongs in the simulation's `Result::message` or the UI, not in
  logs. Build text with `FString::Printf` or `FText::Format`, not long `+` chains in loops.
- **Logging:** use a named category (`DEFINE_LOG_CATEGORY_STATIC(LogHomestead<Area>, Log, All)`),
  not `LogTemp`, for new code. Errors that the player sees go through `Notify(...)`.
- **Console variables and flags:** `homestead.<Name>` CVars (`TAutoConsoleVariable`) for tunables
  you want to change live; `-Homestead<Name>` command-line flags for test routes and trials. Remove
  trial flags when the trial ends.
- **Magic numbers:** give tuning values a named `constexpr` with a comment saying what it's tuned to
  (units: cm, seconds, game hours). Numbers from an authored asset (a clip's contact frame, a
  prop's grip offset) cite the file they come from.
- **Comments** explain why or what an asset/clip assumes, not what the next line does.

## Slate UI

- Widgets take `SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)` and read it in
  `*_Lambda` attributes; actions call controller methods. Keep lambdas cheap: they run every paint.
  Snapshot structs (`HotbarSnapshot`) are fine; rebuilding whole rows per paint isn't.
- Colours come from `UI/HomesteadPalette.h` (`Brass`, `Pine`, `DeepPine`, `Cream`, `Sage`,
  `Warning`); widget-specific shades and sizes go in a named `<Widget>Style` namespace.
- HUD-anchored widgets are wrapped in `SHomesteadHudScale` so one unit is one Canvas HUD unit.
- See the `homestead-add-hud-element` skill for the full recipe.

## Python (editor and pipelines)

- Editor modules under `Content/Python/homestead_agent/` start with a docstring showing how to run
  them (`from homestead_agent import x; x.build()`), keep tunable data (frames, keys, offsets) in
  module-level constants, and reuse `rig_authoring` (animation) and `toolset` (play) rather than
  re-implementing them. PowerShell callers share `Scripts\McpHelpers.ps1`.
- Pipeline scripts under `Scripts/` take paths from arguments or the documented environment
  variables, write generated C++ only to files marked "Generated by ...; do not edit by hand",
  and keep large intermediates on `E:` (never in the repo or on `C:`).
- Generated `.inc` / `.bin` formats carry a version or magic (`HSC1`) that the C++ checks.
- Use type hints on new public functions; `print` a short summary line at the end of a bake.

## Reviews

The Architecture Agent reviews each integrated batch on `main` and sends suggestions to the owning
lane. Suggestions don't block a `[ready]`; fix them in your next increment or say why not. The
checklist it (and you) run over a diff:

1. **Static init:** no runtime reads (command line, config, paths, files, `GEngine`, asset loads) in
   globals, file-scope statics, static members or CVar/console-command arguments.
2. **Unity build:** file-local names unique or in a named namespace; no file-scope `using namespace`;
   no local that hides a member or global.
3. **Saves:** no `SimulationSaveVersion`/`bakeVersion` change; new save data in a tagged section with
   counts first; enums appended, not reordered; placement ids appended.
4. **Rules in the simulation**, with a native test; actors only present.
5. **Per-frame cost:** nothing per tick or paint that scales with placements, resources or components;
   no per-frame `LoadObject`, actor iteration or string building.
6. **Assets** in `UPROPERTY()` members, never function-local statics.
7. **Duplication:** a rule, constant or colour that already exists elsewhere (weather, palette,
   item data) is called, not re-derived.
8. **Logging** to a named category; tuning numbers named with units.
