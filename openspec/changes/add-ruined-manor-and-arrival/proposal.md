# Proposal

## Why

The pivot's story begins with arrival:

- The daughter of the family comes home to an estate in ruins.
- The manor has "fallen into complete disrepair" but still shows where it stood.
- One corner room still stands, where she sleeps until she rebuilds.

The player also has to name her heroine, her family and her estate (`pivot-to-cozy-estate-life-sim`).
Today a new game drops the heroine into generated woodland with nothing built, and the
character has no name.

## What Changes

- **Ruined manor set** on `ManorFootprint`:
  - A large granite and slate country house in collapse. There are broken wall runs to
    varying heights, fallen roof timbers, a chimney stack, rubble mounds and empty window
    openings, all overgrown by the surrounding bramble.
  - The ruin is non-interactive scenery in round 1, apart from its salvage piles. It reserves
    the footprint, so she can't build over it until the dismantling in round 5.
- **Standing corner room**, one intact ground-floor room:
  - Two original walls plus patched ones, a sagging but whole roof, a door and one window.
  - A **hearth**: it's the cooking station, reusing the cookfire's recipes, with firelight.
  - A **bed**: the existing sleep and save behaviour.
  - A **chest**: existing storage, which starts holding the pail and a few Branches.
  - Heroine spawn at `StandingRoomSpawn`, facing the door.
- **Salvage piles** in and around the ruin, which yield rusted tool heads, scrap and stone.
  The kind is defined by `add-overgrown-estate-clearing`. The first billhook head is within a
  few metres of the door.
- **New-game setup:**
  - The New game flow opens the existing appearance page, reworked in 750a4397, followed by a
    short **Names** step for first name, family surname and estate name. The fields have
    sensible defaults, character limits and profanity-agnostic validation (no filtering).
  - The names are saved with the game and shown in the save list: "{First} {Surname} —
    {Estate}, Spring 1".
- **Arrival beat.** On a new game, a brief original title card fades in over the standing room:
  "{Estate}. Spring, 1851." It's followed by a short diary-style note in the field book:
  "Home at last. The house is a ruin, the fields are bramble to the hedgerow… One room still
  keeps the weather out." There's no cutscene and no voice.
- The HUD and toasts use the estate name, for example in the boundary toast.

## Reuse research

- **Project:**
  - The snapped building pieces: foundation, wall, doorway and roof. These are the patched
    walls and roof of the standing room, and they prove the rebuild path.
  - The existing Bed, Chest and Fire pieces, with their sleep, storage and cooking logic.
  - The Blender granite rock library (rubble, spalls, block talus) and procedural materials,
    which the ruin masonry and rubble build on.
  - The appearance page, the save list, the field-book pages, and the toast.
- **In-flight stone building work:** the paused "Stone building and torch assets" session
  produced no diff yet. The ruin's granite wall and slate kit is authored here as Blender
  procedural recipes. The rebuild's stone building pieces (round 5) will reuse that kit.
- **Comparative references (conventions only):**
  - Stardew Valley and Coral Island open with a modest dwelling and a short welcome letter.
  - Coral Island's character-then-name setup flow.
- **Period verisimilitude:** Cornish gentry houses of the period were typically granite
  ashlar or killas rubble stone under Delabole slate, with sash windows and tall granite
  chimney stacks.
- **Custom gaps:** the ruin kit and set dressing, the standing-room composition, the hearth
  piece, the Names step, the save metadata, and the arrival card and note.
- **No external assets or licences.** The ruin kit is procedural Blender and original.

## Smallest useful result and first playable demonstration

**Smallest result:** start a New game, keep the default look, and enter the names. She wakes
in the standing room on the Estate map. Outside the door the ruined manor looms, with a
salvage pile nearby. Sleeping in the bed saves, and the save list shows her names.

**Full acceptance** adds:

- The fully dressed ruin silhouette, readable from the road and the cove.
- Hearth cooking.
- The arrival card and diary note.
- Footprint reservation against building.
- Name validation and editing before confirmation.
- Mouse and controller parity through the whole new-game flow.
- Save and load of names.

**Deferred:**

- Dismantling and the rebuild (round 5), interior décor (round 5), and other estate ruins such
  as cottages and barns (later rounds).
- A spoken introduction or story beats.

## Capabilities

### New Capabilities

- `manor-arrival`: The ruined manor and its reserved footprint, the standing room with hearth,
  bed and chest, salvage placement, the new-game naming, and the arrival beat.

### Modified Capabilities

None.

## Impact

- New Blender recipes: a ruin masonry kit (wall runs, broken quoins, window openings, a chimney
  stack, fallen timbers, slate scatter) and the standing-room shell. Hearth mesh.
- The Estate level: the ruin set dressing on the landmark footprint, and the standing-room
  actors, which are Simulation structures seeded at a new game.
- `Homestead::Simulation`: new-game seeding of standing-room structures, the hearth as a
  cooking station, the `ManorFootprint` placement reservation, and name fields in the save.
- `SHomesteadMenu`: the Names step in the New game flow, save-list labels, and the diary note
  page.
- `HomesteadHUD`: the arrival title card.
- Tests: the new-game flow, the name persistence, the footprint rejection, and hearth cooking.
