# Design

## Context

The current world is `AHomesteadWorld`. It builds `ProceduralMeshComponent` terrain chunks
from `Homestead::Generation` and generates trees, underbrush and rocks from a world seed,
streaming chunks around the player. `Homestead::Simulation` is the portable authority. It
holds `ResourceNode`s keyed by `GeneratedEntityKey`, records player edits (`ResourceEdit`,
underbrush edits), and saves them. Presentation reads Simulation and never mutates it
directly. The creek already uses Single Layer Water.

The pivot needs a fixed, authored 4 km world with coast, river, road and town. Other round-1
lanes need stable anchors: `add-estate-boundary-map-and-minimap`,
`add-overgrown-estate-clearing`, `add-ruined-manor-and-arrival` and
`add-dollars-and-general-store`.

## Goals / Non-Goals

**Goals:**

- A walkable, attractive first-pass fixed map, built from real Cornish terrain, in the
  shipped game.
- Keep Simulation authority and stable-ID persistence; only where the IDs come from changes.
- Give other lanes stable landmark anchors early, before the art is final.

**Non-Goals:**

- Finished art everywhere, tides, swimming, boats, mine interiors, town interiors (that's
  the general-store lane), or runtime procedural generation.

## Decisions

### 1. One World Partition level with a single 4033² Landscape

- The new level is `/Game/SurvivalGame/Maps/Estate`, a World Partition level with one
  Landscape at 1 m per quad. That's 4033×4033 vertices, 63-quad sections, 2×2 sections per
  component, and 32×32 components.
- Streaming proxies and HLODs come from World Partition.
- The Z scale is chosen from the reshaped data's elevation range. It's recorded, along with
  the GDAL command, in `Scripts\Terrain\README.md`.
- `Homestead.umap` remains for the Character Lab and tests until they're retargeted.
- **Alternative considered:** keep procedural chunks and stamp authored regions into them.
  Rejected, because the chunks can't carry a coast, a road or water bodies coherently, and
  the pivot explicitly asks for a fixed map.

### 2. Terrain provenance: real LIDAR, reshaped as a composite

1. Download EA LIDAR Composite DTM 1 m tiles for the St Agnes, Trevaunance Coombe and Chapel
   Porth coast.
2. Clip them in QGIS, reshape them into the agreed layout, and export through GDAL.
3. The raw tiles stay out of git, but the tile IDs and download URLs are recorded. The
   reshaped 16-bit master heightmap is committed with its attribution.
4. Hand-sculpt the seabed below the tideline. EMODnet stays reference only until its licence
   is read.
5. Do later reshaping in the editor on Landscape Edit Layers, so that re-importing the base
   keeps the hand edits.

### 3. Water plugin first, with the Single Layer Water fallback

- The plan is a Water plugin ocean for the whole seaward edge and estuary, a river body for
  the estate river, and a lake body.
- Before committing, prove river editing and ocean shoreline behaviour on a cut-down copy of
  the heightmap, the real 1 km estate.
- If editor iteration or runtime cost is unacceptable, fall back to hand-placed ocean, river
  and lake meshes with the Single Layer Water material and flow-ripple material, which the
  creek already has.
- Either way, the Simulation's water queries for pail refill read authored water-volume
  tags, not the generator.

### 4. Biomes: paint layers plus our own PCG graphs, baked to stable placements

- Landscape layers: WoodlandFloor, Pasture, Moorland, DuneSand, Beach, CliffRock, DirtRoad,
  Farmland.
- PCG graphs sample the layer weights, slope and distance to water and road, and scatter the
  admitted meshes.
- **Interactive resources** (choppable trees, rocks, forage) aren't left as live PCG output:
  - An editor utility bakes them into `DA_EstatePlacements` (a data asset, or JSON under
    `Content\SurvivalGame\Data`).
  - Each record holds a stable integer ID, a kind, a transform and a minimum tool tier.
  - The bake is versioned. A new bake version marks old saves incompatible, which is
    acceptable under the disposable-save policy.
