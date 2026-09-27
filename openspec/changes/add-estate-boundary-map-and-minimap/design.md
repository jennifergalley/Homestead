# Design

## Context

- The HUD is a Canvas `AHomesteadHUD`, plus a Slate `SHomesteadHotbar` overlay that registers
  a protected region.
- The field book is `SHomesteadMenu`. It has tabs, directional focus regions and Settings
  sub-tabs.
- Simulation is the portable authority, and presentation reads immutable snapshots.
- `author-fixed-cornish-estate-map` publishes `DA_EstateLandmarks`, whose named polygons
  include `EstateBoundary` and `ForSaleParcels`.

## Goals / Non-Goals

**Goals:**

- Always-available orientation at almost no render cost.
- Ownership is one Simulation fact that building, the map and later purchases all share.

**Non-Goals:**

- Live map updates from world changes (the bake is static), fog of war, custom pins, and a
  map of the mine interior.

## Decisions

### 1. Parcels live in Simulation; their polygons come from the landmark asset

- At a new game, parcels are created from the landmark polygons. The home estate is owned and
  the other parcels are for sale.
- Polygons are stored in world XY centimetres, as simple non-self-intersecting rings.
- Queries: `ParcelAt(Point)`, `IsOwned(Point)`, and `Result CanBuildAt(PlacementTarget)`.
  `CanBuildAt` checks every footprint corner against the owned parcels.
- Placement validation calls `CanBuildAt`. Its failure uses the existing invalid-placement
  feedback, with the message "Outside your estate".
- Saves store parcel ownership by stable parcel ID. The polygons aren't saved, because they
  come from the level.

### 2. One baked cartographic texture

- An editor utility places a temporary orthographic `SceneCapture2D` over the Estate level.
  Its show flags are limited to landscape, water, foliage and static meshes.
- It captures at 8192² for 4 km, which is about 0.5 m per texel. A stylising pass then
  tints to parchment, inks the coastline and water edges, and lightens the road.
- The output is `T_EstateMap`, a texture with mips, together with an origin and size
  transform.
- The texture is re-baked whenever the terrain changes meaningfully. The re-bake is a
  one-click editor action, documented in `docs\setup.md`.
- **Alternative considered:** a live `SceneCapture2D` every frame. Rejected, because it adds
  constant GPU cost for a static world.

### 3. Minimap widget

- `SHomesteadMinimap` is a Slate overlay in the style of the hotbar. It's circular, about
  220 px at 1080p and scaled by the existing 4K UI rules.
- It draws a UV crop of `T_EstateMap` about 120 m across, centred on the heroine. North-up is
  the default. Optionally it rotates with the camera yaw, with an N marker.
- Overlays:
  - The owned boundary as a dashed line.
  - Landmark glyphs clamped to the rim when off-crop.
  - The player arrow, drawn last.
- The widget registers a protected region top-right, so toasts and context panels avoid it.
- It reads a per-frame snapshot (player XY and yaw) from the controller and never touches
  Simulation.

### 4. Map tab

- A new field-book tab, **Map**, sits after Inventory, Craft and Settings in the icon-tab
  bar.
- It shows the full texture fitted to the content area, with parcel shading: owned parcels
  are clear, and for-sale parcels are hatched with a "For sale" label (inert this round).
- Landmark glyphs carry short names, and there's a pulsing "you are here" marker.
- Input:
  - Mouse: drag to pan, wheel to zoom.
  - Controller: the left stick pans, and the right stick up/down or the triggers zoom.
  - D-pad and directional focus can step between landmarks, showing each one's name and a
    one-line description.
  - LB/RB still switch tabs.
- The Map tab keeps the menu's pause.

### 5. Boundary toast

- The controller tracks `IsOwned(heroine XY)` each tick, with hysteresis of about 2 m so a
  single step doesn't flicker it.
- On a change it shows a feedback toast built from the player-chosen estate name, supplied by
  `add-ruined-manor-and-arrival`. It's shown at most once per crossing, and never while menus
  are open.

## Lanes and ownership

This lane owns:

- The Simulation parcel code, and `SHomesteadMinimap`.
- The Map tab code in `SHomesteadMenu`, coordinating with other menu edits at merge.
- The map-capture editor utility, `T_EstateMap`, and the parcel tests.

It reads:

- `DA_EstateLandmarks`, owned by the world lane.
- The estate name, owned by the arrival lane.

## Risks / Trade-offs

- **The bake goes stale after terrain edits.** → A one-click re-bake, with a check in the
  release checklist.
- **The top-right space is contended.** The calendar HUD redesign lands in round 2 and must
  share this corner. → The minimap owns a registered region, and round 2 composes the clock
  and date around it.
- **Polygon precision against the terrain.** → The boundary follows natural features where
  possible (hedge lines, streams, cliff edge) and is refined with Jenny on the map.

## Open Questions

- The minimap's default size and zoom are to be tuned on Jenny's first playtest.
