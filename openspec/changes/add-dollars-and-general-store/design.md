# Design

## Context

See proposal.md. `CropTable` has eight crop kinds; the six period produce rows already use `StoreBuys`, while roots and berries use `NoBuyers`. Shop rules and the Sell list share `ShopBuys`; existing `CommitInventory` reconciles layouts atomically.

## Goals / Non-Goals

**Goals:** complete crop selling through the existing catalogue and transactional simulation.
**Non-Goals:** new economy, new item IDs, distinguishing wild from grown stacks, save changes, map/inventory/theme edits.

## Decisions

- Mark roots and berries `StoreBuys`, retaining prices 4 and 6 coins. Because stacks do not track provenance, this also accepts gathered roots/berries; adding provenance is unnecessary for Jenny's slice.
- Exercise every crop's actual `HarvestCrop` result through partial/full sale, buy-back, rejection and save/reload in native economy tests, rather than testing only granted stacks.
- Town owns item buyer metadata, shop UI and economy tests. Extract the existing Sell-row eligibility loop into `ShopSellableItems`, consumed by `SHomesteadShop::BuildRows` and the native harvest regression; no new crop-only list or UI feature. UI Agent owns palette and pack-slot changes; the shop consumes its current palette helpers.
- Reuse current build/test scripts and caches, with one editor compile and no package or new dependencies.

## Risks / Trade-offs

Legacy crop sales also enable wild produce sales -> same items and existing prices, no duplicate commodity or serialization changes.
Native tests cannot establish ordinary gameplay appearance -> Jenny performs integrated playtest acceptance after Integration ships.
