# Proposal

## Why

The heroine "somehow knows" her estate's boundaries. In the pivot that knowledge is
communicated through a world map and a minimap, not fences (`estate-life-sim-direction`). On
a 4 km map the player also needs orientation: where the manor, the mine, the road and the
town are, and whether she's on her own land. Building has to respect ownership too, since
building outside the estate is not allowed.

## What Changes

- **Estate parcels in Simulation.** Each parcel is a polygon from `DA_EstateLandmarks`,
  with an owned flag. At a new game the home estate is owned. The for-sale parcels are
  unowned and shown dimmed; buying them comes in round 12.
- **HUD minimap**, a quiet round overlay in the top-right:
  - A baked top-down map texture cropped around the heroine, with a player arrow.
  - A dashed estate boundary line and nearby landmark icons.
  - North-up by default, with a Settings option to rotate with the camera.
  - It hides with the rest of the HUD in menus, planning and cinematics.
- **World-map page** in the field book, as a new **Map** tab:
  - The full 4 km map, with the estate boundary, parcel shading and named landmarks: the
    manor, the mine ruin, the cove, the gateway, the dirt road, the town and the general
    store.
  - A "You are here" marker.
  - Pan and zoom with mouse drag and wheel, or with the controller's left stick and right
    stick or triggers. There's full directional focus parity.
- **Boundary feedback.** Crossing the boundary shows a brief, unobtrusive toast: "Leaving
  the {Estate name} estate" or "Entering the {Estate name} estate". Nothing blocks movement.
- **Build restriction.** Build placement outside owned parcels is invalid, with a clear
  reason. Foraging and gathering outside stay allowed.
- **Baked map pipeline.** An editor utility captures one high-resolution orthographic
  top-down render of the Estate level. It stylises the capture into an original
  cartographic look (a parchment tint, contour hint and coastline ink), and saves it as a
  texture plus its world-to-UV transform. The minimap and the world map share it.

## Reuse research

- **Approach:** a baked orthographic capture drawn in Slate, rather than a live per-frame
  `SceneCapture2D`. This is the community and Epic tutorial consensus for static worlds. See
  §6 of `docs\research\estate-pivot\first-slice-reuse.md`.
- **Project facilities:** the `SHomesteadHotbar` Slate overlay pattern (a pointer-aware
  gameplay overlay with a HUD-protected region), `SHomesteadMenu` tabs and directional
  navigation, `SHomesteadIcon` original icons, the toast/feedback system, and the Settings
  tabs.
- **Comparative references (conventions only, no copied art):**
  - Coral Island: top-right minimap and a full map with named locations.
  - Stardew Valley: a full-screen map with a "you are here" marker.
  - Dreamlight Valley: a clean map with a landmark legend.
- **Custom gaps:** the parcel model, the capture-and-stylise editor utility, the minimap
  widget, the map tab, the landmark icons, and the build-ownership check.
- **No external assets or licences.** Map icons are original `SHomesteadIcon` glyphs.

## Smallest useful result and first playable demonstration

**Smallest result:** in the packaged game, the top-right minimap shows the heroine's arrow
over the estate map with the dashed boundary. Walking out along the road shows the "Leaving
the estate" toast, and the arrow crosses the line.

**Full acceptance** adds:

- The Map tab with pan and zoom, and landmarks.
- The rotate option.
- Build-placement rejection outside the boundary.
- Controller and mouse parity.
- 720p and 4K layouts.
- Save and load of parcel ownership.

**Deferred:** fog-of-war or discovery, custom map pins, worker assignment on the map (round
10), parcel purchase (round 12), and the mine map (round 7).

## Capabilities

### New Capabilities

- `estate-map-and-minimap`: Estate parcels and ownership, boundary feedback, the HUD minimap,
  the world-map page, the baked map texture, and the ownership-based build restriction.

### Modified Capabilities

None.

## Impact

- `Homestead::Simulation`: `Parcel { id, polygon, owned, forSale }`, point-in-parcel queries,
  a placement validity check, and save fields.
- New `UI\SHomesteadMinimap.*`, a Map tab in `SHomesteadMenu`, new `SHomesteadIcon` glyphs,
  and a HUD protected region in `HomesteadHUD`.
- Editor utility `SurvivalGameEditor\EstateMapCapture.*`, which outputs
  `Content\SurvivalGame\UI\Map\T_EstateMap` plus a transform asset.
- A Settings entry for "Minimap rotates with camera".
- Tests: parcel geometry and build rejection in portable Simulation tests, and minimap
  transform and focus navigation in the native menu tests.
- It depends on `author-fixed-cornish-estate-map` for the landmark polygons and the level
  to capture.
