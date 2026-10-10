# Design

## Rules (simulation, seeded per cast)
`FishingSession` gains `beats` (strikes needed, 2-4) and `escapeBeat` (-1, or the 1st/2nd strike at which the fish throws the hook, never the last). Waits stay seeded from the cast hash; nibble/tug times are a pure function of the seed and segment, so presentation and tests read the same times without stored arrays. A click while the float is up loses the fish ("Too soon – it shied away."); a missed window says "The fish slipped the hook."; an escape says "It got away." A test-only `SetFishingEscapeChance` seam (like `SetFishingWaterProbe`) lets tests force a keeper. A catch returns no message: the pack's "+1 <fish>" notice is the only catch message (Jenny, 2026-10-09).

Numbers are Balance's (balance.md section 8; Jenny confirmed 60% means the total failure rate): escape 50%, bite 4-12 s, 0-3 nibbles and 0-1 fake tug per strike wait (0.3 s shiver), hook window 0.8 s, 2-4 strikes 1.0-3.5 s apart, strike window 0.7 s. Named constants in `HomesteadFishing.h`.

## Presentation (approved by Jenny in PIE review, 2026-10-09)
- Controls: the left mouse button (RT) is the only fishing button. Esc/B open the menu as anywhere else (which reels in). The focus card shows only "[LMB] Cast line"; refusals are said once, by the click's notice.
- The zoomed umber `SHomesteadFishing` panel is removed.
- Camera: while fishing it eases to a high view (pitch -50, arm 6.2 m) over the midpoint of her and the float, and back to her own view afterwards. The room camera stays out while it's active (it fought it every frame and caused jitter), and the arm's collision probe is off.
- The float (`SM_FishingFloat`, `fishing_float.py`, short quill) lands 4.8 m straight ahead, angling off only where straight ahead isn't open water; shown at 2.8x, shrinking to true size as it nears her. The line ties to the quill top throughout.
- Cue: a bite or strike pulls the float under on a spring (eased down, bobbing back up when let go) while one gold ring closes on its spot across the reaction window. Nibbles shiver it with one faint ripple.
- The fish being caught swims in just under the surface, arriving 30-60% through the wait, circles, noses the float on nibbles and holds under it thrashing while hooked.
- The cast, fight, strike and catch clips are reused unchanged.