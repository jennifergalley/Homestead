# Proposal

## Why

Jenny selected crop-specific, non-stackable seed packets and an always-visible ready-crop harvest cue for the October 4 afternoon build.

## What Changes

- Present existing seed identities as Roots seeds, Turnip seeds, Carrot seeds and each remaining crop's own packets.
- Give each packet its own inventory/hotbar/chest slot and reject packet merging.
- Retain old quantities and identities when splitting saved seed groups into packets.
- Always show the crop-named E/A harvest action, except while visible weeds require removal first.

## Reuse research

The catalogue and crop table already have every required identity; Item::Seeds is roots only. Reuse inventory reconciliation, group ids, pack-square placement and crop rules. Fishing Art owns original packet icons and mapping; no new item enums or third-party assets are needed.

## Smallest useful result and first playable demonstration

Buy two carrot packets and a turnip packet: see three distinct slots, plant the selected crop, then harvest a ready weed-free plot after tutorial cues have retired. Integration and Jenny's acceptance remain separate.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `crop-growing`: crop-specific packet slots and persistent harvest affordance.

## Impact

Catalogue, reconciliation/merge/drop/save normalization, planting/harvest naming and focus-hint rules. Existing save schema/version and placements stay unchanged; compatibility normalization preserves all old counts and stable referenced group ids.
