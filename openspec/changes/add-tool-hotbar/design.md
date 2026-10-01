# Design

## Context

See `proposal.md` and `specs/tool-hotbar/spec.md`. Homestead currently owns four carried tools, original Slate tool icons, transactional tool checks, contextual work dispatch, a Canvas HUD, native Slate menus, one accepted input-intent classifier, and current save serialization. Mouse wheel adjusts camera distance, E/A performs context interaction, and F/X performs a second context action. No gameplay tool selection state or pointer-addressable gameplay bar exists.

The reference review establishes conventions rather than a visual target: Minecraft contributes sequential selection/wrap and dominant selected state; Factorio contributes ten `1-0` inventory references and ghosted unavailable shortcuts; Coral Island contributes selected farming-tool use; Dreamlight Valley contributes immediate numbered equip. Homestead retains its own Pine, cream, Gold, spacing, icons, typography, and quiet-HUD policy.

## Goals / Non-Goals

**Goals:**

- Make mouse/keyboard playtesting fast and obvious without weakening controller use.
- Introduce selected-tool semantics through one authoritative dispatch path.
- Avoid a second inventory, duplicated tool ownership, input click-through, or a permanent controls legend.
- Leave room for future assignment without building drag-and-drop now.

**Non-Goals:**

- General items/consumables, durability, multiple bars, manual assignment, radial menus, combat, charging, offhand, cooldowns, key rebinding, or new tools.

## Decisions

### 1. Use references to carried tools, not tool storage

Ten saved slot records hold item enum IDs or Empty; they never hold quantities or ownership. A new estate game seeds stable defaults in the order she hafts them (`AHomesteadController::ResetHotbar`): billhook, axe, scythe, pickaxe, hoe, pail, berries, then three empty slots. A slot resolves live against carried inventory. If the tool is absent or chest-stored, the slot renders ghosted and is unusable; it becomes live again when carried.

**Alternative considered:** move tools out of pack capacity into a separate belt. Rejected because it changes survival inventory balance and creates a second ownership system.

### 2. Add a small gameplay Slate hotbar overlay

Create `SHomesteadHotbar` as a pointer-enabled gameplay overlay so it can reuse `SHomesteadIcon`, native hit testing, selected/hovered styling, and responsive slot layout. Controller owns its immutable snapshot and selection; the widget never mutates Simulation directly. The Canvas HUD registers the bar's measured lower-center protected region for toast/context layout.

Each slot is square with a restrained Pine backing, small upper-corner number, centered original icon, muted ghost state, and Gold selected frame/2-3 pixel lift. Selection uses border/position plus color, not glow or copied frames. A transient selected-tool name may appear for roughly one second after a change; no permanent input instructions appear.

**Alternative considered:** hand-draw and hit-test ten Canvas slots. Rejected because the project already has tested Slate pointer/focus/input isolation and an original icon widget.

### 3. Separate interaction from selected-tool use

E/controller A remains context interaction: gather loose resources/forage, open storage, sleep, cook, and fill/interact where applicable. Left mouse/controller right trigger becomes `UseSelectedTool`, resolving the carried tool and current focus/cell:

- Billhook, scythe and pickaxe: clearing overgrowth of their kind at or above the tool's tier (`add-overgrown-estate-clearing`);
- Axe (the `Hatchet` item): sapling/tree clear/fell, stumps and fallen timber, and split-firewood only on an explicit eligible target;
- Hoe (the `DiggingStick` item): plot weed or forward-cell till;
- Pail (the `WateringCan` item): eligible plot water or stream refill.

Controller passes only existing action requests to Simulation. Success triggers current presentation; failure uses current feedback. A selected tool never broadens interaction range or fabricates a target.

