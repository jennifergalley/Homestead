---
name: homestead-test-tools
description: The one-stop kit for verifying a Homestead (SurvivalGame) feature in Play-In-Editor - read the whole game state in one call, go to a named place or target without walking, use or interact with it, select hotbar items, open and read UI screens, set time of day and weather, reset to a known state, sell at the General Store, and take screenshots. Recipe-first (verify a gatherable, a shop sale, darkness and lighting, a UI panel, a tool action). Use whenever you test a gameplay or UI feature in the editor, before you try walking or aiming by hand.
---

# Homestead test tools

All of these run in a PIE session you already started (see `unreal-editor-mcp` for launching the editor,
ports and the machine limits). Dot-source the helpers once: `. .\Scripts\McpHelpers.ps1 -Port <your port>`.
Every command below is a PowerShell helper wrapping one console Exec on `AHomesteadController`
(`HomesteadControllerTest*.cpp`), so they also work as `con "<Exec> ..."` and in Development packages
(use the in-game console, `~`).

**Rule of thumb: don't walk, don't aim, don't guess. Teleport, read state, act, read state again.**
Results of the Execs go to the log category `LogHomesteadPlaytest`; `plog` prints them.

## The commands

| Helper | Exec | What it does |
| --- | --- | --- |
| `state [n]` | (get_play_state `world`) | Whole game picture in one call, see "Reading state". `st` is the raw play state (also has `hotbar`, `nearbyResources`). |
| `goto <target>` | `HomesteadGoto` | Teleports her onto the ground (or the tunnel floor) next to the target, facing it. **Waits and returns the `GOTO:` lines**: `arrived at (x, y, z ...) ... Focus: ...`, or `FAILED` / `refused` / `no target`. |
| `use "<target> [item]"` | `HomesteadUse` | `goto`, settle, then `Interact()` (no item) or select the item on the hotbar and use it. Waits and returns the `USE:` lines ending with the outcome (toast, pack, energy, focus) or the failure. |
| `hotbar [item]` | (key press) | Prints the bar; `hotbar OilLamp` / `hotbar Pickaxe` presses that item's number key. |
| `screen <name>` | `HomesteadScreen` | `close`, `inventory`, `craft`, `build`, `settings`, `appearance`, `map`, `shop`, `chest`. Logs `SCREEN:` with what it shows. |
| `weather rain\|clear` | `HomesteadWeather` | Skips the clock forward to a rain spell (inside it) or a clear dry hour; it can jump days (rain skipped ~6.5 days in testing), so set the weather last. Logs `WEATHER:`. |
| `reset` | `HomesteadReset` | Full energy, full lamp oil, screens and held action closed, morning if it is night. Pack, purse and placements stay. |
| `sellat "Name N,Name N"` | `HomesteadSellAtStore` | Opens the General Store's hours, stands her at its real counter, sells the list through the real sell path, opens the shop. Waits and returns the `SOLD` lines, the purse delta and `SELLAT: shop screen open=`. |
| `plog [n] [pattern]` | | Last `n` `LogHomesteadPlaytest` lines of this worktree's editor log. |
| `tp x y [z]` | `HomesteadTeleport` | Raw teleport (all of the above are built on it). |
| `con "HomesteadGive Item N"` | `HomesteadGive` | Adds items (name by item key, e.g. `Coal`, `ScrapIron`, `OilLamp`). |
| `con "HomesteadMorning h"` | `HomesteadMorning` | Jumps the clock forward to hour `h` (forward only; rolling past midnight starts the next day and can change the weather). |
| `con "HomesteadEnergy n"`, `con "HomesteadLampOil h"` | | Set energy / lamp oil. |
| `con "HomesteadPackMenu 0 2"` | | Opens the field book at page 0 with the chest sub-view (`screen chest` is easier). |
| `shot` / `hshot` | | Screenshots: `shot` shows Slate/UI, `hshot` is the 3D view only. |

### `goto` / `use` targets

- Landmarks: `store` (General Store door), `counter` (inside, in front of the shop counter), `cove`/`beach`,
  `square`/`town`, `gate`, `mill`, `farm`, `home`/`spawn` (standing room); any landmark name or part of one.
