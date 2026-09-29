# Proposal

## Why

Jenny, after the round-1 polish playtest:

- "It's also currently unclear how long various seeds will take to grow to maturity, and I'm
  unsure if we've got the seeds growing as days pass (plant visibly growing and becoming
  harvestable), plus the harvesting animation and ripeness indicators."

Crops grew over game hours (30 for roots, 42 for berries), and nothing told her how long that was.
The plants were placeholder cones and spheres, a ripe crop gave no signal, and harvesting
reused the watering motion. The goal is a readable farming loop in the spirit of Coral Island
and Stardew Valley, set in 1850s Cornwall.

## What Changes

- **Growing time in days.** Every crop states how many days it takes to ripen when watered:
  - in the seed's description ("Matures in about 4 days");
  - when it's sown ("Planted turnips. Ready in about 4 days if watered.");
  - on the plot's focus line ("Turnips: day 2 of 4 | needs water, growing slowly",
    "Turnips: ready to harvest").

  Moisture and weeds still slow growth, but now in clear steps that the focus line names.
  Soil at least half wet grows at full speed and weeds under a quarter don't matter; dry or weedy
  plots grow slowly but never die.
- **Period crops.** Turnips, carrots, potatoes, cabbage, broad beans and strawberries. Each has
  its own seed, sold at Pascoe's general store, and produce she can eat or sell. The legacy
  Roots and Berries crops stay, driven by the same crop table.
- **Visible growth.** Blender-made plants for every crop in five stages (Sprout, Young, Growing,
  Mature, Ripe), with LODs. Ripe roots show their shoulders, the cabbage its head, beans their
  pods and strawberries their fruit. The plot swaps stage as the days pass.
- **Ripeness you can see (Coral Island style, from Jenny's playtest).** The produce itself (turnip
  and carrot shoulders, potatoes, cabbage heads, bean pods, strawberries) appears on the plants,
  grows continuously and colours up from pale green as the crop ripens, so a ripe crop reads as
  "done" from the gameplay camera by its full size and colour alone, with no effect on top. The focus line reads
  "ready to harvest" with a harvest prompt. A dry plot keeps its pale, cracked soil.
- **Playtest aid.** `HomesteadGrowCrops <days> [tend]` lets whole days pass so crops really grow.
- **Harvest animation.** For root crops and cabbage she kneels, pulls or cuts, and lifts the
  produce. For beans and berries she picks from the standing plant. The produce shows briefly in
  her hand and is hidden when she's done.

## Impact

- **New files:** `Source/SurvivalGame/Simulation/HomesteadCrops.{h,cpp}` (the crop table, growth
  modifiers, stages and status text).
- **Updated:** Plant, HarvestCrop and the growth tick in `HomesteadSimulation.cpp`; the plot focus
  text in `HomesteadController.cpp`; the plot visuals in `HomesteadWorld.cpp`.
- **New CropKinds** are appended after `Berries`. Plots already save their kind as an int with
  range validation, so no format change is needed for them.
- **New seed and produce Items** are appended after the architecture lane's count-prefixed stock
  save (v13), which lets appended Items load without a version bump. This change does not bump
  `SimulationSaveVersion`.
- **Other new files:**
  - Blender recipes `Scripts/Blender/Recipes/crop_*.py`
  - Content under `Environment/Props/Crop*`
  - The harvest animation scripts under `Content/Python/homestead_agent`
