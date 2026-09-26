---
name: Homestead native visual language
description: Built Unreal world, Canvas HUD, and third-person camera reference.
---

# Homestead design reference

## Overview

Preserve Jenny's liked **warm, peaceful clearing**: open foreground, enclosing woodland, approachable countryside, and a restrained field-book interface. This records the built Windows-native Unreal game, not a replacement concept or final-art approval.

Authority: `docs\game-plan.md` and `PRODUCT.md` govern product intent. Implementation facts below come from `Source\SurvivalGame\HomesteadWorld.h/.cpp`, `HomesteadHUD.cpp`, and `HomesteadCharacter.cpp`.

Evidence boundary:
- **Directly seen:** the earlier clearing screenshot, with coherent woodland framing, sparse provisional vegetation, a cylinder stand-in, and washed-out highlights.
- **Parent-reported validation:** exposure correction compiled; comparable world-ROI pixels with all RGB channels at least 250 fell from 7.86% to 0%. True 1920×1080 capture, revised HUD placement, and Look framing were subsequently exercised.
- **Not directly seen here:** corrected captures, current heroine, updated field book/Look, or night/rain views. Source facts are not claims of visual approval, contrast certification, or measured frame rate.

## Colors

Values below are the source's **linear `FLinearColor` RGB**, not sRGB hex swatches. Alpha is noted where non-opaque; do not copy these numbers into an unrelated color space.

| HUD token | RGB / alpha | Built role |
|---|---|---|
| Ink | `(0.93, 0.93, 0.84)` | Main text |
| Muted | `(0.71, 0.77, 0.69)` | Secondary details and hints |
| Gold | `(0.92, 0.74, 0.43)` | Selected labels, active-tab underline, actions |
| Pine | `(0.055, 0.09, 0.075)`, alpha `0.96` | Backed HUD/book panels |
| Warning | `(1.0, 0.67, 0.48)` | Errors and need values below 25 |
| Selected row | `(0.09, 0.14, 0.105)` | Opaque selection backing |

World fallback tints: Meadow `(0.22, 0.31, 0.095)`; Leaf `(0.12, 0.26, 0.065)`; LightLeaf `(0.27, 0.37, 0.095)`; Bark `(0.19, 0.105, 0.052)`; Wood `(0.37, 0.23, 0.115)`.
Stone is `(0.32, 0.36, 0.34)`, Soil `(0.16, 0.085, 0.039)`, Cloth `(0.55, 0.43, 0.25)`, and placement preview `(0.65, 0.79, 0.77)`.
Optional textured `M_Ground` and `M_Rock` supersede flat ground/rock treatment; preserve their material color rather than tinting textured rocks gray.

## Typography

- Native Canvas text uses `GEngine->GetMediumFont()` throughout; there is no authored font pairing or separate bold-weight system.
- Sizes are virtual-screen units: book title 38, Look title 35, recovery title 43; row/main labels 21–24; details and hints 16–20.
- `Write` scales against the font's maximum character height and HUD scale.
- Wrapping is word-based, with line advance `size + 7`; book/context calls retain their line caps. Transient feedback draws its complete wrapped lines and sizes its backing to them. It is not automatic reflow of the entire interface.
- Preserve concise action copy and explicit device bindings; long translations/labels require checking for truncation.

## Layout

- HUD scale is `clamp(viewport height / 1080, 0.4, 1.5)`; virtual width/height divide actual pixels by that scale. This bounds 4K text/controls without changing the 3D render scale.
- Calendar/weather occupies the upper left `(30,26)`, size `460×73`. Current-device book/camera hints have upper-right Pine backing.
- Food, Energy, Warmth occupy the lower left in three 200-wide backed meters; numeric values accompany 174×5 bars.
- A gameplay-only ten-slot Pine/cream/Gold tool hotbar is centered along the
  bottom. It retains the Knife, Hatchet, Stone Hoe, and Watering Can slot
  assignments without adding capacity, but hides uncarried tool icons.
  At 4K its physical size is bounded independently of 3D resolution.