- The mine: `mine` (also `mine-house`, `mine-door`) stands her in the **yard in front of the mine house door**, facing it
  (not inside the house); `mine-ladder` (also `underground`, `tunnel`, `ladder-foot`) puts her **underground** on the tunnel
  floor at the ladder foot. Both work on every layout: they read the door and ladder foot from `Mine::GetLayout()`.
- Underground nodes: `node:588011` (any mine node id from `state.nearby` or the layout) stands her on the tunnel floor in
  front of that face, looking at it, at the tunnel's own height; `coal`, `copper`, `gem`, `rock face` and the like resolve
  to the nearest node by true 3D distance, so a face under the yard is not "2 m away".
- Raw point: `-9570,-36300` (no spaces). Surface only: a point inside a building, or where the landscape under it
  is holed, may never settle (see Gotchas). `goto` then reports `FAILED`, not `arrived`.
- Ids exactly as `state` lists them: `node:520001` (also `resource:`), `structure:N` / `piece:N`, `plot:N`, `drop:N`.
  A bare number searches resources, structures, plots, then drops.
- Nearest of a kind: any resource name (`Stone`, `Tree`, `Hay`...), piece name (`Chest`, `Bed`...), `plot` or `drop`.
  "Nearest" is measured from where she stands now.
- `use "node:520001 Pickaxe"` = go there, select the pickaxe, swing. `use "Hay"` = go to the nearest hay and press interact.

## Reading state

`state` returns the `world` object from `get_play_state` (also under `(st).world`):

| Key | Contents |
| --- | --- |
| `time`, `weather` | Date, weekday, season, hour, night flag; rain / overcast / wetness. |
| `energy`, `wellFed`, `purse` | Numbers. |
| `pack`, `packSummary` | Every item and count. |
| `hotbar[]`, `selectedHotbarIndex` | Index, key, item, count, selected, lamp fill. |
| `toolTiers`, `lamp`, `equipment` | Axe/Hoe/Pail/... tier; lamp carried / in hand / lit / oil hours; worn clothes. |
| `position` | `x`, `y`, `z`, `facingYaw`, `underground`, `level` (`surface`/`underground`), `sheltered`, `nearFire`, `nearWater`, `nearestLandmark`, `worldReady`, `groundSnapPending` (+ `groundSnapPendingSeconds`). |
| `focus` | What she would interact with now, and its prompt. |
| `nearby[]` | Closest things with `kind`, `id`, `name`, `distanceCm`, `level`, `bearingDeg`, `x`, `y`, `note`; `nearbyTotalInRadius` says if the list was cut. Same-level distances are 2D; across levels (yard vs tunnel) the height counts. |
| `generalStore` | Open now, hours, door and counter positions. |
| `screen` | `open` is `none`, `shop`, `chest`, `book:<page>` or `placement`, plus what is on it (shop rows with prices, chest contents). |
| `toast`, `gamepadPrompts` | Last toast; whether prompts show gamepad (the last device used). |

Use `nearby` ids with `goto`/`use`. `nearbyResources` in `st` only covers streamed chunks; `nearby` is read from the simulation.

## Recipes

### Verify a new gatherable or tool action

1. `reset` (energy, lamp, clean screens). Give her what the action needs with `con "HomesteadGive <Item> 1"`.
2. `state` and find your thing in `nearby` (or know its id/kind from your placement). Note pack and energy.
3. `use "<id or kind> <tool>"` then `plog 8`. Expect `USE:` with a toast, the pack delta and energy cost.
4. `state` again; compare pack, energy, `nearby` (the node should be depleted or gone) and `focus`.
5. `shot` if the visuals matter. For animation timing, `hshot` right after the `use`.

A standing tree is felled by the axe, not by interact; a depleted node can steal focus, so after a clear
check `focus` before the next interaction; the axe has a ~3.5 s recovery.

### Verify a shop sale

1. `con "HomesteadGive Coal 3"` (item keys are the names in `state.pack`).
2. `sellat "Coal 3"` then `plog 10`: per-item prices, purse delta, `SELLAT: shop screen open=1`.
3. `shot` for the shop screen; `(state).screen` lists the rows. Compare prices against `docs\general-store-playtesting.md`.
4. Closed-store behaviour: `con "HomesteadMorning 3"` and `screen shop` (toast only). `HomesteadOpenStore` drops a dev store at her feet instead; use it only to test the shop UI itself.

### Check darkness, lighting or lamp

