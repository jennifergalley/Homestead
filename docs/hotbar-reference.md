# Tool hotbar reference

Homestead uses familiar toolbelt conventions without copying another game's
art, layout, sounds, type, frames, glow, or branded expression.

- Minecraft: numbered bottom slots, number selection, wheel cycling, wrapping,
  and a dominant selected slot. Sources: [controls](https://minecraft.wiki/w/Controls)
  and [inventory](https://minecraft.wiki/w/Inventory).
- Factorio: quickbar slots are references into inventory, not extra storage;
  unavailable references remain visible but unusable. Sources:
  [quickbar](https://wiki.factorio.com/Quickbar) and
  [Friday Facts 278](https://factorio.com/blog/post/fff-278).
- Coral Island: compact farming-tool slots, number/wheel selection, and selected
  tool use in ordinary character play. Sources:
  [Steam screenshots](https://store.steampowered.com/app/1158160/Coral_Island/)
  and [inventory reference](https://coralisland.fandom.com/wiki/Inventory).
- Disney Dreamlight Valley: immediate numbered tool equip and a compact
  character-focused tool surface. Sources:
  [controls](https://dreamlightvalleywiki.com/Controls) and
  [official screenshots](https://disneydreamlightvalley.com/en/media/screenshots).

No external screenshot is stored in this repository.

## Selected-build baseline

`foliage-camera-v15` has no gameplay selection state or hotbar. Mouse wheel
directly changes spring-arm distance; number keys and left mouse have no tool
binding. E/controller A performs context interaction, F/controller X performs
secondary clearing/weeding/tilling, gameplay LB/RB opens or changes field-book
pages, and R3 toggles camera distance. The starter Knife and crafted Hatchet,
Digging Stick, and Watering Can occupy ordinary pack capacity and persist only
through Simulation inventory. Save schema 6 has no shortcut references.

At 1280x720 and 3840x2160, the lower-left needs panel and lower-right context
leave a lower-center lane of roughly 650 by 80 virtual pixels. The first hotbar
uses that protected lane, ten 60-pixel slots, Homestead's Pine/cream/Gold
palette, and existing original tool icons.
