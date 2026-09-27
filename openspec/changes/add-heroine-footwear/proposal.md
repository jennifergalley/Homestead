# Proposal

## Why

The MetaHuman heroine is barefoot. A pre-industrial countryside needs footwear that matches the
seasons and the work: warm boots for winter, light sandals for summer, and an everyday shoe in
between. Warmth should be something the survival systems can read.

## What Changes

- Author three original, skinned pairs in Blender, fitted to her full body and skinned to
  `metahuman_base_skel`:
  - `SKM_FurBoots`: sheepskin worn hair side in, a fur cuff, a suede outside, a gathered hide
    sole and thong wraps. The shaft goes to mid-calf and clears snug wool trousers by at least
    18 mm.
  - `SKM_WovenSandals`: a twined plant-fibre sole with a corded rim, a toe cord, loops and
    ankle ties.
  - `SKM_TurnShoes`: soft low-cut turnshoes laced at the instep with a leather thong.
- Give each pair:
  - one material slot `M_<Name>`;
  - 2048² procedural PBR maps;
  - a body coverage mask;
  - poke-through metrics for walk, kneel, squat and tiptoe;
  - 4K review renders;
  - a README and a `report.json`.
- Add a seeded rebuild recipe (`Scripts\Blender\Recipes\footwear.py` plus `shoemaking\`) that
  reuses the outfit helpers.

## Capabilities

### New Capabilities

- `heroine-footwear`: wearable footwear assets with a known sole offset, a coverage mask and a
  warmth rating.

### Modified Capabilities

None.

## Impact

- The work adds new source assets and a new recipe. Nothing existing changes.
- The coordinator owns the Unreal work: import, materials, applying the body masks, and lifting
  the character by the sole thickness (0.5 to 1.2 cm).