- Purely decorative grass and groundcover stay as runtime-generated PCG or foliage instances,
  with no Simulation identity.
- **Alternative considered:** Simulation reads live PCG output at runtime. Rejected, because
  IDs would shift whenever a graph was edited.

### 5. Fixed-world mode in `AHomesteadWorld`

- A `bFixedWorld` path skips chunk generation and streaming.
- The ground height comes from a downward trace against the Landscape, and results are cached
  per query cell for the placement preview and props.
- Resource actors and instanced meshes are spawned from Simulation resource state, whose seed
  is the placement table.
- Felling, forage and rock presentation reuse the existing code, keyed by resource ID.
- `GroundHeight(X, Y, WorldDescriptor)` callers in Simulation are audited. Any that need
  height get it through a provider interface, filled by the Landscape in game and by a flat
  or recorded height grid in portable tests.

### 6. Landmark and anchor data asset: the shared interface for round 1

`DA_EstateLandmarks` holds named transforms and polygons:

- `StandingRoomSpawn`, `ManorFootprint` (a polygon), `EstateGateway`, `CoveBeach`,
  `MineEntrance`, `MillSite`
- `RoadEstateEnd`, `RoadTownEnd`, `TownSquare`, `GeneralStoreDoor`
- `EstateBoundary` (a polygon), `ForSaleParcels` (polygons, for later)

This lane publishes a first version within its first increment, from rough placements.
Other lanes read anchors by name only. `EstateBoundary` and parcel polygons are co-owned
with `add-estate-boundary-map-and-minimap`: this lane places them, and that lane defines their
Simulation form.

### 7. Road

- One Landscape spline runs from `EstateGateway` to `RoadTownEnd`, deforming the terrain
  with a gentle falloff and painting DirtRoad.
- Spline meshes carry wheel ruts and grassy verges, authored as Blender procedural meshes.
- The road is a wagon-width 3.5–4 m, ready for round 8.

### 8. Save and version

- `SimulationSaveVersion` bumps. New games record `WorldKind = FixedEstate` and the placement
  bake version.
- Loading a woodland-era save shows the existing incompatible-save reset notice. It never
  deletes other files.

## Lanes and ownership

This lane owns:

- The Estate level and Landscape, the landscape material, Water bodies, PCG graphs, the road
  spline and meshes.
- `DA_EstateLandmarks` (positions) and `DA_EstatePlacements`.
- `Scripts\Terrain\`, and the fixed-world mode in `HomesteadWorld.*`.

Shared with the other lanes:

- The anchor names in the data asset.
- The Simulation resource-kind additions, co-owned with `add-overgrown-estate-clearing`,
  which adds the overgrowth kinds.
- The save version bump, which is serialized at integration by whoever merges first.

Engine packaging and editor authoring are serialized resources.

## Risks / Trade-offs

- **Water plugin cost on a large Landscape.** → Proven early. Single Layer Water is the
  fallback.
- **Authoring time for 16 km².** → Round 1 needs only the estate, the road corridor and the
  town edge at first-pass quality. Everything else gets biome-level PCG dressing and is
  refined later.
- **PCG placement drift.** → The interactive resources are baked. Decorative-only PCG may
  change freely.
- **Test coverage lost with the generated-world tests.** → Retarget the smoke, full-loop,
  felling and watering tests to the fixed map. Retire the generated-world-only tests alongside
  the path.
- **Performance at 4 km.** → HLOD, foliage cull distances, and an early packaged profile on
  Jenny's PC, targeting 60 FPS with upscaling allowed.

## Open Questions

- Whether the Water plugin is reclassified in 5.8. This is checked at enable time, and the
  outcome decides decision 3.
- The exact town site on the reshaped estuary. It's settled with Jenny on the first
  terrain playtest.
