# Proposal

## Why

Repeated tool work needs a visible, immediate selection model instead of relying on hidden contextual E/F meanings or reopening inventory. Jenny expects the familiar bottom toolbelt convention and is likely to use mouse and keyboard heavily during playtesting, so `1-0`, mouse-wheel selection, pointer access, and left-click tool use must be first-class rather than controller afterthoughts.

## What Changes

- Add a quiet ten-slot bottom-center toolbelt labeled `1 2 3 4 5 6 7 8 9 0`, using Homestead's own Pine/cream/Gold visual language.
- Treat slots as shortcuts to carried tools, not extra inventory capacity. The first delivery auto-assigns the current Knife, Hatchet, Digging Stick, and Watering Can in stable order; unowned/stored tools appear absent or ghosted and cannot be used.
- Make the selected slot's carried tool the active world tool. Unmodified mouse wheel and number keys select slots; clicking a slot selects it.
- Add one primary tool-use path on left mouse and controller right trigger while retaining E/controller A for ordinary context interaction such as gathering, chest, bed, fire, and water.
- Route active-tool work through existing authoritative transactions: Hatchet clears/fells, Digging Stick tills/weeds, Watering Can waters/fills where valid, and Knife performs its admitted clearing role. Selection never grants an item or bypasses range, target, inventory, capacity, water, or plot rules.
- Reassign gameplay camera-distance wheel control to Ctrl+wheel while preserving controller R3 distance toggle; menu/Look/planning input ownership remains contextual and explicit.
- Keep LB/RB as gameplay toolbelt cycling and menu tab switching according to current UI state. Preserve focus/device classification and prevent tool use through open menus, planning, failure, load, or appearance preview.
- Persist slot assignment/selected slot with the homestead save when introduced, without turning it into a global user setting; current test saves may reset under the project's disposable-save policy.
- Build ordinary mouse/keyboard acceptance alongside controller parity, including scroll direction/wrap, number `0`, pointer clicks, left-click use, camera-zoom modifier, and no accidental world action under UI.

Reference review:

- Minecraft establishes bottom-center sequential slots, number selection, wheel cycling, strong selected-slot treatment, stack overlays, and selected-item use ([Minecraft Wiki controls](https://minecraft.wiki/w/Controls), [inventory](https://minecraft.wiki/w/Inventory)).
- Factorio establishes ten `1-0` quick slots as references into inventory rather than hidden extra storage, plus ghosted unavailable references ([official Factorio Wiki quickbar](https://wiki.factorio.com/Quickbar), [Friday Facts 278](https://factorio.com/blog/post/fff-278)).
- Coral Island establishes a ten-slot farming toolbelt, number/wheel selection, and selected-tool world use visible in ordinary play ([Steam screenshots](https://store.steampowered.com/app/1158160/Coral_Island/), [community inventory reference](https://coralisland.fandom.com/wiki/Inventory)).
- Disney Dreamlight Valley reinforces immediate numbered tool equip and compact selected-tool clarity for mouse/keyboard, with a separate optional wheel rather than relying on it ([controls reference](https://dreamlightvalleywiki.com/Controls), [official screenshots](https://disneydreamlightvalley.com/en/media/screenshots)).

These references contribute interaction conventions and information hierarchy only. Homestead MUST NOT copy proprietary icons, frames, textures, typography, glows, sounds, exact geometry, or branded artwork. No external asset is required: existing original tool icons, Slate primitives, HUD colors, inventory authority, and input facilities cover the first delivery.

The smallest useful in-game result shows owned tools in a ten-slot bar and lets `1-0`, wheel, and pointer selection visibly agree. The first playable demonstration selects the Hatchet with `2`, left-clicks a focused tree for one authoritative fell, selects the Digging Stick by wheel to till, and uses Ctrl+wheel for camera distance without changing tool selection. Full acceptance covers save/reload, tool moved to chest/returned, all current tools/actions, keyboard/mouse and controller, menu/planning isolation, 720p/4K layout, HUD coexistence, and immutable Shipping replay.

Deferred scope includes arbitrary inventory stacks/consumables on the toolbelt, drag-and-drop reassignment, multiple bars/pages, offhand use, durability, cooldown overlays, combat, charged tool use, key rebinding, radial wheel, and new tool content.

## Capabilities

### New Capabilities

- `tool-hotbar`: Ten-slot carried-tool shortcut bar, selection inputs, selected-tool world use, state isolation, and persistence.

### Modified Capabilities

None.

## Impact

- `HomesteadController`, `HomesteadCharacter`, input mappings, Simulation query/use dispatch, save object/version, and native/full-loop tests: selected tool state and authoritative tool-use routing.
- New focused `SHomesteadHotbar` gameplay overlay plus existing `SHomesteadIcon`: pointer-selectable slots, numbering, owned/ghosted/selected state, responsive layout, and focus/device isolation.
- `HomesteadHUD`: protected lower-center region coordinated with lower-left need meters and lower-right contextual feedback.
- `PRODUCT.md`, `DESIGN.md`, setup/input and visual-playtest documentation: mouse/keyboard becomes co-primary for playtesting, and old controller-first language is updated when implementation lands.
- No external package, asset, account, purchase, download, or license is required.

