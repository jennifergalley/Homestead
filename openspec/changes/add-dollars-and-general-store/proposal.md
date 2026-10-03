# Proposal

## Why

Jenny selected crop sales for measured build `20261002-measured-01`: harvest produce, take it to Pascoe's, and receive coins. The six period crops already have store buyers; legacy grown roots and berries do not.

## What Changes

- Let the existing General Store buy every currently growable crop's produce at its existing catalogue price.
- Reuse `Simulation::Sell/Buy`, the catalogue buyer mask, `SHomesteadShop` sell list, and existing economy persistence. No new shop, asset, dependency, or price scheme is needed; repository components require no new third-party license/access.
- First playable result: harvest, sell part or all of a stack, and see the correct coin balance. Broader economy and new crops are deferred.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `crop-growing`: explicitly include legacy roots and berries in General Store crop sales.

## Impact

`HomesteadItems.cpp` buyer metadata and native economy regression coverage. Existing shop UI consumes that metadata automatically. Preserve coins (not the stale dollars wording in config), buy/buy-back, closures, and save format; UI theme/map/pack-slot ownership remains with UI Agent.
