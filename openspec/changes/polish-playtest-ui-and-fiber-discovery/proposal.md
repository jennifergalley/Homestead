# Proposal

## Why

Jenny's 4K playtest exposed oversized menus, a volume slider that closes Settings, an inert "Read" button, and a Fiber ingredient with no recognizable source in the actual riverbank. Resource-renewal notices are explaining simulation machinery instead of letting the game speak for itself; the roughly half-second wandering hitch also needs to move up the queue.

## What Changes

- Adapt Windows-native Slate and Canvas UI at 4K so the field book continues to use the screen while text, icons, rows and hit targets have appropriate physical sizes. Reflow the larger logical canvas instead of shrinking the entire 720p menu into a centered island. Preserve 720p controls, focus and mouse/controller navigation; keep 3D resolution scale separate.
- Stop global menu-input forwarding from interpreting mouse-down on an audio slider as a Settings activation; pointer click/drag commits the chosen volume once, stays in Settings, and preserves keyboard/controller and save-failure handling.
- Remove the inert "Read" action from informational Guidebook/Credits entries. Selecting an entry should show its contents directly; do not show a fake actionable control.
- Make reeds a distinctive, sufficiently visible plant on the creek banks, starting from an ordinary new-world route, with the existing E/A gathering awarding actual Fiber before a hatchet is required. Keep the resource's seeded IDs, renewable state and authoritative inventory rules. Do not solve discoverability with more instruction panels.
- Remove player-facing "(renewing)", "will regrow", and other resource-reset commentary from world focus and routine gathering/clearing messages. Keep useful concise error feedback and the underlying renewal behavior.
- **Priority handoff, not duplicate implementation:** the previously measured roughly 690 ms synchronous chunk-publication pause matches the reported wandering hitch. Bring `upgrade-woodland-environment-assets` section 8 (incremental component streaming) ahead of extra distant-view/time-card expansion, profile the latest selected build first, then fix the dominant publication stage and review ordinary traversal. Do not claim a current hitch fix based only on an old measurement.

**First playable demonstration:** open screen-filling but well-proportioned Settings at 4K, drag each volume slider without closing it, enter Guidebook and read an entry without a dead button, then walk from a fresh clearing to an unmistakable creek-bank reed patch and gather Fiber. Text and icons are smaller relative to the 4K layout, not a miniature of the whole interface; gathering no longer announces a regrowth schedule.

**Full acceptance:** compare 720p and 4K layout/click regions, mouse and controller settings persistence and error cases, real-world resource visibility/gather/save/revisit, quiet successful and unsuccessful feedback, and fresh-process play. The independent world-streaming round has its own measured boundary/collision/save acceptance before any farther view is promoted.

## Capabilities

### New Capabilities

- `resolution-aware-game-ui`: Bounded game/menu scaling and accurate audio-slider pointer behavior across 720p/4K.
- `actionable-fieldbook-content`: Informational entries show content without inert action controls.
- `discoverable-bank-reeds`: Distinct bank flora provide the existing early Fiber supply through ordinary play.
- `quiet-resource-feedback`: Hide internal renewal state and omit redundant regrowth copy while preserving helpful feedback.

### Modified Capabilities

None; the main capability inventory has no published specs. This change coordinates but does not replace overlapping in-flight world-streaming and contextual-feedback contracts.

## Impact

Reuse existing Slate `SScaleBox`/`SSlider`, Canvas HUD, game input router, six-recipe assessment, seeded resource generator, creek/ground height and authoritative `Harvest`, plus existing original-material/tool geometry. No external download, license, new account or plugin is necessary; only a distinctive original reed silhouette and targeted input/scaling wiring are custom gaps. Likely surfaces: `SHomesteadMenu`, `HomesteadController`, `HomesteadHUD`, `HomesteadWorld`, focused native menu/forage/renewal tests and related docs. Keep the unfinished character-first change and selected `inventory-drop-v18` preview undisturbed until integrated acceptance. Incremental streaming stays owned by `upgrade-woodland-environment-assets` tasks 8.1-8.4; no task is checked by this plan.
