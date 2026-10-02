# Round 1 kickoff: "Walk your estate"

Jenny gave the go on 2026-09-27 at 10:54 (Arizona). This file is the handoff to the implementing coordinator session. It stands in for a cross-session message.

## Read first

- `docs\game-plan.md`: the direction, the working policy, and the lessons for every feature round.
- `openspec\changes\pivot-to-cozy-estate-life-sim\design.md`: the authoritative decisions. Its spec, `estate-life-sim-direction`, sets the product rules.
- `docs\research\estate-pivot\first-slice-reuse.md`: the reuse research. It flags the claims it couldn't verify.

## The five round-1 changes

Each change is fully specified: proposal, design, spec and tasks.

1. **`author-fixed-cornish-estate-map`** (the hub lane)
   - Build the terrain from Environment Agency LIDAR at 1 m for the St Agnes, Trevaunance and Chapel Porth coast. Convert it with GDAL into a 4033² Landscape in a new World Partition level, `Estate`.
   - Use the Water plugin. Prove it early; the fallback is Single Layer Water.
   - Add the road spline and the PCG biomes.
   - Publish version 1 of `DA_EstateLandmarks` early, since the other lanes depend on its anchors.
   - Bake `DA_EstatePlacements` with stable IDs.
   - Add a fixed-world mode to `AHomesteadWorld`.
2. **`add-estate-boundary-map-and-minimap`**
   - Parcels in Simulation.
   - A baked orthographic map texture.
   - The `SHomesteadMinimap` HUD overlay and a Map tab in the field book.
   - A toast when she crosses the boundary.
   - A build-ownership check.
3. **`add-overgrown-estate-clearing`**
   - Overgrowth kinds, each with a required tool and minimum tier.
   - The scythe, billhook and pickaxe; the axe also takes stumps.
   - A Haft recipe that fits salvaged rusted heads to handles.
   - Slow weed creep on untended ground.
   - Remove warmth, the knife, the machete and the fibre paths.
4. **`add-ruined-manor-and-arrival`**
   - Merge and import the stone kit from `jennifergalley-stone-building-and-torch-assets` (commit d0a2a73c). Its status is in `docs\stone-kit-and-torches-status.md` on that branch.
   - The heritage standing room, with a bed, hearth and chest.
   - The ruin massing and the salvage piles.
   - A Names step in the new-game flow.
   - The arrival title card and the first journal note.
5. **`add-dollars-and-general-store`**
   - The `HomesteadItems` catalogue.
   - Money stored in cents, and a wallet on the HUD.
   - The town blockout and the general store.
   - The shopkeeper, with a labelled stand-in body until her MetaHuman is authored.
   - The `SHomesteadShop` screen.
   - Her sold goods listed as "From {Estate}", selling down each day.

## How to run it

- Follow the "Lanes and ownership" section of each design, and use worktrees and sub-sessions in parallel wherever ownership allows.
- Serialize anything shared: the item catalogue file, the save-version bump (bump it once, at integration), the `SHomesteadMenu` edits, and engine packaging.
- Deliver section 2 of each change's tasks as a playable increment. For each one, package to `Build\Windows`, commit, push to `main`, and report what Jenny can try.
- Keep the OpenSpec checkboxes truthful.

## Human steps

- The shopkeeper's MetaHuman needs Jenny's Epic sign-in for its cloud steps. Ask her when you reach that point, and keep the other work going in the meantime.
- Someone needs to confirm the current MetaHuman licence text.
- Make no purchases and create no accounts.

## Saves and placeholders

- Test saves reset when the fixed map lands. Report that plainly.
- These placeholders stay until Jenny changes them: Eleanor Trelawney, the Trevennor estate, the year 1851, Mrs. Martha Pascoe, and $10.00 starting money.

## After round 1

Autopilot through round 1 to its integrated acceptance. Then flesh out round 2, `rework-farming-calendar-and-period-crafting`, the same way before implementing it. Salvage the night-lighting and wake-up work from `wip/foliage-night-dawn-wake` there.