- Normal focus/action context sits lower right: width `min(650, 38% of virtual width)`, right inset 32, top at `height−225`.
- Planning instead uses a lower centered panel up to 880 wide. World/planning toasts remain centered below the top band, up to 900 wide. While the book or Look is open, feedback moves to the free upper-right band at Y26/right inset30, beside the calendar rather than across the book heading. The existing 92-high backing grows if measured lines require it; book rows and footer never move with feedback.
- The legacy Canvas field-book fallback is centered, up to `1180×810`, with seven tabs, 74-high rows and a selected-row-following visible window; the normal game uses the native Slate menu described below.
- The actual native Slate field book uses the viewport rather than a miniature
  centered book: 1280x720 logical at 720p, expanding toward 2560x1440 logical
  at 4K. Individual font, icon and control sizes remain bounded while
  portrait/content/details columns and inventory-grid capacity reflow.
  Guidebook/Credits selection shows its text without an inert Read action.
- Look uses a left sidebar at `(32,156)`, width `min(500, 35% of virtual width)`, up to 790 high; its scene preview remains visible rather than receiving the normal full-screen book scrim.
- Most panel contents use 22–40-unit insets. Panels are plain rectangles, separators are thin rules, and a selected tab has a 3-unit underline.

### Camera and Look

- Gameplay uses a collision-tested spring arm, default distance 470 cm, socket offset `(0,45,55)`, FOV 75°, and camera lag speed 12.
- Ctrl+mouse-wheel distance changes by 80 cm per unit, clamped to 250–1000 cm;
  unmodified wheel cycles hotbar slots; gameplay R3 toggles 380/740 cm.
- Look saves the gameplay view/distance, enters at 280 cm and pitch −6°, facing back toward the character; closing restores the saved view and normal offset.
- While Look is open, framing recomputes from actual viewport size, sidebar width, FOV, and arm length: a negative lateral socket offset places the heroine in the unobscured right region; vertical socket offset is 10 cm.
- Look allows mouse/right-stick orbit; R3 selects 190/320 cm views. Wheel zoom retains its separate 250–1000 cm clamp.

## Elevation & Depth

- HUD depth is tonal layering: near-opaque Pine over the world, opaque selected rows, and a 0.62-alpha dark scrim behind the ordinary field book. No decorative blur, gradients, or drop-shadow system is authored in this HUD.
- World owns sun, moon, real-time skylight capture, sky atmosphere, and exponential height fog; do not duplicate these in another actor.
- Sun intensity follows time of day up to 65,000 lux clear / 19,000 lux rainy. Its color moves from warm `(1,0.49,0.24)` to `(1,0.94,0.81)`; moon peaks at 0.5 lux with cool tint.
- Skylight intensity varies 0.3–0.85. Fog density is 0.007 by day, up to 0.016 at night, or 0.035 in rain; fog starts at 1100 cm.
- Unbounded histogram exposure uses extended-luminance **EV100 0–16**, compensation +0.5 stop, and adaptation speeds 3 stops/s toward brighter scenes / 1 stop/s toward darker scenes.
- The EV floor limits night brightening; it is not a guarantee of nighttime readability. Compare settled dawn/day/night exposures before changing physical light strengths or the established warmth.
- `M_Field` supplies Tint, Roughness, and Glow; ordinary parts default to roughness 0.85. Only fueled fire produces emissive flame shapes and a warm point light.

## Shapes

- Terrain spans −4000…4000 cm on both axes with a 25 cm procedural collision grid and matching analytic placement height; the home center is `(-1000,0)`.
- Deterministic tree clusters concentrate toward the edges; the home area has at least 650 cm decorative clearance, with further exclusions around resources, structures, and plots.
- Trees combine cylindrical trunks with tiered cones or rounded crowns; grass and wildflower clumps are instanced. Trunks/large rocks block movement; decorative plants do not.
- The shallow meandering stream uses low-roughness colored water and bank ribbons over the terrain, not a fluid simulation.
- Optional imported moss rocks are bounds-normalized and use simple hidden collision proxies; unavailable assets retain logged primitive/material fallbacks.
- Buildings use a 300 cm cell grid, timber-colored slabs/beams, 260 cm walls, and a doorway with a 130 cm structural opening and tied-back nonblocking cloth.

## Components

