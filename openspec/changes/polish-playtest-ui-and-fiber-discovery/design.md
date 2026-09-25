# Design

## Context

See `proposal.md` and four capability specs. The current field book uses a fixed 1280x720 Slate scene inside `SScaleBox(ScaleToFit)`; on a 3840x2160 viewport it becomes roughly 3x larger. Canvas HUD scales by height/1080, with a 3x ceiling, and the hotbar uses native Slate sizes. Music/Ambience/Effects `SSlider` widgets are nested in full-row menu buttons. The menu's global pointer preprocessor forwards left-mouse presses into `HandleKey`, which treats them as activation; in the default Session region `Activate()` can call `Back()` and close Settings before the slider receives its own click.

Guidebook/Credits rows have no real primary action; `ActionLabel()` invents "Read" for empty actions. Reeds are seeded at roughly 120 cm to either side of the creek center and yield five Fiber through `Harvest` with a starter Knife. Their world representation is five `SM_GrassMedium01_mid_b` instances, visually blending into the creek grass. Resource labels and Simulation result strings announce renewal. Existing `upgrade-woodland-environment-assets` section 8 owns the measured synchronous publication hitch (1129 -> 690 ms earlier route); the deferred 9x9 view plan already names it as a prerequisite.

## Goals / Non-Goals

**Goals:** A coherent 720p/4K field-book/HUD/hotbar with a full-viewport 4K composition but bounded typography/icon/control sizes; mouse drag ownership by slider rather than menu navigation; honest informational entries; recognizable Fiber plants at actual generated stream positions; quiet player-facing resource feedback. Reprioritize existing incremental streaming as the next independent world-performance round.

**Non-Goals:** New resource recipes/yields, changing the stream shape or world keys, tutorial popups or map waypoints, silence for actual save/transaction errors, installing an asset pack, an unrestricted UI-scale control, solving GPU/Present timing through a guessed CPU optimization, or coupling heroine acceptance to far-view expansion.

## Decisions

### 1. Reflow a larger logical layout rather than shrink the book

Retain the original Pine/cream/Gold hierarchy. At 720p keep the 1280x720 logical field-book composition. At 4K let the logical canvas grow toward 2560x1440 and still fill the viewport: the effective layout scale stays around 1.5x the 720p rendering, rather than the former 3x magnification. Allocate the additional logical width to the portrait, content and details columns; let lists show more rows instead of putting a shrunken 1280x720 snapshot in the middle. Keep icon/count tiles and legible font sizes bounded without oversized individual controls. Bound Canvas HUD with the same intent and give the gameplay hotbar a coherent explicit DPI policy. Verify actual 720p/4K framebuffer, text/icon dimensions, column geometry and Slate hit tests, not just a computed multiplier. Do not change 3D resolution scale, camera, or game-day time to make UI look smaller.

**Alternative:** cap the overall menu at 1920x1080 physical pixels in the middle of the 4K screen. Rejected after Jenny's explicit clarification: it fixes oversized text by making the whole UI tiny and wasting the display. Reducing render percentage would blur the world without fixing the interface either.

### 2. Let Slate own direct pointer controls

The menu pointer preprocessor may classify mouse intent and reject disallowed external automation, but it must not convert a mouse press/release into a synthetic `Activate()` on the current keyboard region. Let button clicks, drag/drop, held Craft and sliders use their respective Slate handlers; keep keyboard/controller activation on the existing `HandleKey` path. The slider's capture-start must not rebuild its owning row, and capture-end commits exactly once using existing `MenuCommitAudioVolume` rollback/error logic. Test cursor release both inside/outside, tab/page continuity and click when the initial region is Session.

**Alternative:** suppress Settings close after it occurs. Rejected: activation would still leak to other menu controls and lose drag capture.

### 3. Remove the fabricated informational action

For Page 3 and Page 5, selecting an entry reads its content in the existing details pane. Do not construct a "Read" button for a row with no executable action. Do not make the same empty-action rule hide real Pack/Craft/Settings operations; retain focusable list rows and details navigation.

**Alternative:** make Read open an identical modal. Rejected: adds a redundant screen to an already dense menu.

### 4. Make the admitted reed role distinct in-world

Reuse original procedural mesh/field material and the current resource ID, authority and bank-generation policy. Create a tall narrow multiple-stem reed silhouette and muted natural seed heads at the existing creek-bank nodes rather than reusing grass asset `mid_b` for the actual resource. Keep the produced/non-produced state obvious through visible tips/stalks, but do not sprinkle words or a marker on the river. Measure visibility from the ordinary approach and distinguish it from decorative grass at 720p/4K. Any density/placement adjustments stay deterministic and must not violate collision, shore/water boundaries, saved edits or generated-world tests.

**Alternative:** add "Fiber here" instructions to the field book. Rejected: the problem is that the plant does not look like a reed.

### 5. Quiet only the player-facing renewal layer

Keep Simulation's precise timing and result codes for authoritative state and diagnostics; adjust world focus/action UI and routine success/error presentation to omit renewal narration. Suppress renewable-gather success toasts and redundant clear tutorial toasts while retaining useful effects, inventory counts and concise failure feedback; never swallow save, capacity or tool ownership errors. Coordinate the existing `HomesteadForageRenewal` fixture's prompt expectations with the changed visible contract while retaining its cooldown, save/reload and anti-duplicate checks.

**Alternative:** alter resource renewal or hide failed actions. Rejected: Jenny asked for less explanation, not less simulation correctness.

### 6. Separate UI/forage from streaming ownership

The UI/input and reed/renewal lanes can prepare changes independently; `HomesteadController`, native test infrastructure, Editor packaging and final promotion remain serialized. No worktree or source actor may overwrite the active heroine trial or selected preview. When this first delivery is usable, advance `upgrade-woodland-environment-assets` tasks 8.1-8.4 under its existing OpenSpec change, profiling latest ordinary mapped crossings before picking a frame-budget publication change. Run that round before increasing the visual rings in `polish-locomotion-view-distance-and-time-hud`.

## Risks / Trade-offs

- **[Whole UI shrinks or 4K columns become empty oceans]** -> Compare full-frame menu coverage and sensible portrait/content/details widths alongside actual text, icon and hit rectangles at 720p and 4K; preserve scroll/focus and readability.
- **[Pointer router changes Craft or inventory dragging]** -> Cover held Craft, item drag/drop, split, direct chest interaction, menu navigation and external automation rejection after isolating the pointer path.
- **[New reeds overlap water or look like trees]** -> Inspect seeded daylight/overcast creek footage and verify ground placement and collisions.
- **[Quiet UI hides genuine failures]** -> Separate successful routine foraging from failure, keep short actionable errors and test an exhausted patch plus save/write failure.
- **[Hitch persists after UI work]** -> Do not label an old 690 ms measurement resolved; retain task 8's measured budget and candidate/rollback criteria.

## Migration Plan

No save schema or world identity change is planned. Keep source-visible selected preview `inventory-drop-v18` until native and ordinary packaged routes demonstrate a materially better first playable; reimport only new reed visuals if needed. Integrate separate world-streaming changes after this UX slice under their existing tasks and package independently if the heroine is still in trial.
