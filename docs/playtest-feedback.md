# Jenny's playtest feedback

## Current priority

Character appearance and robotic movement need an observed gameplay diagnosis
first. MetaHuman is not a settled next step. Use the current build's normal
movement and interaction recordings as a basis for improvements.

## Safe review entry point (2026-09-20)

`Preview.cmd` exposes the separately verified movement/action candidate with
persistent isolated preview saves, rather than replacing `Play.cmd` or copying
Jenny's original world. `Preview.json` explicitly selects the reviewed package.
This is launch/save-safety integration, not another visual redesign or approval
of the heroine. The rejected face/hair trials remain excluded; mid-back waves,
the blonde-bob request and the UI feedback below remain unmet/deferred.

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
The blonde-bob redesign and other remaining deferred entries below remain deferred.
See `docs\visual-playtesting.md` for rejection and recovery evidence.

## Controller prompt switching - implemented and technically verified (2026-09-20)

The coordinator has explicitly scheduled only this later-feedback item as
`prompts-01`: interaction labels revert to keyboard/mouse while using a controller.
Investigate accepted engine events and shared device-intent state, preserving
actual input, existing layout/text/mappings and the working `review-01` launcher
selection. This does not schedule tearing, book/recipe/inventory redesign or
further cosmetic work.

An isolated engine fixture reproduced the bug on the unchanged runtime:
accepted mouse-X input of `0.01` changed the paused Notes footer from controller
to keyboard labels. `MouseLook` wrote the device flag before checking whether
that screen allowed camera motion; movement and stick-look callbacks also
overwrote the same state. This is distinct from test-only physical-input isolation.

The fix uses one typed accepted-event classifier for the existing shared flag.
Buttons and fresh controller gestures switch immediately; releases/repeats and
zero/noise axes do not. Stick intent honors the inherited per-axis shaping and
existing radial modifier. Mouse intent uses one raw input unit of signed travel
within 120ms, allowing fine fractional motion without accumulating idle noise
forever. Already-held sticks cannot undo deliberate keyboard/mouse intent for
200ms. These rules affect hints only: input dispatch, movement/camera scales,
control deadzones, mappings, layout and glyph text are unchanged.

Both editor and fresh `prompts-01` package pass 70 focused checks, including the
same previously failing sequence and actual fine camera response. The final
rendered comparison was inspected: Notes/context/Settings/Look/planning hints
agree with deliberate use of both devices. All 914 previous action/full-loop
assertions, 308 save/normal-input checks and 40 launcher guards pass. `Preview.json`
intentionally stays on parent-reviewed `review-01` pending coordinator selection.
This reproduces and fixes a concrete cause, not every possible hardware/driver/
large-cursor-warp case or Jenny's physical-controller review.

## Deferred feedback - 2026-09-19

Jenny explicitly marked the following as feedback for later, not an instruction
to change these systems immediately.

| Area | Report | Follow-up when scheduled |
| --- | --- | --- |
| Rendering | Screen tearing / horizontal flickers while moving. | Reproduce and capture the artifact; distinguish presentation tearing from temporal/rendering artifacts before choosing a fix. Do not assume a VSync diagnosis from the description alone. |
| Music and ambience | "Great." | Preserve this as a successful baseline; avoid unnecessary replacement or remixing during unrelated work. |
| Field book / recipes | The book feels overwhelming; learning recipes gradually might be better than exposing all recipes immediately. | Explore progressive disclosure or a learn/unlock flow. This is a proposal, not a confirmed progression design; essential opening-survival actions must remain attainable. |
| Inventory / field book | It is difficult to tell what is actually in inventory versus what belongs to the field book. | Make carried possessions distinct from knowledge, recipes and guidance, with clear entry points, labels and quantities. Evaluate the interaction model, not only visual styling. |
| Bob hairstyle | The intended straight blonde bob is closer to Melinoe's haircut in Hades II, not the current "karen hairstyle." | Deferred visual direction for that alternative only; preserve the long brown-haired default. Inspect the reference before specifying cut details. Author an original interpretation, not a copy or import of game assets. |

The remaining table entries are still deferred; the attempted wavy-length
correction does not mean that this or all other feedback has been fixed.
