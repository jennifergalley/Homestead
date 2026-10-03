# Design

## Context
See proposal.md. SHomesteadMapView already selects landmarks and distinguishes
a click from dragging. The menu maps manor/town/store glyphs to existing travel
destinations, and T opens the existing travel confirmation.

## Decisions
Reuse the same OnPlaceAction/OpenTravelPrompt path for a clicked destination and
controller confirm. Keep the existing travel plan, revision check, refusal and
confirmation; do not duplicate travel authority or change signpost behavior.
Unknown/non-travel landmarks remain selectable and keep their zoom behavior.
Preserve map panning, wheel/trigger zoom and tabs navigation.

## Risks / Verification
An action dispatched during mouse release may change focus/capture. Dispatch only
after selecting a hit landmark, through the same menu guard as the action line.
Compile once with the other UI changes; Jenny checks mouse and controller travel.
