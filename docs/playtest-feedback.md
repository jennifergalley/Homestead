# Jenny's playtest feedback

## Current priority

Character appearance and robotic movement need an observed gameplay diagnosis
first. MetaHuman is not a settled next step. Use the current build's normal
movement and interaction recordings as a basis for improvements.

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
| Wavy hairstyle | The current wavy hairstyle is too long; the desired length is approximately the middle of her back. | Deferred to a later hair slice. Retain the wavy brunette direction and shorten its target length to mid-back; no hair geometry changes belong to the movement slice. |

No fixes for these deferred items have been implemented as part of recording
the feedback.
