# Proposal

## Why

Jenny's feedback `jenny-mut5c51h-orhi39` asks each marked map/minimap location to unlock travel after its first visit, including Town.

## What Changes

- Track first visits to every currently marked place and show `Fast Travel Destination Unlocked: <Location>` once.
- Gate both map and signpost travel on saved discovery; preserve manor arrival and sensible initial manor access.
- Extend map travel beyond Manor/Town to Mine, Cove, Mill, Gateway and General Store.
- First playable result: walk to Town once, see the notice and then travel there; save/load retains unlocks. Jenny owns integrated acceptance; manor-arrival relocation is explicitly excluded.

## Capabilities

### New Capabilities

### Modified Capabilities

- `estate-map-and-minimap`: add persistent visit-gated travel.

## Impact

Reuse estate anchors, map glyphs, `PlanTravel`, `WalkRoad`, road routing, ordinary time advancement and controller ground-snap rollback. Custom gaps are discovery state, bounded arrival plans for non-road destinations, and complete glyph/action wiring. No new external assets or dependencies. A counted optional save section requires coordinator notification but no version bump.
