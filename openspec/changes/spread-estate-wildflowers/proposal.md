# Proposal

## Why

Jenny likes the lake-walk wildflowers and selected wider Estate coverage for the Oct 4 afternoon build (`jenny-mutdnje8-0et783`). Woods, open fields and dry lake/river margins should share that colour without invading the farm, manor ruins or cultivated squares.

## What Changes

- Extend deterministic decorative scatter using the seven existing original flower assets, preserving the lake trail.
- Exclude farm/manor footprints at placement and hide flowers intersecting newly tilled squares using existing plot state.
- Keep flowers non-interactive and reuse cell-batched HISM rendering, short cull distances and change-gated masking.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `decorative-wildflower-groundcover`: add Estate-wide habitat coverage and explicit Estate exclusions.

## Impact

Reuse research: `lake_path_plants.py` already authors flower kinds 42-48; `HomesteadWorldEstate.cpp` loads their existing original meshes into scenery cells. Existing terrain weights, layout polygons and plot coordinates supply the required distribution/exclusions. No new third-party content, purchase, import, crop or forage placement is needed. The custom gap is wider scatter plus the missing plot exclusion in Estate scenery masking.

First playable result: flowers beyond the lake walk, with a newly hoed square staying bare after reload. Environment owns scatter/render helpers; Gameplay/UI keeps crop rules and interfaces. Native/compile checks establish branch readiness; Jenny's integrated visual acceptance remains separate.
