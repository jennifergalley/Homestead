# Design

## Context

See `proposal.md` for motivation. The current one-second walk cycle improved foot sliding but remains a prototype: one loop covers all movement, stops crossfade to idle, turns rotate the pawn under the same pose, and slopes have no foot placement. Existing documentation explicitly discloses robotic motion, turn/stop sliding, and absent foot IK.

The generated world renders a 5x5 set of 24 m terrain chunks, with only the inner 3x3 colliding. Async preparation already builds a 9x9 baseline halo, while decorative cover generally culls at 45-70 m and outer mature trees use forced LOD2. The selected environment still publishes UObject/procedural mesh/HISM/collision changes synchronously, leaving a measured roughly 700 ms boundary pause; `upgrade-woodland-environment-assets` section 8 already defines the prerequisite incremental-publication work.

The Canvas HUD currently draws a wide 460x73 season/day/time/weather block at upper-left. The user reference demonstrates a compact upper-right hierarchy with icon, date, and time, but its copyrighted visual assets are not project inputs. The current contextual-HUD plan removes the unrelated top-right shortcut strip, freeing that location for an original Homestead card.

## Goals / Non-Goals

**Goals:**

- Replace the robotic one-loop walk presentation with authored walk/start/stop/turn motion and conservative slope foot placement.
- Extend major terrain and woodland silhouettes to a deterministic 9x9 visual window while keeping collision and interaction authority bounded.
- Move truthful season/day/time/weather information into a compact original upper-right card.
- Preserve character customization, controls, action overlays, saves, generated identity, permanent edits, camera, and selected-build cadence as explicit acceptance constraints.

**Non-Goals:**

- Motion matching, purchased mocap, root-motion navigation, multiplayer prediction, parkour, or full-body procedural locomotion.
- Expanding authoritative collision, focus, resources, structures, or plots to the entire far visual window.
- A minimap, currency/economy, quest tracker, hotbar, companion UI, or reproduction of Coral Island's artwork.
- Solving GPU/Present/scanout performance through actor-cadence evidence alone.

## Decisions

### 1. Re-author a small locomotion set on the retained skeleton

The animation lane will produce a polished walk loop plus start, planted stop, left turn, and right turn clips using the existing Blender rig and analytical verification. The walk emphasizes asymmetric human weight transfer, pelvis/shoulder counter-rotation, relaxed arm timing, heel-to-toe contact, and less uniform interpolation. All clips remain in-place, scale one, root-motion disabled, and notify-free.

Runtime extends the explicit C++ animation proxy with movement-state and turn selection below the existing sprint/action layers. Velocity, acceleration, requested direction, facing delta, and grounded state choose clips; gameplay movement remains CharacterMovement authority.

**Alternative considered:** simply slow or speed the current clip. Rejected because cadence tuning cannot add missing weight transfer, starts, planted stops, or turns.

### 2. Add bounded presentation-only foot placement after authored locomotion

Each foot samples the authoritative terrain/collision surface beneath its expected contact. Conservative leg/pelvis offsets blend in only within measured reach and slope bounds, then decay to the authored pose when traces are missing or steep. Foot placement does not change capsule collision, pawn location, resource reach, action targets, or save state.

This may use supported skeletal-control nodes available in the installed engine; a new IK plugin or generalized Control Rig framework is admitted only if a prototype proves the current C++ proxy cannot apply the required bounded controls.

### 3. Reuse the prepared 9x9 halo as the far visual target

The existing 9x9 deterministic chunk baseline halo becomes the maximum planned visual window: inner 3x3 retains collision/interaction, the current 5x5 remains detailed visual terrain/cover, and the added two rings use noncolliding terrain plus far mature-tree and restrained silhouette cover policy. Stable global keys and sparse edits drive every ring.

Outer rings avoid resource actors, focus, navigation, overlap, structure/plot duplication, and close-detail vegetation. LOD and cull distances are derived from the actual 108 m radius rather than arbitrary unlimited values.

**Alternative considered:** increase HISM cull distances without adding terrain/components. Rejected because trees would outlive the current terrain window and expose empty ground edges.

### 4. Complete incremental publication before admitting the farther window

The deferred streaming tasks in `upgrade-woodland-environment-assets` section 8 are an integration dependency. Component creation, terrain section publication, HISM instance/tree rebuild, collision readiness, and teardown are budgeted across frames with destination ownership and cancellation. The 9x9 view is enabled only after same-route evidence shows the larger queue does not restore the original multi-second hitch.

Compatible baseline/region caches are reused. No full `GenerateRegion` call runs synchronously, and collision readiness remains atomic for the active 3x3.

### 5. Create an original compact time/weather card

Canvas draws a compact upper-right stack: an original sun/moon/cloud/rain glyph beside a rounded Pine date/time card. The first line carries season and day; the second carries a small clock glyph and `HH:MM`; pause is a concise state treatment rather than appended sentence. Layout uses current UI scale, semantic colors, existing fonts, and protected-region measurement.

No currency chip appears until an economy exists. The reference screenshot is not copied, stored, traced, color-sampled, or redistributed; it informs only hierarchy and compactness.

### 6. Separate first delivery from full-round acceptance

First delivery pairs the compact card with one additional noncolliding visual ring and one re-authored walk/start-stop slice in the integrated world. Full acceptance expands to the bounded 9x9 view, turn set, foot placement, all appearances, transitions, producer/consumer persistence, and matched performance.

Independent lanes may own locomotion source assets, HUD rendering, and streaming/LOD implementation. Shared `HomesteadCharacter`, `HomesteadAnimInstance`, `HomesteadWorld`, Editor/cook/package resources, and final promotion remain serialized.

## Risks / Trade-offs

- **[Natural motion remains robotic despite more clips]** -> Judge chronological ordinary-play footage, not source metrics alone; iterate pelvis/torso/arm timing before adding infrastructure.
- **[Foot IK causes knee pops or slope jitter]** -> Clamp reach/slope/speed, blend offsets, retain authored fallback, and inspect feet/pelvis at slow/full walk and turns.
- **[9x9 publication worsens the transition hitch]** -> Make incremental streaming a prerequisite, budget outer rings after active collision, and reject on matched boundary evidence.
- **[Far detail increases memory or overdraw disproportionately]** -> Use low LOD, sparse cover, no collision/shadows where appropriate, and record component/instance/VRAM proxies before promotion.
- **[Top-right card collides with toasts/context at narrow views]** -> Use measured protected geometry and dedicated 720p/4K active-state captures.
- **[Reference influence becomes imitation]** -> Preserve only information hierarchy; use project colors, typography, proportions, icons, and copy, with no currency or unrelated UI.

## Migration Plan

1. Record selected-build locomotion, horizon, transition timing, memory/component, and HUD baselines.
2. Complete and verify incremental component publication from the existing deferred woodland tasks.
3. Deliver the compact time/weather card and one farther visual ring in Editor.
4. Author/import the locomotion set, integrate transitions and bounded foot placement, and run ordinary visual review.
5. Expand to the accepted 9x9 outer view only after transition and cadence gates pass.
6. Build an immutable Shipping candidate and run producer/consumer, boundary, appearance, HUD, full-loop and matched performance acceptance.
7. Promote only with exact receipt/rollback; retain `work-actions-v13` otherwise.