- **Resources:** branches and loose stones are small ground groups; berries sit on rounded bushes; roots have low leaves; flowers have distinct blossoms; reeds are original upright green stalks with brown heads and a separate stubble mesh; saplings use small conifer forms.
- Harvesting removes produce while keeping plant bases where applicable; clearing removes the complete node. Resource visuals are keyed by ID and update on state changes.
- **Crops:** nine small soil tiles follow the ground. Moisture darkens soil and lowers roughness; staged growth scales plants; mature roots become visible; weeds add separate yellow-green shoots.
- **Shelter:** foundation, wall, doorway, roof, fire, bedroll, and chest have separate shapes. Fire, bed, and chest use different within-cell offsets to preserve a central route.
- Wall orientation is `0 north (+Y), 1 east (+X), 2 south, 3 west`. Preview uses the same geometry, pale neutral tint, no collision/shadow, and no red/green validity promise; controller feedback explains rejection.
- **Field book:** Pack, Craft, Build, Notes, Settings, Credits, Look. Selected labels are Gold on a darker opaque row; active tabs also have an underline. The first three page titles are "Your pack", "Crafting recipes" and "Building plans". Pack rows label Carried and nearby Chest quantities separately; recipes/plans label authoritative costs "Needs". The selected footer names eat/take/craft/plan, with store/take hints only when stock exists. Inert tools/materials advertise no primary pack action. Empty Pack copy describes the actual empty state, not missing recipe knowledge. No unlocks, new screens or item reordering are implied.
- **State/input:** menus and construction planning pause simulation and expose that state in text. Movement is blocked in the book; camera orbit remains enabled specifically in Look. Hints switch between gamepad and keyboard/mouse bindings.
- **Item popups:** right-clicking a pack or chest tile opens a small Pine context menu at the pointer (Gold title "Name xN", one full-width row per action, Cancel last) over a dim click-away scrim, clamped inside the book. Ctrl+click opens the same panel as an amount picker: a large "N of M" readout, a slider, one hint line, and action rows that name the amount ("Drop 12", "Split off 12"). Popups never replace the details pane; they close on any action, Escape/B, or a click outside.
- **Pack portrait:** the left column shows the live heroine cut out against the book (no scenery), studio-lit, full height, with her current clothes and held tool; the caption reads "As you look now".
- **Action cues:** keyed world cues ("[A] Gather") stop appearing after three successful uses of that verb on that kind of thing; unkeyed guidance ("Select Hatchet to fell") always shows. Settings row 15 "Show action hints again" restores them.
- **Tool hotbar:** `1-9/0`, wheel, pointer, and gameplay LB/RB select ten
  world-specific tool references. Left mouse/controller RT uses the selected
  carried tool through existing authority. Menu LB/RB remains tab navigation.
- **Feedback:** errors use warning text and explanatory messages, not color alone. Need bars retain labels/numbers. Recovery provides explicit checkpoint retry copy. Key hints and the prototype label have their own backing.
- **Vertical sync:** appended Settings row 11 uses the same scrolling rows, On/Off text and selected-device toggle hint. It separates the saved preference from a conflicting active engine override. Copy notes possible tearing reduction/input delay without claiming every flicker is fixed; there is no new panel or default change.
- **Heroine:** runtime supports long-wave/bob skeletal variants, shared idle/walk clips, and material-based appearance changes. Missing required assets keep an explicitly labeled cylinder; the HUD calls a loaded heroine a character prototype.

## Do's and Don'ts

- Preserve the liked warm clearing composition, quiet interaction rhythm, and readable Pine/cream/Gold hierarchy; refine defects without wholesale restyling.
- Keep the heroine clear of the Look sidebar, gameplay context off the central character, and essential hints backed against changing scenery.
- Treat clipped-highlight measurements as bounded evidence, not a blanket visual-quality or contrast pass; record capture resolution and allow exposure to settle.
- Do not present cone/sphere foliage, simplified water/fire/cloth, or provisional character assets as approved realistic final art. The current world mixes original technical geometry with optional CC0 textured assets.
- Do not equate source-level scaling, collision, or instancing with verified accessibility, every-aspect-ratio support, or a proven 60 FPS result. Check those in the actual native game.
- Keep this reference descriptive of shipped source; update it when implementation changes, without turning it into a feature roadmap.
