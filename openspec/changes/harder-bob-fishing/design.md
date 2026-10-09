# Design

## Rules (simulation, seeded per cast)
`FishingSession` gains `beats` (strikes needed, 2-4), `escapeBeat` (-1, or the strike at which the fish throws the hook). Waits stay seeded from the cast hash; nibble/tug times are a pure function of the seed and segment, so presentation and tests read the same times without stored arrays. An early click (any time the float isn't under) loses the fish, as before. When the fight reaches `escapeBeat`'s strike time the cast ends with "It got away." and no reward. A test-only `SetFishingEscapeChance` seam (like `SetFishingWaterProbe`) lets tests force a keeper; the default is Jenny's 60%.

Numbers are agreed with Balance and live as named constants in `HomesteadFishing.h`.

## Presentation
- `SHomesteadFishing` (the zoomed bank scene) is removed; the cancel hint moves to the focus line.
- The float is a visible painted cork float on the water, sized for the gameplay camera. Waiting: lazy bob; nibble: shiver plus a small outgoing ripple; bite/strike ready: pulled under (hidden) with collapsing gilt rings over the reaction window; fight rest: back on the surface, towed and trembling; escape/miss: an outgoing splash ring and the existing Miss reel-in.
- Rings are drawn by a HUD Slate leaf that projects world circles at the float's water height, so they sit on the water in perspective without new materials. Nothing scales with placements; it only paints while fishing.
- The cast, fight, strike and catch clips are reused unchanged.

## Risks
Waits are long enough that idle wobble must read as "not yet": nibbles are deliberately small and short against the full sink.
