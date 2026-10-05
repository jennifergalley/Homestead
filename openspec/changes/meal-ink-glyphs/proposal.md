# Proposal

## Why

Jenny (2026-10-04, backlog:jenny-muuha81v-mpk0ja): the meal icons sit on grey photo tiles while tools are flat ink glyphs; "those grey photo tiles are awful and need to be removed ASAP."

## What Changes

- Draw the 11 shipped meals as original flat ink glyphs showing the finished dish, in the fish, pole and tool glyph style, filling the cell.
- Route those meals glyph-first wherever item icons draw (craft grid, hotbar, pack, chest, shop). Pasty and bread were already glyphs.
- Keep the original meal meshes and photo entries for world presentation; no item, recipe, save or price changes.

## Capabilities

### Modified Capabilities

- `compact-inventory-grid`: meal icons use ink glyphs, not photo tiles.

## Impact

`UI/SHomesteadIcon.*` only (append-only icon kinds, painter helpers, routing). Balance reviewed cohesion.
