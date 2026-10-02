# estate-clearout Specification

## Purpose
Start the estate like the first days on a Coral Island farm: the ground round the manor must be
cleared before she can dig or build there.

## Requirements

### Requirement: The manor surroundings start densely littered
A new estate game SHALL place several hundred clearable obstacles on the ground round the ruined
manor. They SHALL mix weeds, nettles, tall grass, bramble, saplings, stumps of several sizes,
rocks, rubble, boulders and rubbish. The obstacles SHALL NOT lie inside the ruin, the standing
room or the derelict farm. Walkable lanes SHALL stay clear to the front door, the rear-wall gap,
the farm gate, the road and every salvage pile.

#### Scenario: First morning
- **WHEN** a new game begins and she walks out of the front door
- **THEN** the ground round the house is thick with things to clear, and the path to the road
  and the salvage piles is open

### Requirement: Each obstacle has its tool, tier and yield
- Weeds and nettles SHALL clear by hand or with the scythe.
- Bramble and saplings SHALL clear with the billhook.
- Stumps SHALL clear with the axe: the small and medium stumps with a worn axe, the larger ones
  only with an upgraded axe.
- Rocks and rubble SHALL clear with the pickaxe; boulders only with an upgraded pickaxe.
- Rubbish (broken crates, broken barrels, rubbish heaps and rotten planks) SHALL clear by hand and
  yield kindling and scrap, with a chance of a useful find.
- Every clear SHALL cost energy.

#### Scenario: Clearing a broken crate
- **WHEN** she clears a broken crate by hand
- **THEN** it is gone for good, she gains kindling (and perhaps scrap or twine), and energy is spent once

#### Scenario: A large stump with a worn axe
- **WHEN** she strikes a large stump with a worn axe
- **THEN** it is unchanged, no energy is spent, and the prompt says it needs an iron axe

### Requirement: Uncleared ground can't be dug or built on
Each obstacle SHALL spoil the ground within its radius until it is cleared. The game SHALL refuse
to till a square or place a building piece that overlaps spoiled ground, and the refusal SHALL
name the obstacle to clear. Once the obstacle is cleared, the same action SHALL succeed.

#### Scenario: Tilling beside a stump
- **WHEN** she tries to till a square next to an uncleared stump
- **THEN** she is told "Clear the stump here first.", and after clearing the stump the square tills

### Requirement: Clearing feels satisfying
Clearing an obstacle SHALL play the tool's strike or a hand-gather animation and a sound. The
obstacle SHALL pop away rather than vanish, with chips of what it was flying out, and a toast
SHALL show what was gained.

#### Scenario: Pulling nettles
- **WHEN** she pulls a nettle patch by hand
- **THEN** she kneels and pulls it, the patch swells and shrinks away with leaf clippings flying
  out, and the toast shows the weeds gained