**Garden target outline.** With the hoe or pail selected (in her pack's first row), `AHomesteadController::UpdateGardenOutline` draws a thin border on the ground round exactly the square the next stroke or pour acts on: the hoe's `Homestead::HoeCellAhead` 85 cm square (a till, or a weeding when the square is already a plot), or the pail's focused plot (hidden at the water, where it fills instead). `Homestead::PreviewGarden` checks it with the side-effect-free `Simulation::CheckTill` / `CheckWeed` / `CheckWater`, which `Till` / `Weed` / `Water` also call first, so the preview and the action can't disagree and previewing never mutates state or saves anything. Green means the action would succeed; red means it wouldn't, and a hoe with no other focus shows the refusal in the focus line in place of `Till ground`. `AHomesteadWorld::SetGardenOutline` builds 16 no-collision, shadowless, slightly emissive segments that follow sampled ground 2.5 cm up and rebuilds only when the square or its colour changes; it hides with no tool, in the book, shop, build planning or failure. The packaged Hotbar suite checks and captures green/red for both tools (`garden-outline-{hoe,pail}-{valid,invalid}.png`). For a real garden, the Development-only `AHomesteadGardenProbe` (`-HomesteadGardenProbe=<out dir>` with `-UserDir` pointing at a copy of her saves; never spawned in Shipping) loads the copy, records and captures the same four cases beside her own plots, checks the previews left plots and stock unchanged, pauses autosave in memory, and exits with `garden-probe.txt`.

**Seed outline and plant cue.** A seed (or berry) selected on the hotbar previews the [A]/[E] sowing the same way: `PreviewGarden(..., GardenTool::Seed, focusPlotId, seed)` outlines the focused plot, which is the plot `PlantFocusedPlot` sows, checked with the side-effect-free `Simulation::CheckSow` that `Plant` now calls first. It's green when the plot is tilled and bare, she's beside it, she isn't exhausted and she has that seed. Otherwise it's red, with Plant's own refusal: an occupied plot ("A crop is already growing here."), out of reach, too tired, or no seed left. With nothing in focus, the square the hoe would till next (`HoeCellAhead`, 85 cm) is red with "Till this square before sowing.", but only where `Simulation::CheckTillGround` (CheckTill's ground checks without the hoe or her energy) passes; over a building, resource, spoiling overgrowth or plot, nothing shows. A berry, which she more often holds to eat, outlines only a focused plot. Main has no crop seasons or withering yet, so neither refuses; add them to `CheckSow` when they land. The focus line on a bare plot comes from `Homestead::DescribeSow`:
- `[E]/[A] Plant <seed>` (keyed, so it retires after three uses like other hints) when the press would sow;
- the refusal, unkeyed, when a selected seed wouldn't sow;
- `Select <seed> (<key>) to plant`, unkeyed, naming a seed in the hotbar row and its number key (a berry only when no seed is there) when none is selected, since E never sows an unselected seed;
- otherwise `Choose seeds on the hotbar to sow`.

E's refusal toast uses the same text. The Hotbar suite captures `garden-outline-seed-{valid,invalid}.png` (Seeds before and after E sows the plot), and the native `SeedSowPreview` covers the valid/invalid matrix, the cue text, and that the preview's plot is the one `Plant` sows, with state unchanged by previewing.
**Alternative considered:** replace all context interaction with one selected-tool button. Rejected because chests, beds, forage, cooking, and other non-tool interactions remain clearer on E/A.

### 4. Reassign the mouse wheel deliberately

Unmodified wheel selects one slot per detent and wraps. Ctrl+wheel retains continuous camera-distance changes. Number keys `1-9/0` select directly; pointer click selects but consumes that click before world input. Gameplay LB/RB cycles; menus retain LB/RB tabs. Right trigger uses tool; R3 retains controller camera toggle.

Input ownership order remains automation isolation -> modal/menu -> planning/look -> hotbar pointer/selection -> world. This prevents click-through and makes mouse/keyboard co-primary.

### 5. Save world-specific state with bounded compatibility

Current-version saves add ten validated slot IDs and selected index. Invalid/duplicate/non-tool IDs sanitize to Empty; selected index clamps. Because test saves remain disposable, no elaborate multi-version migration is required, but load failures remain explicit and unrelated saves are not deleted. Hotbar is homestead state, not `GameUserSettings.ini`.

### 6. Integrate after the quiet HUD changes

The hotbar's lower-center footprint is measured against the planned lower-left vertical survival meters and target-gated lower-right context. Full-screen menus hide it. Planning may hide/dim it and owns numbers/wheel. The HUD lane and tool-dispatch/save lane can proceed separately after agreeing on the snapshot type; Editor/cook/package remain serialized.

## Risks / Trade-offs

- **[Ten slots feel empty with four tools]** -> Stable empty slots establish the promised `1-0` muscle memory and future room without fake content; review spacing at 720p.
- **[Wheel direction feels inverted]** -> Match common game convention, test both directions and wrap with Jenny's mouse during ordinary acceptance.
- **[Left click selects and uses simultaneously]** -> Slate consumes hotbar pointer down/up before world dispatch; native tests assert no same-click tool action.
- **[Selected tool changes existing E/F expectations]** -> Keep E/A interaction, add tool use separately, and update contextual prompts only near valid targets.
- **[Ctrl+wheel camera control is undiscoverable]** -> Settings may list the binding; gameplay adds no permanent instruction. Playtest the affordance rather than restoring HUD coaching.
- **[Hotbar/save state diverges from inventory]** -> Resolve references live for every draw/use and sanitize on load; never cache ownership.

## Migration Plan

1. Capture current gameplay input/HUD/save behavior and reference screenshots.
2. Deliver visual ten-slot selection with number/wheel/pointer/controller parity but no world mutation.
3. Add saved reference state and live carried/ghost resolution.
4. Route each existing tool through authoritative primary use and presentation.
5. Run keyboard/mouse-first ordinary routes, controller parity, menus/planning/load, 720p/4K layout, full-loop, save and performance acceptance.
6. Build one immutable Shipping candidate and retain `work-animation-complete-02-shipping / work-actions-v13` until explicit promotion.

