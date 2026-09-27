# Design

## Context

- **Building today:** Simulation owns `Building` grids and `Structure` pieces (Foundation,
  Wall, Doorway, Roof, Fire, Bed, Chest). The Bed sleeps and saves, the Chest stores, and the
  Fire cooks.
- **Appearance:** the appearance page was just reworked (750a4397) into compact chips and
  swatches with a live preview.
- **Dependencies:**
  - The world lane publishes `ManorFootprint` and `StandingRoomSpawn`.
  - The clearing lane defines the salvage-pile resource kind and the rusted heads.

## Goals / Non-Goals

**Goals:**

- A memorable first view: a lived-in room inside a grand ruin.
- The room is made from the same building system the rebuild will use.
- Naming is quick and integrated into the flow that already exists.

**Non-Goals:**

- Dismantling, rebuilding, décor, story quests, cutscenes and voice.

## Decisions

### 1. The standing room is seeded Simulation structures, not level art

- At a new game, a free-standing `Building` is created at `StandingRoomSpawn`'s room origin.
  It holds:
  - foundations;
  - walls on the ruin-facing sides, with a new `StoneWall` skin of the Wall piece;
  - a Doorway;
  - Roof pieces;
  - a Hearth, a new Piece that behaves as Fire, always fuelled for round 1 so fuel isn't a
    chore;
  - a Bed;
  - a Chest, seeded with the pail and four Branches.
- Seeded structures are flagged `heritage`. They can't be removed in round 1, and round 5
  makes them dismantlable.
- **Alternative considered:** static level art with trigger volumes. Rejected, because the
  rebuild must treat this room as real structures, and the existing sleep, storage and cooking
  logic already works on pieces.

### 2. The ruin is level set dressing plus a reserved footprint

- The ruin meshes are placed in the Estate level on `ManorFootprint`. There are no Simulation
  records, except the salvage piles, which are placements.
- Simulation reserves the footprint polygon, and `CanBuildAt` rejects it with "The old manor
  stands here". The standing room sits inside a carved-out corner of the polygon.
- Round 5 replaces the set with dismantlable Simulation pieces. That's why the ruin kit is
  authored modularly: each wall run and pile is a separate mesh with a stable name.

### 3. Ruin kit (Blender procedural)

- **Recipes:**
  - Granite ashlar and rubble-stone wall runs in heights from 0.5 to 6 m, with broken tops.
  - Quoin corners, sash-window openings with granite lintels, and a doorway with a fallen
    lintel.
  - A tall chimney stack, charred and fallen roof timbers, and slate scatter.
  - Ivy and bramble clinging masks. Ivy is a vertex-colour foliage-card overlay built from the
    existing shrub library.
- **Materials** reuse `homestead_materials.granite` with lichen and soot variants. Slate is a
  new procedural material.
- Everything targets Nanite static meshes. Collision is simple boxes on wall runs.
- The ruin's footprint is about 30 × 18 m, two storeys implied by the surviving gable and
  chimney. It's visible from the road's approach and the cove.

### 4. New-game Names step

- The flow is New game → Appearance (the existing page) → **Names** → Begin.
- The Names page has three text fields with defaults:
  - First name: "Eleanor".
  - Surname: "Trelawney".
  - Estate: "Trevennor", a Cornish "tre-" farmstead name.
- Each field is limited to 24 characters and must be non-empty after trimming. There's no
  word filtering.
- Controller use: the fields open the platform virtual keyboard on focus. The page follows
  standard directional focus, B goes back, and Start begins.
- Names live in Simulation state (`heroineName`, `familyName`, `estateName`) and in the save
  header metadata for the save list.

### 5. Arrival beat

- The first frame after spawn shows a HUD title card: the estate name, then "Spring, 1851",
  in the existing serif display style. It fades in for 1 s, holds for 3 s and fades out, and it
  doesn't block input after 1 s.
- The field book gains a **Journal** entry list, which starts with one arrival note. That's
  the minimal version of a journal round 12 may grow. It lives under the existing Inventory or
  Map region rather than a new top-level tab, to limit tab sprawl.

### 6. Hearth cooking

- The Hearth reuses the Fire piece's cooking recipes and interaction. A warmth effect doesn't
  exist anymore.
- It emits a warm point light and has a crackle ambience loop.

## Lanes and ownership

This lane owns:

- The ruin kit and set, and the standing-room seeding.
- The Hearth piece, the `StoneWall` skin, and the footprint reservation.
- The Names page, the save labels, the title card and the journal note.

It reads:

- The landmarks, from the world lane.
- The salvage-pile kind, from the clearing lane.

This lane places the piles.

## Risks / Trade-offs

- **A ruin this big is a large Blender job.** → Ship a coarse massing pass first: wall runs,
  the chimney and rubble. Add windows, ivy and detail after the first playtest.
- **The virtual keyboard on Windows with a controller.** → Use Slate's editable text with the
  platform virtual keyboard, and verify it with the actual controller. If it's unreliable, add
  a simple on-screen letter grid fallback.
- **The year "1851".** It's the Great Exhibition year and sits squarely in the agreed range. It's
  cosmetic now, and Jenny can pick another year.

## Open Questions

- Default names are placeholders for Jenny to change: first name, family surname and estate.
