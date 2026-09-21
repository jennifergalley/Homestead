# Proposal

## Why

Jenny likes the delivered menu but cannot move naturally between its sections
with the D-pad or left stick. Requiring triggers to leave a grid makes ordinary
inventory work feel disconnected.

## What Changes

- Retain directional grid/list movement; cross visible section boundaries in
  the requested direction, including carried items down to equipment and back.
- Give semantic selection real Slate focus, with focus-following scrolling and
  stable item IDs across refreshes. Focus movement never activates a control.
- Route left-stick, D-pad and keyboard directions through the same behavior.
  Keep LB/RB tab shortcuts and optional triggers, but stop advertising triggers
  as the required region-navigation mechanism.
- Keep modal focus trapped; make quantity editing an explicit entered mode,
  distinct from moving among dialog controls.
- Preserve visuals, mouse behavior, the sole input-intent classifier, world
  controls, transactions and the three-press Settings exit path.

## Capabilities

### New Capabilities

- `menu-directional-navigation`: Spatial directional movement between visible
  controls/sections, consistent focus and explicit modal editing.

### Modified Capabilities

None in the main spec inventory (currently empty). This round refines the
in-flight `homestead-menu-overlay` capability without rewriting its old tasks.

## Impact

Owned targets: `UI\HomesteadMenuNavigation.h`, `SHomesteadMenu.*`, focused UI/
portable tests, and directly related navigation guidance. Controller input is
changed only if necessary; world/camera/authority/assets/common QA are excluded.
Relevant UI/controller files match committed main `a8dee30a053c3bcbc34ee45441b1220b42f2d5ac`.
Main owns engine execution and integration after its live endurance boundary.

## Reuse and first playable result

Local inspection found Slate `FNavigationMetaData`, `FNavigationReply`,
`FSlateApplication` focus/navigation routing and `SScrollBox` focus scrolling.
Reuse these installed Unreal facilities and the existing portable grid helper,
not CommonUI or a new focus framework. No external package, asset or license
admission is needed. Custom work is limited to bridging semantic item selection,
logical offscreen grid edges and the existing accepted-input path to Slate focus.

First demonstration: open Inventory, move down through the last carried row
into equipment using only D-pad/left stick, reverse up, then move sideways to
details/actions and portrait controls. Full acceptance also covers empty/full/
short grids, dialogs, focus restoration and both input devices in the real game.
