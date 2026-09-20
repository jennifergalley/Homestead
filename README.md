# Homestead

A Windows-native, offline, controller-first survival and homesteading game.
The design and roadmap are in `docs\game-plan.md`.

Bounded autonomous development, status updates, pause/stop controls and protected
candidate builds are described in `docs\autonomous-development.md`.

## Current state

This is a **playable homestead prototype**, not the finished game. It has three
complete adult character presets, long waves/bob/ponytail, two cosmetic outfits,
skin/hair/iris/tunic colors, and a saved, orbitable Look preview. It includes
foraging, tool crafting, modular shelter, storage, roots and regrowing berry crops,
watering/weeding, basic cooking, day/night needs, sleep, and checkpoint recovery.

Unreal 5.8.2 is installed at `E:\Program Files\UE_5.8`. The native editor module,
content bootstrap, and initial engine gameplay smoke scenario now run successfully.
A standalone Windows technical build has also been produced at
`Build\Windows\SurvivalGame.exe`; keep the entire `Build\Windows` directory together.
That preserved original package passed the complete homestead and recovery route at
native 3840x2160, averaging about 59 FPS on the target PC.
Use its build receipt when comparing newer source changes. Controls are exercised
through real engine input events; physical-controller feel and listening review
remain human checks. See `docs\setup.md` for precise verification limits.

## Build and run

To play the preserved original prototype, double-click **`Play.cmd`** in this folder
or `Build\Windows\SurvivalGame.exe`. The packaged game does not need the editor
open. In-game Settings includes **Save and quit**.

To review the separately verified movement and action improvements, double-click
**`Preview.cmd`** (requires PowerShell 7). It selects only the explicit candidate
in `Preview.json`, checks its acceptance receipt/executable hash, and uses the
persistent **`jenny-review`** save profile. It does not replace `Play.cmd`, import
your original world, run automated inputs, or quit automatically.

Preview saves live in
`%LOCALAPPDATA%\SurvivalGame\PreviewProfiles\profile-jenny-review\SaveGames`.
Manual saves, all three autosaves, recovery and backups stay there. Relaunch
`Preview.cmd` to continue; use `Play.cmd` to return to the untouched original.
**Do not launch the candidate executable directly:** the preview-profile argument,
not its package folder, selects this isolated save namespace.

The review candidate retains the accepted relaxed locomotion and generic
gathering, watering, weeding and sapling-hatchet feedback. These are technical
improvements, not Jenny's aesthetic approval or precise hand/tool-contact IK.
Rejected facial-shader and shortened-wave trials are not included. The requested
mid-back wavy length and Melinoe-inspired blonde bob remain unmet/deferred.
See `docs\setup.md` for profile selection, verification and known limits.

Controller prompt stability and limited book clarity are included in the
parent-reviewed **`book-clarity-01`**, now explicitly selected by `Preview.json`.
That candidate distinguishes
**Your pack**, **Crafting recipes** and **Building plans**, labels carried/chest
counts separately from required materials, and names actual eat/take/craft/plan
actions. It adds no recipe learning, unlocks, inventory expansion or new navigation.

The movement tearing/flicker report is **investigated, unresolved**, not fixed.
Read-only diagnostics found a reported 4K/30Hz desktop mode and a VSync-off,
60fps-capped diagnostic runtime; neither alone proves tearing. Offscreen game frames do not
observe physical scanout. See `docs\presentation-diagnostics.md` for exact runtime
settings, capture limits and the next human check. No graphics defaults changed.

The separate `video-sync-01` candidate adds **Settings > Vertical sync**, a
reversible On/Off toggle for later human comparison. Default remains Off;
tearing is still unresolved. It is **not yet selected by Preview.cmd**.
Graphics preferences are shared within that game's Unreal config, not isolated
per preview-save profile. See `docs\setup.md` for controls and override behavior.

From this directory in PowerShell:

```powershell
.\Scripts\Test-Native.ps1
.\Scripts\Build-Game.ps1
.\Scripts\Test-Game.ps1
.\Scripts\Start-Game.ps1
```

