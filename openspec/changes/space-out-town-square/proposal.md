## Why

Jenny's 2026-09-29 playtest: the town buildings are bunched too tightly. Twelve blockouts ringed a 40 ×
34.5 m square only 0.2–0.35 m apart (about 0.9 m by the store), and the main road stopped 72 m short.

## What Changes

- `Scripts/Terrain/town_layout.py` lays out an open 60 × 45 m square round the TownSquare anchor.
  Fifteen buildings face it: terraces of two, sharing a party wall, and cottages, with irregular
  setbacks, depths and roofs. Side lanes of 4–5 m separate the groups (the rule allows 3–6 m).
  The general store sits in the middle of the east side. A curved 5.5 m `townStreet` (47 m, tightest
  bend 17 m) runs from the main road's end into the square's north-west corner.
- The script checks its own rules before it writes: lanes, no overlaps, nothing inside the square,
  street clearance and curvature. It writes `town` and the moved store anchors to
  `estate_layout.json`, plus test data.
- `town_massing.py` (editor) places the blockouts from the layout and deletes stale ones. It first
  loads World Partition actors so a rerun can't duplicate them. The EastHouse slot north of the store
  is kept for the parked seedsman's shop.
- The street and square are painted packed earth (weightmaps) and worn in `bake_ground.py`, and the
  map draws the square, the street and the buildings.
- The main road, its 1.94 km chainage and its anchors don't move. The town stays on reshape's plane
  pad, so the heightfield is unchanged. The store anchors move 9.5 m east, and saves keep their
  `Shop.id`: the game re-seats the store's counter on load (`RefreshShopCounters`).

## Impact

- Layout, map, ground and weightmaps; `HomesteadEstate.cpp` store anchors; `town_massing.py`;
  native checks in `HomesteadPublicRoadTests`.
