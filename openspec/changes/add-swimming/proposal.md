# Swimming (backlog)

## Why

Jenny, on 2026-09-30: "We don't have to add swimming just yet (though I'd ideally be able to wade into the water and
swim to the other side with a swimming animation), but I'd like it on the backlog somewhere." Today she can only wade
knee deep: an invisible wall (`AHomesteadWaterPool::WadeLimit`) stops her at the estate lake, and the sea's depth check
stops her in the sea and estuary.

## What Changes

- Wading deepens gradually: walking slows and her stride shortens with depth, and water splashes at her shins.
- Past about waist depth she swims: a breaststroke with her head above water, slow and steady, turning freely. She can
  cross the lake and come out on any shore.
- She can't use tools, gather, carry the lamp lit, or fill the pail while swimming. Her pack stays on (no weight rules
  yet). Swimming costs Energy; at low Energy she's told to make for the shore.
- Out of the water she drips for a little while (a wet-hair and clothes look; part of add-rain-weather 1.9's wet
  sheen).
- Scope: fresh water (the lake, the river's pools) first. The sea, with its swell and currents, is a separate
  decision.

## Impact

- New: the swim and tread clips (MetaHuman Control Rig, `homestead-animation-layer`), a swimming movement mode on
  `AHomesteadCharacter`, depth from the water surface (the lake's level, the river ribbon), splashes and wet surfaces.
- Changes: the lake's wading wall goes, or moves to the deep-water edge for a "too tired to swim" refusal. The sea's
  depth check stays until the sea is decided.
- Simulation: Energy drain while swimming, native-tested.
