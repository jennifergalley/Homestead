# Jenny's playtest feedback

## Current priority

Character appearance and robotic movement need an observed gameplay diagnosis
first. MetaHuman is not a settled next step. Use the current build's normal
movement and interaction recordings as a basis for improvements.

## Wild gathering - first motion slice (2026-09-20)

The separately packaged `gathering-01` adds a short restrained reach/pick/recover
after successful wild gathering, with movement interruption and clean recovery.
Actual ordinary berry gathering now visibly changes pose instead of remaining
static. This is technical/action-presentation progress, not Jenny's approval:
the gesture is generic, exact fingertip contact is not proven, and rigid hair,
fixed face and close-fitting clothes remain. See `docs\visual-playtesting.md`
for the two-pass review and packaged lifecycle/full-loop evidence.

No further cosmetic experiment was folded into this slice. Both requested
mid-back wavy length and the blonde-bob redesign remain unmet/deferred, as do
the UI/rendering feedback entries below.

## Watering - first motion slice (2026-09-20)

The separately scheduled `watering-01` slice adds one contextual can lift/tilt/
recover after a successful watering transaction. Its fresh-start ordinary route
and packaged checks are documented in `docs\visual-playtesting.md`. The can is
original wood/fiber presentation matching the existing recipe, not a new tool
or inventory rule. This does not complete the other interaction gestures or
resolve any deferred cosmetic/UI feedback.

## Weeding - reused motion feedback (2026-09-20)

The separately scheduled `weeding-01` slice reuses the accepted picking clip
after successful planted-plot X/F weeding. Actual walking and interaction were
recorded from a clearly disclosed copy of an existing test-world save; the
prior setup used functional teleports and ordinary sleep to grow weeds.
The final footage shows a gentle dip/pull and return to idle rather than a
static transaction. No new clip, prop, appearance edit or gardening rule was
added. This is generic feedback above the plot, not hand-to-ground contact:
weeds disappear at the existing immediate transaction. It is not Jenny's
approval or a fix for the remaining cosmetic/UI requests.

## Sapling clearing - contextual hatchet feedback (2026-09-20)

The separate `clearing-01` slice adds one restrained lift/swing/recover and a
small original wood/stone/fiber hatchet only after successful permanent sapling
clearing. Fresh ordinary controls gathered supplies, crafted the existing
hatchet and approached/cleared an actual sapling. The final full-body recording
shows the tool arc and recovery without gross new clipping. This remains a
generic gesture after immediate sapling disappearance, not synchronized impact,
realistic tree felling, combat or exact hand/tool contact. Other clearing/input
contexts and all gameplay costs/rewards remain unchanged. Existing cosmetic/UI
feedback is still unresolved; this is not Jenny's aesthetic approval.

## Wavy length - attempted and reverted (2026-09-20)

Jenny's wavy-hair length feedback was scheduled as its own bounded slice:
"the wavy hairstyle is too long - I'm thinking more the middle of her back."
Only the existing long-wave length across all three bodies and both outfits is
in scope. The packaged trial shortened the tips by about 22cm, but compressed
the lower waves into obvious accordion-like ridges and a blunt/frayed shelf.
It failed the final visual gate despite passing technical/functional checks.
The coordinator independently inspected the comparison and agreed to recovery.

**Not completed: the original overlong hair is restored, and the mid-back
request remains unmet.** No third sculpt/review pass was attempted. Only useful
back/three-quarter diagnostics are retained; brunette color, crown/framing,
other styles, face, materials, rig and locomotion remain at the accepted baseline.
The blonde-bob redesign and all other deferred entries below remain deferred.
See `docs\visual-playtesting.md` for rejection and recovery evidence.

## Deferred feedback - 2026-09-19

Jenny explicitly marked the following as feedback for later, not an instruction
to change these systems immediately.

| Area | Report | Follow-up when scheduled |
| --- | --- | --- |
| Rendering | Screen tearing / horizontal flickers while moving. | Reproduce and capture the artifact; distinguish presentation tearing from temporal/rendering artifacts before choosing a fix. Do not assume a VSync diagnosis from the description alone. |
| Input prompts | Interaction labels revert to keyboard/mouse even while playing with a controller. | Check device-switching thresholds and incidental input events; retain controller prompts during genuine controller play. |
| Music and ambience | "Great." | Preserve this as a successful baseline; avoid unnecessary replacement or remixing during unrelated work. |
| Field book / recipes | The book feels overwhelming; learning recipes gradually might be better than exposing all recipes immediately. | Explore progressive disclosure or a learn/unlock flow. This is a proposal, not a confirmed progression design; essential opening-survival actions must remain attainable. |
| Inventory / field book | It is difficult to tell what is actually in inventory versus what belongs to the field book. | Make carried possessions distinct from knowledge, recipes and guidance, with clear entry points, labels and quantities. Evaluate the interaction model, not only visual styling. |
| Bob hairstyle | The intended straight blonde bob is closer to Melinoe's haircut in Hades II, not the current "karen hairstyle." | Deferred visual direction for that alternative only; preserve the long brown-haired default. Inspect the reference before specifying cut details. Author an original interpretation, not a copy or import of game assets. |

The remaining table entries are still deferred; the attempted wavy-length
correction does not mean that this or all other feedback has been fixed.