1. `con "HomesteadMorning 22"` (night; adds a day if already past) or `HomesteadMorning 12` for noon.
2. `goto mine-ladder` for underground (or `goto node:<id>` at a tunnel face), `goto home` for indoors. `(state).position.underground` / `.level` confirm where she is.
3. `con "HomesteadGive OilLamp 1"`, `hotbar OilLamp` to hold it, `con "HomesteadLampOil 6"` for full oil. `(state).lamp` shows carried / inHand / lit / oilHours.
4. `hshot` for the 3D view. Compare with and without the lamp, and day vs night.

### Check weather

`weather rain` then `state` (`weather`, wetness) and `hshot`; `weather clear` to return. Rolling the clock to the next day can re-roll the weather, so set the weather last.

### Check a UI panel

1. `screen <name>`; `(state).screen` says what is open and what it lists. `plog 3` shows the `SCREEN:` line.
2. `shot` (not `hshot`: it misses Slate). `screen close` when done.
3. Input device matters: prompts follow the last device used (`gamepadPrompts`), and mixing devices silently drops actions. A new game opens the field book first; `screen close` closes it.
4. Crafting is press-and-hold; use the `unreal-editor-mcp` input tools for that, not `screen`.

### Start every test from a known state

`reset`, then `goto <where>`, then `state`. A fully clean world means moving `Saved\SaveGames\*` aside and restarting PIE (action hints retire after 3 uses, in `GameUserSettings.ini` `[Homestead.ActionHints]`).

## Known coordinates

Standing room ~(-25750,-63800); General Store counter (-9570,-36300); manor bedroll ~(-25500,-63720);
salvage piles `520001`-`520005`, branch piles `500001`-`500003`, stump `510091`, fallen log `510092`.
Prefer `goto <name or id>` over these numbers.

## Gotchas

- `HomesteadTeleport` and its wrappers refuse while a ground snap is pending. `goto`/`use`/`sellat` now log `refused: still being placed ... (pending for N s)` instead of silently doing nothing; Interact and hotbar keys pressed during a snap are dropped with only the "Still finding your footing" toast, so wait for `arrived` first.
- Underground work needs light at the face: `use "node:N Pickaxe"` returns `Too dark to work` (toast) while the lamp is not lit. Set the lamp down first with `use "node:N OilLamp"` (it stays lit on the floor beside her), then swing the pickaxe; a worn pickaxe takes 3 swings on a rock face (verified: Stone +5, then `Nothing left here until it comes back`). `state.lamp` shows `lit`.
- `goto` to a surface point where the settle never completes (inside the mine house) takes the full 90 s and then logs `FAILED: ended at (...)`; the wrapper returns that line. Don't wait on it twice: stand in the yard (`goto mine`) and walk in.
- `goto` waits up to 90 s for the area to stream in. If the snap is abandoned (she is put back where she started) it logs `FAILED: ended at ... not the target` rather than `arrived`; never trust a `goto` whose result line you did not read.
- The teleport settle needs both floor collision and landscape collision (underground it needs only the tunnel floor). A target inside a surface building with a holed landscape never settles (`HOMESTEAD_GROUND_HOLD floor=1 landscape=0` in the log): stand outside (`goto mine`) and walk in, until the settle treats that footprint like the sea cave.
- Don't use `walk_to` for testing: it stops ~150 cm short and sticks on walls, doors and trees, and an open field book blocks it.
- Tool swings are dropped while she moves; `use` settles first. `UseSelectedTool` needs the hotbar showing; `Interact` needs the world ready (`state` works only after `worldReady`).
- If `con` reports nothing, PIE isn't running (`pie`, then poll `st`).
- Read the result in the lines the wrapper returns (`goto`, `use`, `sellat` wait for them); `plog [n] [pattern]` shows older lines. Other wrappers return the command string they ran, and their results are in the log.
- The Execs exist only once the editor module with `HomesteadControllerTest*.cpp` is built and the editor restarted (Live Coding can't add Exec declarations to the header).

## Adding to the kit

New tools go in `Source\SurvivalGame\HomesteadControllerTest<Area>.cpp`, log to `LogHomesteadPlaytest` (`HomesteadPlaytestSupport.h`),
reuse `HomesteadTeleport` / the item-key lookup / the real simulation path, and get a wrapper in `Scripts\McpHelpers.ps1`
and a row here. Add what you had to fumble to find.
