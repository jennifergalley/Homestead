# Development setup

## Installed prerequisites

- Epic Games Launcher: `C:\Program Files\Epic Games\Launcher`.
- Visual Studio Build Tools 2022 17.14.41: `E:\Tools\VSBuildTools`.
- Windows SDK: 10.0.26100.0.
- The Microsoft installer signature was verified before launch. The first
  Build Tools attempt returned 1618 while Epic's installer was active; the
  serialized retry completed with exit code 0, without a required reboot.
- Normal builds do not need an administrator shell.
- The build/start/test scripts set Unreal's documented `UE-LocalDataCachePath`
  override to this project's `DerivedDataCache` on E:, for their own process and
  children only. They do not change persistent user/system environment settings.

## Unreal installation status

**Unreal Engine 5.8.2 is installed at `E:\Program Files\UE_5.8`.**
Epic's install metadata reports `bIsIncompleteInstall=False`; the engine has
successfully compiled the project, executed its content bootstrap, run the
initial gameplay smoke scenario, and produced a Windows package.

### Earlier launcher issue

After sign-in, the Unreal Library had no engine tiles; Install Engine opened
the library, but the add-version control did not create an engine entry.
The news page loaded normally. Jenny later confirmed installation began working;
the exact cause was not established.

The launcher was restarted after its self-update, and the News/Library view was
refreshed. Neither restored engine availability. "Hide Game Library" is already
unchecked. No account restrictions, license agreement, or credential settings have
been bypassed, and no login caches have been deleted.

Epic's official installation flow is Unreal Engine > Library > Engine Versions
> + > choose a version > Install. The earlier failure occurred before a version tile
appeared; it was not a C++ compiler error. The documented pricing/EULA confirmations
also require the user's review when presented, but no such dialog has appeared
during the observed failure. The cause of the empty engine catalog is unconfirmed.
Avoid speculative permission rewrites, firewall changes, or deleting sign-in data.

- Installation guide: https://dev.epicgames.com/documentation/en-us/unreal-engine/install-unreal-engine
- Launcher updates: https://www.epicgames.com/help/c-32735058/c-36403860/a12304262
- Service status: https://status.epicgames.com/

The installation blocker is resolved. The earlier diagnostics above are retained
as history, not current setup steps. Do not repeat launcher repairs.

The selected installation is **Unreal Engine 5.8.2** at `E:\Program Files\UE_5.8`.
The resolver checks this path, Epic's installation manifest, and common `E:` locations.
An alternate path can be supplied explicitly:

```powershell
.\Scripts\Build-Game.ps1 -EngineRoot 'E:\Program Files\UE_5.8'
```

No MetaHuman data download is required just to compile the technical foundation.
The final heroine pipeline remains a separate feasibility/visual approval step.

## Repeatable workflow

1. Run `Scripts\Test-Native.ps1` to exercise game rules and persistence.
2. Run `Scripts\Build-Game.ps1`. It imports only source assets whose expected
   sizes are recorded in `Assets\asset-manifest.json`.
3. Run `Scripts\Test-Game.ps1` for actual engine input/gameplay/save integration
   checks and rendered captures. The harness uses `Saved\Automation\SmokeSave`
   rather than normal saves; previous reports/images are archived before a run.
4. Run `Scripts\Start-Game.ps1` for standalone gameplay through the engine.
5. Run `Scripts\Build-Game.ps1 -Package` for a standalone Windows distribution.
6. Review the packaged build, not editor FPS, against the 60 FPS target and
   controller-only playthrough in `game-plan.md`.

## Verification status

**Current packaged MVP:** `Build\Windows\SurvivalGame.exe`.
The final standalone native-4K run passed **432 mapped-input steps**, including
all 18 appearance combinations, both crops, cooking/storage/construction, the
enclosed doorway, overnight survival, exact persistence, and genuine failure
followed by same-world checkpoint recovery.
At **3840x2160 with 100% 3D screen percentage**, the bounded route averaged
**59.16 FPS**, with **16.90 ms p95 / 17.03 ms p99** frame times, excluding startup
and screenshot-readback windows. The game-only stereo audio capture was
**233.984 seconds at 48 kHz**, RMS **0.02690**, peak **0.35782**, with no
near-clipped samples. These measurements do not certify every possible scene,
physical-controller comfort, or listening quality.

- Portable simulation: MSVC Debug and Release builds passed. The final suite has
  16 scenarios and 986 explicit checks, including the default opening loop,
  ordinal wall placement, multi-cell enclosure, farming, atomic transactions,
  independent fires, failure recovery, both crop cycles, and strict version-3
  persistence with explicit version-2 root-plot migration.
