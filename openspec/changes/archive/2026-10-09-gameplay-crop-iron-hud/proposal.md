# Proposal

## Why

Jenny's Next-build cards: see a crop's name and day whenever she points at a growing plot; buy iron tools at the General Store until mining exists; read the minimap and compass landmark icons at 4K.

## What Changes

- Pointing at any planted plot always shows its card (`<Crop>: day X of Y`, plus water/weed notes), even with no action to offer.
- The General Store sells a worn-to-iron upgrade per crafted tool for 2,000 coins; the row disappears once the tool is iron.
- Compass tokens and minimap badges are about 1.6x larger.

## Capabilities

### New Capabilities

- `iron-tool-upgrades`: `Simulation::BuyToolUpgrade` and the store's Upgrades rows.

### Modified Capabilities

## Impact

New `HomesteadToolUpgrades.{h,cpp}`; rows in `SHomesteadShop`; `IsCropFocused` and the interact card in `HomesteadHUD`; sizes in `SHomesteadCompass`, `SHomesteadMinimap` and `CompassBox`. Tool tiers are already saved, so no save-format change.
