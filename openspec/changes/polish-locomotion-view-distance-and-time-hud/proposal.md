# Proposal

## Why

The heroine's ordinary walk still reads as robotic despite reduced foot sliding, distant woodland ends too close to the camera, and the wide upper-left calendar block does not match the compact cozy presentation Jenny wants. These are important presentation issues, but they belong in a later focused polish round rather than expanding the current contextual-HUD/sprint delivery.

## What Changes

- Re-author ordinary walk motion with more natural pelvis/torso counter-motion, arm swing, weight transfer, stride variation, and less mechanical symmetry.
- Add readable start, stop, and turning transitions plus terrain-aware foot placement so direction changes and slopes no longer rely only on a looping walk crossfade.
- Preserve the heroine's skeleton, appearance/equipment compatibility, controller responsiveness, collision, simulation authority, and root-motion-disabled movement.
- Increase visible world distance beyond the current 5x5 terrain window and 45-70 m cover culling by extending noncolliding terrain/tree/cover LOD rings.
- Make farther rendering incremental and budgeted so it does not worsen the already disclosed synchronous ~700 ms active-window publication pause.
- Replace the wide upper-left season/day/time panel with an original compact upper-right card inspired by the reference's information hierarchy: weather/daylight icon, season and day, and clock time.
- Use Homestead's own Pine/gold/linen visual language and original icon geometry; do not copy Coral Island artwork, fonts, exact shapes, currency display, quest UI, hotbar, or companion elements.
- Keep day/night, rain, pause state, accessibility, 720p/4K layout, and current game-state authority exact.

The smallest useful future result is a compact original top-right season/day/time/weather card plus a modest farther noncolliding terrain ring in the actual game. The first playable locomotion result replaces the current walk clip and adds one planted stop/turn transition that is visibly less robotic at ordinary camera distance. Full-round acceptance requires the complete walk/start/stop/turn/slope set, increased view-distance comparison, chunk-transition and cadence evidence, responsive HUD review, and an immutable Shipping candidate.

Deferred scope includes motion matching, mocap purchases, root-motion navigation, full-body procedural locomotion, dynamic-resolution redesign, a minimap, currency/economy UI, quest tracker, hotbar, and wholesale adoption of another game's interface.

## Capabilities

### New Capabilities

- `locomotion-polish`: Natural walk, start, stop, turn, and slope-aware heroine presentation.
- `world-view-distance`: Farther terrain and woodland rendering with deterministic LOD and streaming budgets.
- `time-weather-hud`: Compact original top-right season/day/time/weather presentation.

### Modified Capabilities

None.

## Impact

- `Assets/Characters/Heroine/Locomotion`, `Scripts/Characters`, trial animation assets, `HomesteadCharacter`, and `HomesteadAnimInstance`: deeper authored locomotion and runtime transition selection.
- `HomesteadWorld`, generated-world tests, chunk preparation/streaming, HISM LOD/cull policy, and performance evidence: expanded visual rings without expanding collision authority.
- `HomesteadHUD`, HUD geometry tests, design documentation, and prompt/feedback layout evidence: compact upper-right time/weather card.
- The user-provided screenshot is a private visual reference only. It is not copied into the repository or shipped; all game UI assets remain original project content.
- `work-animation-complete-02-shipping / work-actions-v13` remains the rollback until this later change is implemented and accepted.