The first command tests the portable simulation without Unreal. `Test-Game.ps1`
runs the actual game's synthetic controller/keyboard smoke checks and captures
rendered frames, using a separate save sandbox. It does not replace visual,
audio, or physical-controller review. The other commands
require **Unreal Engine 5.8**. If needed, pass `-EngineRoot 'E:\Tools\UE_5.8'` to
the game build/start scripts. To create a standalone Windows build:

```powershell
.\Scripts\Build-Game.ps1 -Package
.\Scripts\Test-Game.ps1 -Packaged -FullLoop -WithAudio
```

`-WithAudio` records only the game's own output submix for silence/clipping
analysis, never the microphone or other applications. This is not a substitute
for listening. `-FullLoop` adds the extended homestead scenario.

`Build-Game.ps1` fetches the licensed source assets, compiles the native editor
module, generates the materials/map/audio assets in the editor, and optionally
cooks/packages the game. Failed steps stop with an explicit error.

## Controls

| Action | Xbox controller | Mouse and keyboard |
| --- | --- | --- |
| Walk / camera | Left / right stick | WASD / mouse |
| Contextual interaction | A | E or Enter |
| Clear / weed / fuel / till | X | F |
| Field book | Menu | I or Tab |
| Notes | View | H |
| Craft / build pages | D-pad left / right | C / B |
| Book selection | D-pad up / down | Up / down |
| Book pages | LB / RB | Left / right |
| Back / pause menu | B | Escape |
| Camera distance | Right stick click | Mouse wheel |
| Placement position | Left stick | WASD |
| Rotate placement | RB or X | R or F |
| Save / load | Field book Settings | Settings or F5 / F9 |

The field book and construction preview pause simulation. Near a storage chest,
the Pack page supports storing with X/F and taking stored materials with Y/G.
Food selections consume one item; core actions explain unmet requirements.
Settings also offers 100/85/70-percent 3D resolution scaling; the UI remains sharp
while TSR upscales the world.

## First session

Gather berries and eat them from the Pack page. Gather branches, stones, and
fiber from reeds. Make a hatchet, a digging stick, and a watering can. Clear a
small patch, build a floor/walls/doorway/roof, and add a bedroll, chest, and fire.
Collect planting stock from wild roots, till a plot, and plant it. Refill the can
at the stream; water and weed as needed. Fuel the fire with branches and prepare
the simple root recipes. Food and warmth matter while time passes.

On a bare plot, **A/E plants roots using seeds**; **X/F plants berry seeds using
one foraged berry**. Once planted, X/F weeds. Mature roots produce roots and seeds,
then leave the plot available for replanting; mature berry plants give six berries
and remain in place to regrow. The Look page changes appearance without making a
cosmetic apron into free winter insulation.

## Prototype limits

- Character hair/cloth motion and lighting are provisional; small garment overlap
  can occur in some poses. Fine-grained face/body sliders are a later feature.
- Construction is a small snapping kit, not voxel terrain editing or a full
  furnishing catalog. The landscape is a compact authored clearing, not the
  eventual river/lake/waterfall world.
- Villagers, romance, companions, carcass scavenging, hunting/predators, and a
  complete winter economy are roadmap features, not hidden unfinished buttons.
- Save before experimenting. Test saves/captures are isolated from normal play.

## Architecture

- `Source\SurvivalGame\Simulation`: portable C++ state and rules, independent of
  Unreal, rendering, wall-clock time, and input.
- `Source\SurvivalGame`: Unreal world, character/camera, input, field book,
  audio playback, and recoverable save integration.
- `Tests`: native simulation tests.
- `Scripts`: asset acquisition, editor content generation, building, and launch.
- `Assets`: source/license manifest and downloaded asset receipts.

Normal-movement visual review is separate from functional smoke tests:
`Scripts\Playtest-Visual.ps1` records an isolated game session without teleport
travel, and `Scripts\Review-VisualPlaytest.py` creates a timestamped local viewer.
See `docs\visual-playtesting.md` for evidence, sampling limits and findings.

Prototype meshes are original; selected landscape/audio assets are licensed
separately. See `docs\asset-credits.md`. No game assets from the inspiration titles
are used, and the supplied portrait must remain a local reference.
