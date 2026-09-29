## Why

Jenny wants the opening days to feel like the start of Coral Island: "I also want parts of the
estate nearer to the manor to need clearing of stumps / debris / weeds / rubbish, much like early in
Coral Island. I really enjoy the early gameplay of Coral Island where you have to clear the farm
before you can do anything with it."

Round 1 scattered about 60 overgrowth placements along the drive and down the valley, and the
manor lane added its own grounds ring. The land right round the house was still mostly open
pasture, though. She could dig or build almost anywhere without clearing anything first, and the
rubbish round the ruin was static dressing that she couldn't touch.

## What Changes

- **A dense clear-out field round the manor** (ids 570000-579999). About 380 clearables sit in a
  band 2-38 m out from the ruin's walls, baked by `Scripts/Terrain/clearout.py` into
  `HomesteadEstateClearoutPlacements.inc`. The field has three moods:
  - the yard, between the ruin and the derelict farm: middens, crates, barrels, rotten planks,
    nettles and shed rubble;
  - the old kitchen garden, east of the standing room: weeds, nettles and rank grass;
  - the grounds, south and west toward the valley: stumps, saplings, bramble, rocks and the odd
    large stump or boulder.
  Walkable lanes stay open to the front door, the rear-wall gap, the farm gate, the road and every
  salvage pile. The field sits outside the ruin, the standing room and the derelict farm, clear of
  the other lanes' placements.
- **New clearable kinds:**
  - **Nettles:** scythe or by hand.
  - **StumpMedium:** a worn axe, 5 swings. With it there are three stump sizes she can clear with
    her starting axe or the next tier up: small (worn), medium (worn) and large (iron). The
    ancient stump needs steel.
  - **Four hand-cleared rubbish kinds:** BrokenCrate, BrokenBarrel, RubbishHeap and RottenPlanks.
    They yield kindling and scrap, with a chance of a useful find: twine, seed, or lead.
  - **Weeds** can now be pulled by hand as well as mown. Weeds and nettles hide a chance of
    self-sown root seed from the old garden.
- **Uncleared ground can't be used.** Every overgrowth kind declares the radius of ground it
  spoils. Tilling a square or placing a building piece that overlaps an uncleared obstacle's
  spoiled ground is refused, and the message names what to clear, e.g. "Clear the stump here
  first." Weed creep never regrows onto ground whose spoil would reach a plot or a building.
- **Satisfying clears:** a cleared obstacle swells slightly, then shrinks into the ground, while
  chips fly out: clippings, splinters or grit, depending on what it was. The existing sound and
  yield toast still play. Rubbish is cleared with the stick or stone kneel, and weeds are pulled
  with the root gather. The prompts say "Pull" and "Clear away".
- **Assets:**
  - A new Blender recipe, `nettle.py`, builds `SM_NettlePatch`: spring nettles with last year's
    dead stalks.
  - The manor lane's EstateDebris meshes (SM_BrokenCrate, SM_BrokenBarrel, SM_RubbishHeap) are
    reused, with store stand-ins until that lane lands.
  - The ruin's fallen timbers, at plank scale, serve as the rotten plank pile.
  - Wild garlic uses the existing `wild_garlic.py` clump instead of the stand-in flowers.

## Capabilities

### New Capabilities
- `estate-clearout`: the manor clear-out field, rubbish kinds and spoiled ground.

### Modified Capabilities
- `estate-overgrowth-clearing` (add-overgrown-estate-clearing): weeds are also pulled by hand, and
  every overgrowth kind gains a spoil radius.

## Impact

- Simulation:
  - `HomesteadSimulation.h/.cpp`: the kinds and names.
  - `HomesteadOvergrowth.h/.cpp`: the table rows, spoil radius, `OvergrowthSpoiling`, `IsRubbish`
    and nettle creep.
  - `HomesteadEstate.cpp`: the clear-out include, with a runtime skip.
  - Till and CheckSite refuse spoiled ground.
- Unreal:
  - `HomesteadWorld`: the visuals and the clear pop.
  - `HomesteadController`: the prompts, kneel choice, stump radius and mow summary.
- Save format unchanged: ids are stable placements, and clears persist as ResourceEdits.
- Coordination: the manor lane dropped its 8 static barrel, crate and heap rows near the ruin. They
  are clearable nodes at the same spots now. The lanes and keep-clear circles follow its layout.
