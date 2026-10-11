# Design: open the estate mine, shallow level

Draft for Jenny's review. Numbers marked **(Balance)** are placeholders until the Balance Agent sets
them in §7.

## 1. Where the underground level lives

**Recommended: in the same Estate map, deep under the mine house.** The tunnels are authored
geometry about 40-60 m below the landscape at the mine site. The trapdoor shows a short ladder climb
with a quick fade, then places her at the foot of the ladder below, and the ladder back up works the
same way in reverse. There's no level load, no loading screen, and still one world and one save.
World Partition streams the tunnels with the mine house. The sea cave already proves that enclosed
authored geometry with its own lighting works in this map.

Rejected: a separate map loaded by `OpenLevel`. It brings loading screens, a second persistent
world, and save and HUD state handed across a level change. That's new infrastructure without a
blocker that needs it.

Safety: the tunnels need their own floor collision and a "stuck" rule that returns her to the
ladder foot, never to the surface terrain. The open sea-cave card ("The back of the sea cave
teleported me up the cliffs", `jenny-mv445w7s`) is the same class of bug. That fix lands first or
alongside, and the mine reuses it.

## 2. Darkness and the lamp

- The underground has no sky or sun contribution. A post-process volume covers the tunnels with a
  fixed (manual) exposure, so the eye doesn't adapt the dark away. Lumen's sky light is occluded by
  the rock above; we verify that in PIE.
- The only lights are her oil lamp, a cool shaft of daylight at the ladder foot during the day, and
  the mine house's desk lamp upstairs. The existing lamp rules apply unchanged: 6 h of oil, burning
  while held or set down, refilled from flasks.
- **Without the lamp (or with an empty one):** near-darkness, not harm. A faint ambient level lets
  her see tunnel outlines and walk out, and the ladder shaft glows during the day. Swinging the
  pickaxe in the dark is refused ("Too dark to work").
- The current spec's chalk marks are dropped. The level is small enough, and the lit shaft is a
  clear way home (see Decision D3).

## 3. The shallow level's layout

A fixed, hand-built network about the size of the manor grounds. The ladder chamber opens onto a
main gallery that loops back on itself, with three or four short side drifts, timber props, rails,
an old ore cart and a dripping sump. Walking from the ladder to the far end takes about 30-45 s.
There are no dead ends that hide the way back, and the floors are flat enough that she can never
fall somewhere she can't climb out of. Ambient sound: drips, creaking timber and a low draught.

## 4. What she mines

| Node (`ResourceKind`, appended) | Tool | Yields | Comes back |
| --- | --- | --- | --- |
| Rock face | pickaxe, worn or better | Stone, a lot of it | every morning at 6:00 |
| Coal seam | pickaxe | Coal | every **(Balance)** days |
| Copper vein | pickaxe | Copper ore | every **(Balance)** days |
| Gem pocket | pickaxe | Quartz (common) or amethyst (rare) | every **(Balance)** days |

- Rock faces are the bulk. There are many of them, each taking a few swings, and they're laid out
  so one lamp's worth of oil (6 game hours) mines far more stone than her pack would otherwise
  gather in a day.
- Iron pickaxes use the existing tool ladder: energy × 0.85 and fewer swings.
- New `Item` rows are appended before `Count`: `Coal`, `CopperOre`, `Quartz`, `Amethyst` (Stone
  already exists). Each has a general-store buyer and an icon glyph. Stack size for Stone is
  **(Balance)**; the manor needs hundreds.
- The upstairs rock pile in the mine house is a one-time pickup of stone **(Balance)** so she can
  see the loop before going down.

## 5. Gentle failure

If she runs out of energy underground, she wakes at the mine house's ground floor, not the manor,
and loses nothing (Decision D4; the old spec lost "some ore"). It's the same cozy rule as everywhere
else: no death, no harm.

## 6. Stone and the manor (the reason for the mine)

The manor restoration is `rebuild-manor-and-decorate`, which needs a rewrite before it's built: its
spec assumes "freeform rebuilding with the snapped building system", and Jenny has just scheduled
**removing** that building menu (card `jenny-mv48a218`). Recommended replacement (Decision D1):
**restoration projects**. She walks up to a ruined part of the manor (the hall, kitchen, bedchamber,
upper floor, roof and chimneys) and sees what it needs: stone, timber, perhaps lime mortar and coins
for a mason. She hands materials over a little at a time, and once the part is fully supplied it's
restored overnight, much as in Coral Island and Stardew Valley. The mine plan only needs to supply
enough stone, at a rate Balance sets in §7, for each stage to take a few days of mixed play rather
than a week of grinding.

## 7. Numbers (Balance Agent fills this section)

Balance sets: swings, energy and stone per rock face; how many faces are on the level; refill
cadence and yields for coal, copper and gems; sale prices; Stone's stack size; the one-time pile
upstairs; a target number of mining trips per manor stage, with a total stone budget across the
manor stages; and how many stone per day a typical trip gives, against her energy and lamp oil.

## 8. Decisions for Jenny

- **D1. How the manor restores:** restoration projects that she supplies with stone and timber,
  restored overnight (recommended), versus wall-by-wall building with a new stone-only build tool.
- **D2. Build split:** (a) build 1 is the mine house plus the shallow level, and build 2 is the
  first manor restoration project (recommended, so the payoff follows straight after); or (b) both
  in one build; or (c) the mine house first, alone, then the tunnels.
- **D3. Chalk marks:** drop them; the lit ladder shaft and a looping layout are enough
  (recommended).
- **D4. Exhaustion underground:** wake at the mine house with nothing lost (recommended), or lose
  some ore.
- **D5. Mine house look:** a Cornish-style engine house (tall stone, chimney stack, slate roof) to
  match the "engine house on the clifftop" text, or a plainer stone mine office. The setting is
  moving away from Cornwall, so check whether an engine house still fits.

## 9. Lanes and ownership

Two heavy lanes, spawned together in dependency order, use the two heavy-job slots.

1. **Mine Art Agent** (Claude Opus 5.5, Blender + Unreal). Owns the mine house (exterior, the
   furnished ground-floor room, the trapdoor and ladder shaft) and a modular tunnel kit: walls, timber
   sets, rails, cart, sump, and rock face, coal, copper and gem node meshes in the sea cave's granite.
   It assembles the shallow level and places it under the mine site. Files: `Scripts/Blender/Recipes/mine_*.py`,
   `Content/SurvivalGame/Mine/**`, the level's placement script.
2. **Mine Gameplay Agent** (Claude Sonnet 5.5). Owns the simulation: item and resource-kind appends,
   node yields and refill (reusing the beach daily-refill path), trapdoor and ladder travel, the
   underground "stuck" rule, the exhaustion wake point, the dark-work refusal, store rows, and native
   tests. It also owns the actor side: the underground post-process volume, exposure, ambience, and
   icons through the item recipe. Files: new `Simulation/HomesteadMine.*`, `HomesteadControllerMine.cpp`,
   `HomesteadWorldMine.cpp`, appends to `HomesteadItems.*` and `HomesteadSimulation.h`.

Shared interface: Art publishes node socket transforms (a placement list with ids) that Gameplay
loads. Gameplay can greybox with simple shapes until Art's kit lands. Placement ids get a new claimed
range on the round page. No `SimulationSaveVersion` change is expected: new kinds and items are
appends, and the nodes' daily state uses the existing per-placement records. If that turns out
wrong, tell the orchestrator before `[ready]`.

Balance consults at planning (§7) and before `[ready]`. Jenny reviews the mine house, the tunnel
look and any new icons from in-game captures before `[ready]`.