- `Scripts\Test-Native.ps1` was rerun successfully through configuration,
  compilation, CTest, and the detailed test executable.
- All 18 downloaded/extracted environment/audio source files match recorded hashes.
  PowerShell scripts parse and the editor Python bootstrap passes syntax checking.
- Unreal editor-module build and Python material/map/audio bootstrap now pass.
  Targets use UE 5.8's V7 build settings and 5.8 include order. Compilation is
  local-only; no remote build service is used.
- The actual engine smoke scenario passes: synthetic gamepad movement/look,
  controller/keyboard field-book navigation, menu pausing, foraging/raw eating,
  checksummed save/load, crafting a hatchet, clearing a sapling, and placing a
  foundation. Fresh clearing, field-book, and foundation screenshots are produced.
  Test saves are isolated from normal game saves.
- Windows packaging succeeded at `Build\Windows\SurvivalGame.exe`. The package
  itself passed the final complete route through the standalone executable,
  without launching Unreal Editor.
- Editor `-FullLoop` passes the expanded mapped-input route covering crafting, an enclosed
  room and actual doorway traversal, cooking/eating, chest transfers, gardening,
  rain, six overnight sleeps, harvest/replant and exact saved-world restoration.
  Genuine outdoor failure and gamepad retry restore the same protected cabin,
  garden, stored resources, clearing and appearance. Both crop types persist.
- Actual 1920x1080 captures are now asserted; earlier offscreen runs had silently
  resized to 888x500. High-DPI awareness and `-ForceRes` corrected this.
- An editor route averaged 59.75 FPS, p95 16.90 ms/p99 17.08 ms, excluding startup
  and screenshot-readback windows. The extended route measured 59.83 FPS,
  p95 16.91 ms. These are bounded automated-route measurements, not all-scene/4K
  performance certification.
- Game-only master-submix capture produced 36.416 seconds of stereo 48 kHz audio,
  RMS 0.02084, peak 0.25064, with no near-clipped samples. No microphone or other
  application audio was captured. Silence/clipping checks do not establish sound
  quality or replace listening.
- Physical-controller feel and audio listening remain unverified. Jenny liked the running-scene
  screenshot; this is positive visual-direction feedback, not full MVP acceptance.
- Three clothed adult face/body presets with three hair choices and two cosmetic
  outfit choices share a 53-bone authoring rig
  (54 Unreal bones including its wrapper), and
  idle/walk clips was authored offline and successfully imported into Unreal.
  Runtime animation, color controls, saved appearance, and the Look sidebar
  passed editor and packaged smoke checks. Camera framing and UI corrections
  subsequently passed the 1080p editor route. All eighteen mesh combinations
  resolve correctly; fine-grained sculpt sliders and clothing physics are deferred.
- Material-slot import persistence is checked in a fresh engine process before
  accepting the character import receipt. Modified Python skeletal-material
  structs must be assigned back to their array elements; merely mutating an
  iteration wrapper can leave null references after reopening.
- The bounded visual review resolved framing, readable selection and hint
  backing, and the missing character evidence. Highlight rendition on sunlit
  faces remains a documented prototype limitation; no further polish loop was
  used to disguise it. Small apron overlap can occur in some poses.
- While installation was pending, comparison with the downloaded 5.8 headers
  exposed and corrected a controller override: use `FInputKeyEventArgs`, not the
  legacy final `FInputKeyParams` overload. An engine smoke harness now exercises
  synthetic gamepad/keyboard input, menu pause, foraging/eating, saving/loading,
  crafting, and construction. Its first placement attempt correctly refused an
  uncleared sapling; the scenario now clears the site and passes.

The bootstrap does not replace existing maps or material graphs. It reapplies
the documented source-texture compression/color-space settings, rock-material
assignments, and ambience looping flag. If those import conventions need to
change, update the script deliberately. Never delete the whole Content folder
to regenerate a map or revise an imported material.

## Persistence

The Unreal integration stores checksummed save records beneath `Saved\SaveGames`.
It writes and verifies a temporary record before replacement, retains the prior
file as `.bak`, and surfaces failed writes. Three automatic slots, a manual slot,
and a recovery checkpoint are kept separately. New clearings have distinct world
identifiers so a failed new game cannot silently jump to an unrelated old world.

Unreal-side save/restore and actual failure/retry pass editor and standalone
extended routes. Appearance save records are version 4 and accept earlier
versions 1-3 with default values for newly introduced choices. Simulation records
are version 3; version 2 plots migrate explicitly to roots. Version 1 degree-based
prototype simulation records remain rejected rather than misplacing structures.
