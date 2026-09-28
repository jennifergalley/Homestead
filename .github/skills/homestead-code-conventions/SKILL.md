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
  tell the orchestrator before your `[ready]` if your branch changes what `Serialize` writes,
  **including appending an `Item`** (stocks are positional until `harden-save-item-stocks` lands).
  New data goes in a tagged trailing section.
- Test it natively: add cases to the matching `Tests/Homestead*Tests.cpp` and run
  `Scripts\Test-Native.ps1 -Configuration Release`.

## Unreal C++

- **Unity builds merge `.cpp` files.** File-local helpers go in an anonymous namespace with a
  file-specific prefix or inside a *named* namespace (`namespace HotbarStyle { ... }`,
  `namespace VitalsStyle`); an anonymous namespace nested in a shared named namespace
  (`HomesteadMenus`) still clashes. No `using namespace` at file scope. No file-scope names that
  shadow engine globals (`Face`, `Color`...). Lanes' editor builds hide these clashes; the game
  target shows them.
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
lane. Suggestions don't block a `[ready]`; fix them in your next increment or say why not.
