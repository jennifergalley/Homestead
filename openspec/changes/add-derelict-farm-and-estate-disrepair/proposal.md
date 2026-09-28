# Proposal

## Why

Jenny's round-1 playtest (round-1 polish):

- "Is there a derelict farm anywhere yet? If so, I don't see it. Please add a large area penned
  in by a crumbling wood wall where crops obviously used to grow, but which has now fallen to
  obvious disrepair and become hopelessly overgrown with weeds and brambles."
- "The estate itself doesn't seem particularly overgrown or in disrepair, so there's little
  incentive yet to go around clearing it. I think it's going to need to look a bit more trashed
  when we start."

## What Changes

- **Derelict farm** (`Anchor::DerelictFarm`, a 60 x 60 m field 19 m behind the ruin's rear
  wall, with `Anchor::DerelictFarmGate` on its south fence facing the rear-wall gap):
  - A rotten cleft-oak post-and-rail fence all round: sound, leaning, snapped and missing posts in
    runs; rails in their mortises, dropped at one end, lying in the grass or gone; a broken
    five-bar gate hanging off one hinge.
  - The ghost of the old crop rows: slumped ridge-and-furrow patches running downhill, dead
    bolted stalks on the ridges, rows of rotten bean poles, and a rusted plough left mid-furrow.
  - Hopelessly overgrown with the clearing lane's existing clearable kinds (weeds, rank grass,
    bramble, saplings, thickets and two bramble banks), ids 550000+.
- **Estate disrepair** round the manor and along the drive:
  - A collapsed lean-to against the ruin's east gable, burst barrels, smashed crates, two
    rubbish middens, an abandoned tip cart by the field, fallen roof timbers and slipped slate
    outside the walls, more ivy on the walls, and a toppled old drive fence.
  - About 190 more clearable weeds, brambles, saplings, rank grass, fallen branches and a stump
    round the grounds and along the drive's verges, clear of the drive, the salvage piles, the
    ruin's gaps and the starter forage.
- Set dressing is scenery only (`AHomesteadDerelictFarm`, `AHomesteadManorRuin`); the clearable
  overgrowth is Simulation placements. No save-format change.

## Impact

- `Source/SurvivalGame/Simulation/HomesteadEstate.*`, new `HomesteadEstateDisrepair.cpp` and the
  generated `HomesteadEstateDisrepairPlacements.inc`.
- New `AHomesteadDerelictFarm` with the generated `HomesteadEstateDebrisPlacements.inc`
  (`Scripts/Terrain/estate_disrepair.py`); more ivy and slate in `HomesteadManorRuin.cpp`.
- New Blender props: FarmFence, FarmField, FarmPlough, FarmCart, EstateDebris.
