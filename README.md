# Homestead

A Windows-native, offline cozy life sim: restore a derelict family estate on the
early-Victorian Cornish coast, then farm, ranch, fish and mine your way to a fortune.
The direction and round order are in `docs\game-plan.md`; the survival-era prototype plan
is archived in `docs\archive\`.

Bounded autonomous development, status updates, pause/stop controls and protected
candidate builds are described in `docs\autonomous-development.md`.

Original static props are modeled with Blender, live in a visible window or headless;
see `docs\blender-assets.md`.

## Current state

This is a **playable prototype** of the cozy-estate direction, not the finished game. It's in
round 1, "Walk your estate": the round's lanes are integrated on `main` and being polished. Status,
lanes and known issues are in `docs\handoff\round-1.md`; the design and round order are in
`openspec\changes\pivot-to-cozy-estate-life-sim\design.md`.

- **The Estate is the default map** (`/Game/SurvivalGame/Maps/Estate`): a fixed 4 km World
  Partition landscape built from real Environment Agency LIDAR of the St Agnes coast
  (`Scripts\Terrain\README.md`), with sea, river and scenery.
- **What you can do there:** arrive at the ruined manor's standing room (bed, hearth, chest) and
  name your family and estate; clear overgrowth with tiered tools you salvage and haft (billhook,
  axe, scythe, pickaxe, hoe; the pail comes too); see your estate boundary on the minimap and the
  field book's Map tab; walk to town and buy and sell at the general store with money in dollars and
  cents.
- **Heroine:** a MetaHuman with authored work animations (gathering, felling, tilling, watering,
  eating) and appearance options (hairstyle, hair, skin and eye colour).
- **Jenny's builds:** the integration session packages the estate build to `Build\Windows` in its worktree,
  and the **"Homestead Estate"** desktop shortcut launches it. The earlier procedural-woodland
  survival game continues separately as the MVP on the `mvp-survival` branch, packaged to
  `E:\Repos\HomesteadMVP\Windows` (the `Homestead` shortcut). The two lines never merge.
- The old woodland map (`/Game/SurvivalGame/Maps/Homestead`) is still in the project: packaged test
  suites that haven't been retargeted to the estate run there.

Unreal 5.8.2 is installed at `E:\Program Files\UE_5.8`. Several agent sessions share this PC;
read `docs\handoff\README.md` and `docs\setup.md` before building. The `Preview.cmd`/`Play.cmd`
candidate workflow below dates from the survival prototype.

## Build and run

The selected **qualified directional-menu increment** is
`Build\Releases\20260921-033354-2d257ba0\directional-navigation-06\Windows`.
It adds natural D-pad/left-stick movement between menu sections, including
downward equipment access and the return from the portrait to your selected item.
The following environment/camera improvements are retained unchanged.
It replaces the purely decorative placeholder tree groups with15 reduced,
textured Tree Small02 instances while retaining interactive resource plants,
rocks and outer-meadow grass. The clearing now adds512 native-scale authored
grass clumps (297633 triangles) and a textured ground blend; all terrain
positions, topology, normals, UVs and collision remain unchanged.
Actual ordinary gameplay verified gathering, whole-crown
framing, specific-tree inward blocking with natural sliding, and retreat.
Existing dawn is very bright/golden, the vegetation is sparse, and resource
silhouettes remain unfinished. Original tree import03 remains **FAILED**:
twelve branch source-description tangent/binormal corners are explicitly
qualified, not recast as a clean mesh or full woodland/performance/art approval.
The sapling canopy now participates in the existing camera sweep without blocking
the player. Actual settled-camera evidence removes the confirmed foreground
occluder; an early-orbit near-side surface remains, so this is not all-angle
camera completion. A controlled29-step harvest/clear/save/reload route verifies
visible canopy and query lifetime. A70-minute fresh-world route passed27.95
natural game hours, four actual harvested-node renewals and daylight/night/Rain
observation without time edits. Night heroine/near-ground darkness remains.
See `docs\research\environment-assets\readability-delivery-01` and
`docs\endurance-playtesting.md` for actual evidence and timing/memory limits.
Fresh matched grass01/grove03 screenshot-free routes measured approximately
60 actor ticks/second at a60 cap and720p output, with no intervals over33.33ms;
this is not GPU/Present timing,4K performance or uncapped-headroom evidence.
Camera02, grass01, grove03 and all earlier candidates remain available.

The selected `wardrobe-complete-10` candidate completes the native
inventory/wardrobe presentation. Actual720p/4K routes cover visible native focus,
held-stick repeat/release/reversal, short/full/empty grids and explicit quantity
editing/cancel. Existing4K equipment/dye/save, separate-process F9 and genuine
save-and-quit passed, followed by180s of fresh mapped gathering/eating/save/load.
Sealed results, failed attempts and image-review limits are in
`docs\research\character-assets\directional-navigation-01\receipt.json`.
Broad deliberate mouse switching and general held-stick comfort remain open.
All nine base combinations, real tunic/apron/footwrap layering, complete feet,
37 original icon keys, purpose pages and preview cleanup are verified. `Preview.json`
is the authority for the human build, with a fresh `wardrobe-complete-v12`
profile and byte-identical graphics defaults; no existing save was reset.

In the native menu, D-pad, left stick or arrows move within grids and across
their edges to nearby sections; triggers are not required. LB/RB or
Ctrl+Tab/Ctrl+Shift+Tab changes tabs; LT/RT or Tab/Shift+Tab remain optional
region shortcuts. A/Enter activates and B/Escape cancels/closes. Choose Amount
and activate it before changing a quantity; Back leaves editing before cancelling.
Inventory clothing actions operate on real owned
garments; Appearance changes body/hair/skin/eyes, not free outfits. Item details
and actions scroll inside their pane. From gameplay, Escape/B, Right, Activate
reaches a cancel-default save-and-quit confirmation. The preserved original
prototype below retains its older field-book controls.

To play the preserved original prototype, double-click **`Play.cmd`** in this folder
or `Build\Windows\SurvivalGame.exe`. The packaged game does not need the editor
open. The game opens borderless fullscreen at the monitor's native resolution by
default (`FullscreenMode=1` in `Config\DefaultGameUserSettings.ini`), however it is
launched; `Play.cmd` and Jenny's desktop and taskbar shortcuts also pass `-Res=0x0wf`.
Pass `-windowed` to get a window instead. In-game Settings includes **Save and quit**.

To review the separately verified movement and action improvements, double-click
**`Preview.cmd`** (requires PowerShell 7). It selects only the explicit candidate
in `Preview.json`, checks its acceptance receipt/executable hash, and uses the
persistent **`wardrobe-complete-v12`** save profile. This promotion starts
a separate profile; no reset or migration was performed. It uses
UE save schema6 / portable6; `inventory-safety-v10` and old `jenny-review` progress remain
untouched rather than being migrated or silently reset. It does not replace `Play.cmd`, import
your original world, run automated inputs, or quit automatically.

Human preview now requests monitor-sized **windowed fullscreen** through the
native startup argument; `Scripts\Start-Preview.ps1 -Windowed` opts out.
Visible fullscreen confirmation is still a human check. The separate
`offline-startup-01` Shipping candidate removes the observed Development
TraceControl listener at compile time and has one isolated startup/save/socket
proof; it is not selected until coordinator review. See
`docs\offline-startup.md` for evidence, preserved preferences and limitations.

Preview saves live in
`%LOCALAPPDATA%\SurvivalGame\PreviewProfiles\profile-wardrobe-complete-v12\SaveGames`.
Manual saves, all three autosaves, recovery and backups stay there. Relaunch
`Preview.cmd` to continue; use `Play.cmd` to return to the untouched original.
Prior selected-build graphics preferences were copied byte-for-byte; the old wave
package/profile, both wardrobe candidates and single-tree03 remain available. The new candidate's
`previous-preview.json` records the prior selection for an explicit rollback.
**Do not launch the candidate executable directly:** the preview-profile argument,
not its package folder, selects this isolated save namespace.

The review candidate retains the accepted relaxed locomotion and generic
gathering, watering, weeding and sapling-hatchet feedback. These are technical
improvements, not Jenny's aesthetic approval or precise hand/tool-contact IK.
Rejected facial-shader and shortened-wave trials are not included. The requested
mid-back waves are now a disclosed rough increment (broad locks/scalloped ends
remain); the Melinoe-inspired blonde-bob direction is still deferred.
See `docs\setup.md` for profile selection, verification and known limits.

Controller prompt stability and the limited clarity introduced in
parent-reviewed **`book-clarity-01`** are retained in the selected preview.
The book distinguishes
**Your pack**, **Crafting recipes** and **Building plans**, labels carried/chest
counts separately from required materials, and names actual eat/take/craft/plan
actions. It adds no recipe learning, unlocks, inventory expansion or new navigation.

The movement tearing/flicker report is **investigated, unresolved**, not fixed.
Read-only diagnostics found a reported 4K/30Hz desktop mode and a VSync-off,
60fps-capped diagnostic runtime; neither alone proves tearing. Offscreen game frames do not
observe physical scanout. See `docs\presentation-diagnostics.md` for exact runtime
settings, capture limits and the next human check. No graphics defaults changed.

The current candidate retains the **`hotkey-safety-01`** correction and `video-sync-01`
**Settings > Vertical sync** control, a
reversible On/Off toggle for later human comparison. Default remains Off;
tearing is still unresolved. The current schema-named profile is `jenny-review-v5`.
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

To let AI agents drive the live editor (load the map, play in editor, capture
screens, read logs, edit actors/assets), run `.\Scripts\Start-EditorMcp.ps1`.
It enables Epic's built-in Unreal MCP server for that editor session only; see
`docs\editor-mcp.md` and the `unreal-editor-mcp` skill in `.github\skills`.

`Build-Game.ps1` fetches the licensed source assets, compiles the native editor
module, generates the materials/map/audio assets in the editor, and optionally
cooks/packages the game. Failed steps stop with an explicit error.
That ordinary Development workflow can open Unreal trace listeners; it is not
the offline Shipping/reused-container path. Honor the current launch hold and
use the explicit procedure in `docs\offline-startup.md` for this release.

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
| Aim placement | Walk + right stick | WASD + mouse |
| Rotate placement | RB / LB or X | R, F or mouse wheel |
| Place piece | A | E or left click |
| Save / load | Field book Settings | Settings or F5 / F9 |

The retained `hotkey-safety-01` correction removes inherited engine
debug commands from F5/F9 without changing their save/load actions. It does not
force a graphics mode after saving. See `docs\hotkey-safety-playtesting.md`;
historical selection checkpoint `af0c907` used `jenny-review`; the current
wardrobe delivery deliberately uses the separate `jenny-review-v5` profile.

The field book and construction preview pause simulation. The book is
translucent so the woodland shows through, and the pack and chest pages have no
details pane: every item action is on a click. Right-click any pack or chest tile
(F or controller Y) for its actions: eat, pin/unpin, wear, move to or take from
the open chest, drop 1, drop all, pick an amount, or sort. Shift+click moves the
whole stack between pack and chest; with no chest open it pins a tool to the
hotbar or wears a garment. Ctrl+click a stack opens an amount slider (drag,
mouse wheel, or Left/Right; LB/RB step by 10) to split off, drop, or move exactly
that many. Drag a tile to rearrange or move it; X splits a stack in half. Click
an equipped slot to take off or swap what she's wearing. The portrait beside the
pack shows her live, as she stands in the world, with whatever she's wearing or
holding.
Food selections consume one item; core actions explain unmet requirements.
Action cues ("[A] Gather", "[LMB] Fell") retire once you've done each action
three times; Settings > "Show action hints again" brings them back.
Settings is a centred column about a third of the screen wide: Resume, then
Save, Load latest save and Quit game side by side, then Game, Sound and Video
tabs (Game holds speed, camera, autosave, new woodland and hint reset; Sound the
three volume sliders; Video resolution scale and vertical sync). D-pad Down
walks Resume, the top row, the tabs, then the list; Left/Right on the tabs
switches tab. Video also offers 100/85/70-percent 3D resolution scaling; the UI remains sharp
while TSR upscales the world.

## First session

Gather berries and eat them from the Pack page, or pin food to the hotbar (Pack page:
Pin to hotbar) and eat it with the left mouse button / RT while it's selected. Gather branches, stones, and
fiber from reeds. Make a hatchet, a stone hoe, and a watering can. Clear a
small patch, build a floor/walls/doorway/roof, and add a bedroll, chest, and fire.
Collect planting stock from wild roots, hoe a garden square (one 1 m square per stroke, just ahead of her), and kneel to plant it with the seed you choose (E roots, F berry seeds). Refill the can
at the stream; water and weed as needed. Fuel the fire with branches and prepare
the simple root recipes. Food and warmth matter while time passes. Work spends
Energy (felling and tilling most, gathering little) while time alone tires her
only slowly; when she's too exhausted to work, eat or sleep.

On a bare plot, **A/E plants roots using seeds**; **X/F plants berry seeds using
one foraged berry**. Once planted, X/F weeds. Mature roots produce roots and seeds,
then leave the plot available for replanting; mature berry plants give six berries
and remain in place to regrow. The Look page changes appearance without making a
cosmetic apron into free winter insulation.

## Prototype limits

- Character hair/cloth motion and lighting are provisional; small garment overlap
  can occur in some poses. Fine-grained face/body sliders are a later feature.
- Construction is a small snapping kit, not voxel terrain editing or a full
  furnishing catalog. There is no world grid: a foundation goes wherever she
  aims (about 3.5 m ahead, at any angle) and starts a new building. Pieces aimed
  near a building snap onto it, Satisfactory-style: foundations lock to a free
  side, walls, doorways and roofs to its edges and cells, and furniture inside
  its floor. Unsnapped pieces rotate in 15° steps and snapped ones in quarter
  turns. A red preview means the spot is blocked; the panel says why. The landscape is a compact authored clearing, not the
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

The opt-in `Scripts\Test-Endurance.ps1` exercises one continuous45-minute mapped
route in a disclosed copied test homestead with isolated saves/graphics. The first
run passed fixed participation/action/save criteria across a natural night and
morning. See `docs\endurance-playtesting.md` for exact results and limitations;
this diagnostic package does not replace the selected human preview.
Its post-F5 rendering was later identified as ShaderComplexity. The separately
authorized normal-Lit confirmation is documented in
`docs\endurance-lit-playtesting.md`; it preserves the original frozen criteria,
observes actual render flags every tick and never forces Lit.
The separate targeted `Scripts\Test-ForageRenewal.ps1` covers selected wild-resource
renewal through normal sleep and save/load; `docs\forage-renewal-playtesting.md`
records actual component/HUD/frame evidence and the distinction from elapsed-time
endurance. No game rules or selected preview were changed.

The bounded `feedback-layout-01` candidate keeps active success/error feedback
away from book headings without moving rows or changing controls.
See `docs\feedback-layout-playtesting.md`. That correction is retained in the
explicitly reviewed hotkey-safe selection.
Read `docs\debug-hotkey-evidence.md` for the newly confirmed inherited F5/F9
debug-command conflict and appended qualifications to earlier rendering evidence;
the input-conflict fix is a separate follow-up, not part of this UI change.

Prototype meshes are original; selected landscape/audio assets are licensed
separately. See `docs\asset-credits.md`. No game assets from the inspiration titles
are used, and the supplied portrait must remain a local reference.
